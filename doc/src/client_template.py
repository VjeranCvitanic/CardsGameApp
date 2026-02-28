"""
CardsGame – Python Template Client
===================================
Implements the complete interface towards the CardsGame gRPC server.
Replace the on_*() handler stubs with your game logic.

Requirements:
    pip install grpcio grpcio-tools

Generate Python stubs from proto (run from repo root):
    python -m grpc_tools.protoc \
        -I gRPC/proto \
        --python_out=doc/template_client \
        --grpc_python_out=doc/template_client \
        gRPC/proto/cardsGame.proto

Usage:
    python client_template.py <name> <briscola|tressette> <2|4>
"""

import sys
import threading
import grpc

# Generated stubs (run protoc first, see docstring above)
import cardsGame_pb2 as pb
import cardsGame_pb2_grpc as pb_grpc


# ============================================================
# Helpers
# ============================================================

CARD_COLORS = {
    pb.SPADE: "Spade", pb.COPPE: "Coppe",
    pb.DENARI: "Denari", pb.BASTONI: "Bastoni", pb.NO_COLOR: "-",
}
CARD_NUMBERS = {
    pb.ASSO: "A", pb.DUE: "2", pb.TRE: "3", pb.QUATTRO: "4",
    pb.CINQUE: "5", pb.SEI: "6", pb.SETTE: "7",
    pb.FANTE: "J", pb.CAVALLO: "Q", pb.RE: "K",
}
CALLS = {pb.NO_CALL: "", pb.BUSSO: "Busso", pb.STRISCIO: "Striscio",
         pb.CON_QUESTA_BASTA: "ConQuestaBasta"}


def card_str(card: pb.Card) -> str:
    return f"{CARD_COLORS.get(card.color,'?')}{CARD_NUMBERS.get(card.number,'?')}"


# ============================================================
# Game state (plain object, no locking needed – single event thread)
# ============================================================

class GameState:
    def __init__(self):
        self.my_server_id: int = -1      # from Connect response
        self.my_session_id: int = -1     # from START_MATCH_EVENT
        self.teammate_id: int = -1       # -1 for 2-player
        self.game_type: int = pb.INVALID_GAME_TYPE
        self.single_or_multi: int = pb.INVALID_SINGLE_MULTI
        self.briscola_color: int = pb.NO_COLOR

        self.hand: list[pb.Card] = []
        self.legal_cards: list[pb.Card] = []
        self.moves_in_round: list[pb.Move] = []


# ============================================================
# Template client
# ============================================================

