"""
End-to-End tests – Full game flow over real gRPC
==================================================
Spins up the real server binary, runs N AI clients, and verifies
the complete event sequence from connect to MATCH_OVER.

Prerequisites:
    pip install grpcio grpcio-tools pytest

Generate Python stubs (once, from repo root):
    python -m grpc_tools.protoc \
        -I gRPC/proto \
        --python_out=tests/e2e \
        --grpc_python_out=tests/e2e \
        gRPC/proto/cardsGame.proto

Run:
    pytest tests/e2e/test_full_game.py -v
    pytest tests/e2e/test_full_game.py -v -k briscola_2p

Environment:
    SERVER_BIN   path to the server binary  (default: ./build/server)
    SERVER_PORT  lobby port                  (default: 50051)
"""

import os
import subprocess
import sys
import threading
import time
import queue
import uuid

import grpc
import pytest

# Adjust path so we can import generated stubs from this directory
sys.path.insert(0, os.path.dirname(__file__))
import cardsGame_pb2 as pb
import cardsGame_pb2_grpc as pb_grpc

# ============================================================
# Configuration
# ============================================================

SERVER_BIN  = os.environ.get("SERVER_BIN",  "./build/server")
SERVER_PORT = int(os.environ.get("SERVER_PORT", 50051))
LOBBY_ADDR  = f"localhost:{SERVER_PORT}"
STARTUP_TIMEOUT_S = 5
GAME_TIMEOUT_S    = 30


# ============================================================
# Server fixture
# ============================================================