class CardsGameTemplateClient:

    def __init__(self, lobby_address: str, player_name: str,
                 game_type: int, single_or_multi: int):
        self.player_name = player_name
        self.state = GameState()
        self.state.game_type = game_type
        self.state.single_or_multi = single_or_multi

        self._lobby_channel = grpc.insecure_channel(lobby_address)
        self._lobby_stub = pb_grpc.CardsGameServerStub(self._lobby_channel)

        self._session_stub: pb_grpc.CardsGameSessionStub | None = None
        self._session_channel = None

    # ----------------------------------------------------------
    # Entry point
    # ----------------------------------------------------------

    def run(self):
        if not self._connect():
            return
        self._event_loop()

    # ==========================================================
    # STEP 1 – Connect to lobby
    # ==========================================================

    def _connect(self) -> bool:
        fmt = pb.GameFormat(
            gameType=self.state.game_type,
            singleOrMulti=self.state.single_or_multi,
        )
        req = pb.ConnectReq(name=self.player_name, gameFormats=[fmt])

        try:
            rsp: pb.ConnectRsp = self._lobby_stub.Connect(req)
        except grpc.RpcError as e:
            print(f"[connect] RPC error: {e.code()} – {e.details()}")
            return False

        if rsp.HasField("fail_reason"):
            reasons = {
                pb.ConnectRsp.EMPTY_NAME: "empty name",
                pb.ConnectRsp.INVALID_NAME_SIZE: "name too long (max 30)",
                pb.ConnectRsp.NAME_ALREADY_TAKEN: "name already taken",
            }
            print(f"[connect] Rejected: {reasons.get(rsp.fail_reason, rsp.fail_reason)}")
            return False

        self.state.my_server_id = rsp.successMsg.playerId
        session_addr = rsp.successMsg.address
        print(f"[connect] OK  serverId={self.state.my_server_id}  session={session_addr}")

        self._session_channel = grpc.insecure_channel(
            session_addr,
            options=[("grpc.wait_for_ready", True)],
        )
        self._session_stub = pb_grpc.CardsGameSessionStub(self._session_channel)
        return True

    # ==========================================================
    # STEP 2 – Subscribe to event stream
    # ==========================================================

    def _event_loop(self):
        info = pb.PlayerInfo(playerId=self.state.my_server_id)
        try:
            stream = self._session_stub.SubscribeEvents(
                info,
                timeout=3600,          # 1 hour deadline
                wait_for_ready=True,
            )
            for event in stream:
                self._dispatch(event)
        except grpc.RpcError as e:
            if e.code() == grpc.StatusCode.OK:
                print("[event_loop] Stream finished cleanly.")
            else:
                print(f"[event_loop] Stream error: {e.code()} – {e.details()}")

    # ==========================================================
    # STEP 3 – Send a move
    # ==========================================================

    def send_move(self, card: pb.Card, call: int = pb.NO_CALL):
        """
        Call this from on_your_turn (or schedule it asynchronously).
        Always use state.my_server_id (the ID from Connect), NOT the session ID.
        Always play a card from state.legal_cards.
        """
        req = pb.PlayMoveReq(
            playerInfo=pb.PlayerInfo(playerId=self.state.my_server_id),
            move=pb.Move(card=card, call=call),
        )
        try:
            self._session_stub.PlayMove(req)
        except grpc.RpcError as e:
            print(f"[send_move] Failed: {e.code()} – {e.details()}")

    # ==========================================================
    # STEP 4 – Event dispatcher
    # ==========================================================

    def _dispatch(self, event: pb.GameEventMsg):
        t = event.eventType
        if   t == pb.START_MATCH_EVENT:          self.on_start_match(event.startMatch, event.playerInfo)
        elif t == pb.START_GAME_EVENT:           self.on_start_game(event.startGame, event.playerInfo)
        elif t == pb.START_ROUND_EVENT:          self.on_start_round(event.startRound, event.playerInfo)
        elif t == pb.PLAYER_DEALT_CARDS_EVENT:   self.on_dealt_cards(event.playerDealtCards, event.playerInfo)
        elif t == pb.YOUR_TURN_EVENT:            self.on_your_turn(event.yourTurn, event.playerInfo)
        elif t == pb.MOVE_RSP_EVENT:             self.on_move_response(event.playMoveRsp, event.playerInfo)
        elif t == pb.PLAYER_PLAYED_MOVE_EVENT:   self.on_player_played_move(event.playerPlayedMove, event.playerInfo)
        elif t == pb.ROUND_OVER_EVENT:           self.on_round_over(event.roundOver, event.playerInfo)
        elif t == pb.GAME_OVER_EVENT:            self.on_game_over(event.gameOver, event.playerInfo)
        elif t == pb.MATCH_OVER_EVENT:           self.on_match_over(event.matchOver, event.playerInfo)
        elif t == pb.ACUSSO_EVENT:               self.on_acusso(event.acussoMsg, event.playerInfo)
        elif t == pb.BRISCOLA_LAST_ROUND_EVENT:  self.on_briscola_last_round(event.briscolaLastRound, event.playerInfo)
        else:
            print(f"[dispatch] Unknown event type: {t}")

    # ==========================================================
    # EVENT HANDLERS – replace with your logic
    # ==========================================================

    def on_start_match(self, msg: pb.StartMatchMsg, addressed_to: pb.PlayerInfo):
        """
        First event received on the stream (always).
        addressed_to.playerId = your session-local ID (0-3).
        msg.gameFormat tells you the game type and format.
        msg.teammateId = -1 for 2-player, (sessionId+2)%4 for 4-player.
        """
        self.state.my_session_id = addressed_to.playerId
        self.state.teammate_id   = msg.teammateId if msg.HasField("teammateId") else -1
        self.state.game_type     = msg.gameFormat.gameType
        self.state.single_or_multi = msg.gameFormat.singleOrMulti

        print(f"[on_start_match] sessionId={self.state.my_session_id} "
              f"teammate={self.state.teammate_id} "
              f"game={'Briscola' if self.state.game_type == pb.BRISCOLA else 'Tressette'}")

        # TODO: initialise your UI / AI

    def on_start_game(self, msg: pb.StartGameMsg, addressed_to: pb.PlayerInfo):
        """
        A new game begins within the match.
        msg.lastCard is set only for Briscola – its color is the trump suit.
        msg.firstToPlayId.playerId = session-local ID of first player.
        """
        if msg.HasField("lastCard"):
            self.state.briscola_color = msg.lastCard.color
            print(f"[on_start_game] Briscola trump: {CARD_COLORS[self.state.briscola_color]}")
        print(f"[on_start_game] firstToPlay={msg.firstToPlayId.playerId}")

        # TODO: reset per-game UI state

    def on_start_round(self, msg: pb.StartRoundMsg, addressed_to: pb.PlayerInfo):
        """
        A new trick begins.
        msg.firstToPlayId.playerId = session-local ID of first player.
        """
        self.state.moves_in_round = []
        print(f"[on_start_round] firstToPlay={msg.firstToPlayId.playerId}")

        # TODO: reset trick display

    def on_dealt_cards(self, msg: pb.PlayerDealtCardsMsg, addressed_to: pb.PlayerInfo):
        """
        Cards were dealt.
        Briscola:   addressed_to.playerId == my_session_id → your own 10 cards.
        Tressette:  you receive OTHER players' dealt cards (full visibility design).
                    msg.playerInfo.playerId identifies the owner.
        """
        if addressed_to.playerId == self.state.my_session_id:
            self.state.hand = list(msg.card)
            print(f"[on_dealt_cards] My hand: {[card_str(c) for c in self.state.hand]}")
        else:
            owner = msg.playerInfo.playerId
            print(f"[on_dealt_cards] Player {owner} dealt: {[card_str(c) for c in msg.card]}")

        # TODO: update hand display

    def on_your_turn(self, msg: pb.YourTurnMsg, addressed_to: pb.PlayerInfo):
        """
        It is your turn to play.
        msg.hand             – your current hand
        msg.legalCards       – ONLY cards you may play (ALWAYS pick from here)
        msg.movesPlayedInRound – moves so far this trick
        msg.strongColor      – trump color (NO_COLOR for Tressette)

        You MUST call self.send_move() before returning (or in a thread).
        """
        self.state.hand           = list(msg.hand)
        self.state.legal_cards    = list(msg.legalCards)
        self.state.moves_in_round = list(msg.movesPlayedInRound)

        print(f"[on_your_turn] hand={[card_str(c) for c in self.state.hand]}")
        print(f"               legal={[card_str(c) for c in self.state.legal_cards]}")

        # ---- Example: always play the first legal card, no call ----
        if self.state.legal_cards:
            self.send_move(self.state.legal_cards[0], pb.NO_CALL)

        # TODO: replace with your decision logic (AI, human input, etc.)

    def on_move_response(self, msg: pb.PlayMoveRspMsg, addressed_to: pb.PlayerInfo):
        """
        Server's acknowledgement of your PlayMove call.
        msg.moveRsp values:
            MOVE_OK                   – accepted, wait for next event
            NOT_YOUR_TURN             – sent move out of turn
            CARD_NOT_IN_HAND          – card not in your hand
            COLOR_CONSTRAINT_NOT_MET  – must follow the lead suit
            CANT_CALL_IF_NOT_FIRST_OF_HAND – calls only on first card of trick
        """
        if msg.moveRsp != pb.MOVE_OK:
            rsp_names = {
                pb.NOT_YOUR_TURN: "NOT_YOUR_TURN",
                pb.CARD_NOT_IN_HAND: "CARD_NOT_IN_HAND",
                pb.COLOR_CONSTRAINT_NOT_MET: "COLOR_CONSTRAINT_NOT_MET",
                pb.CANT_CALL_IF_NOT_FIRST_OF_HAND: "CANT_CALL_IF_NOT_FIRST_OF_HAND",
            }
            print(f"[on_move_response] INVALID: {rsp_names.get(msg.moveRsp, msg.moveRsp)}")
            # TODO: re-trigger move selection
        else:
            print("[on_move_response] MOVE_OK")

    def on_player_played_move(self, msg: pb.PlayerPlayedMoveMsg, addressed_to: pb.PlayerInfo):
        """
        Another player just played a card.
        msg.playerInfo.playerId = session-local ID of the mover (never yourself).
        msg.move = their card + call.
        """
        print(f"[on_player_played_move] player={msg.playerInfo.playerId} "
              f"card={card_str(msg.move.card)} call={CALLS.get(msg.move.call,'')}")

        # TODO: update shared board state, track opponent hands

    def on_round_over(self, msg: pb.RoundOverMsg, addressed_to: pb.PlayerInfo):
        """
        A trick just ended.
        msg.winnerId.playerId = session-local ID of the trick winner.
        msg.points.punti      = points in this trick.
        msg.points.bella      = bella bonus (Tressette only).
        """
        print(f"[on_round_over] winner={msg.winnerId.playerId} "
              f"punti={msg.points.punti} bella={msg.points.bella}")

        # TODO: update running score

    def on_game_over(self, msg: pb.GameOverMsg, addressed_to: pb.PlayerInfo):
        """
        A single game (within a match) has ended.
        msg.teamWinnerId       = winning team (0 or 1).
        msg.points[i].teamId   = team index.
        msg.points[i].points   = accumulated points for that team.
        """
        print(f"[on_game_over] winningTeam={msg.teamWinnerId}")
        for tp in msg.points:
            print(f"  team {tp.teamId}: punti={tp.points.punti}")

        # TODO: display game result

    def on_match_over(self, msg: pb.MatchOverMsg, addressed_to: pb.PlayerInfo):
        """
        The entire match is over.
        msg.teamWinnerId          = overall winning team.
        msg.score[i].numWonGames  = games won (Briscola).
        msg.score[i].points       = cumulative points (Tressette).
        """
        print(f"[on_match_over] winningTeam={msg.teamWinnerId}")
        for s in msg.score:
            if s.HasField("numWonGames"):
                print(f"  team {s.teamId}: wonGames={s.numWonGames}")
            else:
                print(f"  team {s.teamId}: punti={s.points.punti}")

        # TODO: show final screen / exit

    def on_acusso(self, msg: pb.AcussosMsg, addressed_to: pb.PlayerInfo):
        """
        Tressette only. A player declared their acussos.
        You receive this for every player EXCEPT yourself.
        msg.AcussoPlayerId.playerId = session-local ID of the declaring player.
        msg.acusso[i].acussoType    = type enum.
        msg.acusso[i].acussoPoints  = bonus points.
        """
        print(f"[on_acusso] player={msg.AcussoPlayerId.playerId} "
              f"count={len(msg.acusso)}")

        # TODO: track declared acussos for running score

    def on_briscola_last_round(self, msg: pb.BriscolaLastRoundMsg,
                               addressed_to: pb.PlayerInfo):
        """
        Briscola 4-player only, last trick: your teammate reveals their hand.
        msg.teammateId.playerId = session-local ID of your teammate.
        msg.cards               = their remaining cards.
        """
        print(f"[on_briscola_last_round] teammate={msg.teammateId.playerId} "
              f"cards={[card_str(c) for c in msg.cards]}")

        # TODO: use teammate's hand info to inform your strategy


# ============================================================
# main
# ============================================================

def main():
    if len(sys.argv) != 4:
        print(f"Usage: python {sys.argv[0]} <name> <briscola|tressette> <2|4>")
        sys.exit(1)

    name       = sys.argv[1]
    game_str   = sys.argv[2].lower()
    n_players  = int(sys.argv[3])

    game_type_map = {"briscola": pb.BRISCOLA, "tressette": pb.TRESSETTE}
    if game_str not in game_type_map:
        print("game type must be 'briscola' or 'tressette'"); sys.exit(1)

    sm_map = {2: pb.SINGLE, 4: pb.MULTI}
    if n_players not in sm_map:
        print("numPlayers must be 2 or 4"); sys.exit(1)

    client = CardsGameTemplateClient(
        lobby_address="localhost:50051",
        player_name=name,
        game_type=game_type_map[game_str],
        single_or_multi=sm_map[n_players],
    )
    client.run()


if __name__ == "__main__":
    main()