@pytest.fixture(scope="module")
def server_process():
    """Start the server binary once per test module."""
    proc = subprocess.Popen(
        [SERVER_BIN],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    # Wait for server to be ready
    deadline = time.time() + STARTUP_TIMEOUT_S
    channel = grpc.insecure_channel(LOBBY_ADDR)
    while time.time() < deadline:
        try:
            grpc.channel_ready_future(channel).result(timeout=1)
            break
        except grpc.FutureTimeoutError:
            pass
    else:
        proc.terminate()
        pytest.fail(f"Server '{SERVER_BIN}' did not start within {STARTUP_TIMEOUT_S}s")

    yield proc

    proc.terminate()
    try:
        proc.wait(timeout=5)
    except subprocess.TimeoutExpired:
        proc.kill()


# ============================================================
# AI client helper
# ============================================================

class EventRecorder:
    """Plays a full game as a random-move AI and records every event."""

    def __init__(self, name: str, game_type: int, single_or_multi: int):
        self.name            = name
        self.game_type       = game_type
        self.single_or_multi = single_or_multi
        self.events: list[pb.GameEventMsg] = []
        self.error: Exception | None = None
        self.my_server_id    = -1
        self.my_session_id   = -1
        self._done           = threading.Event()

    def run(self):
        try:
            self._run_impl()
        except Exception as exc:
            self.error = exc
        finally:
            self._done.set()

    def wait(self, timeout: float = GAME_TIMEOUT_S) -> bool:
        return self._done.wait(timeout)

    # ----------------------------------------------------------

    def _run_impl(self):
        # 1. Connect to lobby
        lobby_ch   = grpc.insecure_channel(LOBBY_ADDR)
        lobby_stub = pb_grpc.CardsGameServerStub(lobby_ch)

        fmt = pb.GameFormat(gameType=self.game_type, singleOrMulti=self.single_or_multi)
        rsp = lobby_stub.Connect(pb.ConnectReq(name=self.name, gameFormats=[fmt]))

        assert rsp.HasField("successMsg"), \
            f"[{self.name}] Connect failed: {rsp.fail_reason}"

        self.my_server_id = rsp.successMsg.playerId
        session_addr      = rsp.successMsg.address

        # 2. Subscribe to session events
        sess_ch   = grpc.insecure_channel(session_addr,
                                          options=[("grpc.wait_for_ready", True)])
        sess_stub = pb_grpc.CardsGameSessionStub(sess_ch)

        stream = sess_stub.SubscribeEvents(
            pb.PlayerInfo(playerId=self.my_server_id),
            timeout=GAME_TIMEOUT_S,
            wait_for_ready=True,
        )

        # 3. Consume event stream
        for event in stream:
            self.events.append(event)
            self._handle(event, sess_stub)

    def _handle(self, event: pb.GameEventMsg, stub: pb_grpc.CardsGameSessionStub):
        if event.eventType == pb.START_MATCH_EVENT:
            self.my_session_id = event.playerInfo.playerId

        elif event.eventType == pb.YOUR_TURN_EVENT:
            legal = list(event.yourTurn.legalCards)
            if not legal:
                legal = list(event.yourTurn.hand)  # fallback (should not happen)

            import random
            chosen = random.choice(legal)
            stub.PlayMove(pb.PlayMoveReq(
                playerInfo=pb.PlayerInfo(playerId=self.my_server_id),
                move=pb.Move(card=chosen, call=pb.NO_CALL),
            ))


# ============================================================
# Helpers to inspect event sequences
# ============================================================

def events_of_type(recorder: EventRecorder, t: int) -> list[pb.GameEventMsg]:
    return [e for e in recorder.events if e.eventType == t]


def assert_event_sequence(recorder: EventRecorder,
                          expected_types: list[int],
                          msg_prefix: str = ""):
    """Assert that at least one occurrence of each expected type appears, in order."""
    idx = 0
    for t in expected_types:
        found = False
        while idx < len(recorder.events):
            if recorder.events[idx].eventType == t:
                found = True
                idx += 1
                break
            idx += 1
        assert found, (
            f"{msg_prefix} Missing event type {t} in sequence.\n"
            f"Full sequence: {[e.eventType for e in recorder.events]}"
        )


# ============================================================
# Tests
# ============================================================

class TestBriscola2Player:

    @pytest.fixture(autouse=True)
    def _clients(self, server_process):
        """Run two AI clients in parallel for a 2-player Briscola game."""
        run_id = uuid.uuid4().hex[:8]
        c1 = EventRecorder(f"Alice_{run_id}", pb.BRISCOLA, pb.SINGLE)
        c2 = EventRecorder(f"Bob_{run_id}",   pb.BRISCOLA, pb.SINGLE)

        t1 = threading.Thread(target=c1.run, daemon=True)
        t2 = threading.Thread(target=c2.run, daemon=True)
        t1.start()
        t2.start()

        assert c1.wait(GAME_TIMEOUT_S), "Client Alice did not finish in time"
        assert c2.wait(GAME_TIMEOUT_S), "Client Bob did not finish in time"

        assert c1.error is None, f"Alice error: {c1.error}"
        assert c2.error is None, f"Bob error: {c2.error}"

        self.c1 = c1
        self.c2 = c2

    # ----------------------------------------------------------

    def test_both_receive_start_match(self):
        assert events_of_type(self.c1, pb.START_MATCH_EVENT), "Alice missing START_MATCH"
        assert events_of_type(self.c2, pb.START_MATCH_EVENT), "Bob missing START_MATCH"

    def test_game_type_is_briscola(self):
        ev = events_of_type(self.c1, pb.START_MATCH_EVENT)[0]
        assert ev.startMatch.gameFormat.gameType == pb.BRISCOLA

    def test_start_game_has_briscola_card(self):
        ev = events_of_type(self.c1, pb.START_GAME_EVENT)
        assert ev, "No START_GAME_EVENT received"
        assert ev[0].startGame.HasField("lastCard"), \
            "Briscola START_GAME_EVENT must have lastCard"
        assert ev[0].startGame.lastCard.color != pb.NO_COLOR, \
            "lastCard color must not be NO_COLOR"

    def test_both_receive_dealt_cards(self):
        # In Briscola, each player receives ONLY their own dealt cards
        dealt1 = events_of_type(self.c1, pb.PLAYER_DEALT_CARDS_EVENT)
        dealt2 = events_of_type(self.c2, pb.PLAYER_DEALT_CARDS_EVENT)
        assert dealt1, "Alice received no PLAYER_DEALT_CARDS_EVENT"
        assert dealt2, "Bob received no PLAYER_DEALT_CARDS_EVENT"

    def test_your_turn_contains_legal_cards(self):
        turns = events_of_type(self.c1, pb.YOUR_TURN_EVENT)
        assert turns, "Alice never received YOUR_TURN_EVENT"
        for t in turns:
            assert t.yourTurn.legalCards, \
                "YOUR_TURN_EVENT must contain at least one legal card"

    def test_move_responses_are_ok(self):
        for c in (self.c1, self.c2):
            move_rsps = events_of_type(c, pb.MOVE_RSP_EVENT)
            for rsp in move_rsps:
                assert rsp.playMoveRsp.moveRsp == pb.MOVE_OK, \
                    f"{c.name}: Got non-OK move response: {rsp.playMoveRsp.moveRsp}"

    def test_game_ends_with_match_over(self):
        assert events_of_type(self.c1, pb.MATCH_OVER_EVENT), \
            "Alice never received MATCH_OVER_EVENT"
        assert events_of_type(self.c2, pb.MATCH_OVER_EVENT), \
            "Bob never received MATCH_OVER_EVENT"

    def test_match_over_declares_winner(self):
        ev = events_of_type(self.c1, pb.MATCH_OVER_EVENT)[0]
        assert ev.matchOver.HasField("teamWinnerId"), \
            "MATCH_OVER_EVENT must declare a winner"

    def test_event_sequence_order(self):
        # Minimum required ordering for any Briscola game
        assert_event_sequence(self.c1, [
            pb.START_MATCH_EVENT,
            pb.START_GAME_EVENT,
            pb.PLAYER_DEALT_CARDS_EVENT,
            pb.START_ROUND_EVENT,
            pb.YOUR_TURN_EVENT,
            pb.ROUND_OVER_EVENT,
            pb.MATCH_OVER_EVENT,
        ], msg_prefix="[Alice]")

    def test_player_played_move_not_sent_to_mover(self):
        """The player who played should NOT receive their own PLAYER_PLAYED_MOVE_EVENT."""
        for c in (self.c1, self.c2):
            my_moves = [
                e for e in events_of_type(c, pb.PLAYER_PLAYED_MOVE_EVENT)
                if e.playerPlayedMove.playerInfo.playerId == c.my_session_id
            ]
            assert not my_moves, \
                f"{c.name}: Received own PLAYER_PLAYED_MOVE_EVENT (should not)"

    def test_no_unexpected_events(self):
        allowed = {
            pb.START_MATCH_EVENT,
            pb.START_GAME_EVENT,
            pb.START_ROUND_EVENT,
            pb.PLAYER_DEALT_CARDS_EVENT,
            pb.YOUR_TURN_EVENT,
            pb.MOVE_RSP_EVENT,
            pb.PLAYER_PLAYED_MOVE_EVENT,
            pb.ROUND_OVER_EVENT,
            pb.GAME_OVER_EVENT,
            pb.MATCH_OVER_EVENT,
        }
        for c in (self.c1, self.c2):
            unexpected = {e.eventType for e in c.events} - allowed
            assert not unexpected, \
                f"{c.name}: Unexpected event types received: {unexpected}"


class TestConnectValidation:
    """Unit-level validation of Connect RPC (no game session needed)."""

    @pytest.fixture(autouse=True)
    def _stub(self, server_process):
        ch = grpc.insecure_channel(LOBBY_ADDR)
        self.stub = pb_grpc.CardsGameServerStub(ch)

    def test_empty_name_rejected(self):
        fmt = pb.GameFormat(gameType=pb.BRISCOLA, singleOrMulti=pb.SINGLE)
        with pytest.raises(grpc.RpcError) as exc_info:
            self.stub.Connect(pb.ConnectReq(name="", gameFormats=[fmt]))
        assert exc_info.value.code() == grpc.StatusCode.INVALID_ARGUMENT

    def test_name_too_long_rejected(self):
        long_name = "A" * 31
        fmt = pb.GameFormat(gameType=pb.BRISCOLA, singleOrMulti=pb.SINGLE)
        with pytest.raises(grpc.RpcError) as exc_info:
            self.stub.Connect(pb.ConnectReq(name=long_name, gameFormats=[fmt]))
        assert exc_info.value.code() == grpc.StatusCode.INVALID_ARGUMENT

    def test_duplicate_name_rejected(self):
        fmt = pb.GameFormat(gameType=pb.TRESSETTE, singleOrMulti=pb.SINGLE)
        name = f"DuplicateTest_{uuid.uuid4().hex[:8]}"
        # First connect should succeed
        rsp1 = self.stub.Connect(pb.ConnectReq(name=name, gameFormats=[fmt]))
        assert rsp1.HasField("successMsg")
        # Second connect with same name should fail
        with pytest.raises(grpc.RpcError) as exc_info:
            self.stub.Connect(pb.ConnectReq(name=name, gameFormats=[fmt]))
        assert exc_info.value.code() == grpc.StatusCode.ALREADY_EXISTS

    def test_successful_connect_returns_player_id_and_address(self):
        fmt = pb.GameFormat(gameType=pb.BRISCOLA, singleOrMulti=pb.SINGLE)
        rsp = self.stub.Connect(pb.ConnectReq(
            name=f"ValidPlayer_{uuid.uuid4().hex[:8]}", gameFormats=[fmt]
        ))
        assert rsp.HasField("successMsg")
        assert rsp.successMsg.playerId >= 0
        assert ":" in rsp.successMsg.address, \
            "address must be in host:port format"
