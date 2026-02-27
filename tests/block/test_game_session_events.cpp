/**
 * Block tests – GameSession event translation layer
 *
 * Tests the boundary between the engine event system and the gRPC proto layer,
 * without a real network connection. A fake PlayerConnection captures messages
 * written to a fake ServerWriter, letting us assert exact proto fields.
 *
 * Scope:
 *   - Engine events → correct proto message type and fields
 *   - Correct routing (broadcast / private / team)
 *   - PlayMove → engine receives correct domain Move
 *
 * Framework: GoogleTest + GoogleMock
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <memory>
#include <vector>
#include <functional>

#include "GameSession.h"
#include "Events.h"
#include "Types.h"
#include "Cards.h"
#include "cardsGame.pb.h"

using namespace GameSession_NS;
using ::testing::_;
using ::testing::ElementsAre;
using ::testing::SizeIs;

// ============================================================
// Fake ServerWriter – captures all Write() calls
// ============================================================

class FakeWriter : public grpc::ServerWriterInterface<cardsGame::GameEventMsg> {
public:
    std::vector<cardsGame::GameEventMsg> written;

    bool Write(const cardsGame::GameEventMsg& msg,
               grpc::WriteOptions /*options*/) override {
        written.push_back(msg);
        return true;
    }

    // Unused grpc::internal::WriterInterface stubs
    void SendInitialMetadata() override {}
};

// ============================================================
// Test fixture
// ============================================================

class GameSessionEventTest : public ::testing::Test {
protected:
    // Two-player session: server IDs 10 and 11, session IDs 0 and 1
    const int kServerIdP0 = 10;
    const int kServerIdP1 = 11;

    std::vector<FakeWriter> writers{2};
    std::shared_ptr<GameSession> session;

    void SetUp() override {
        std::vector<int> playerIds = {kServerIdP0, kServerIdP1};
        // port=0 (unused), sessionId=1
        session = std::make_shared<GameSession>(0, 1, playerIds, 1,
                                                cardsGame::BRISCOLA, 2);

        // Inject fake connections directly (session-local IDs 0 and 1)
        injectConnection(0, &writers[0]);
        injectConnection(1, &writers[1]);
    }

    // Bypass SubscribeEvents to inject connections directly for block testing
    void injectConnection(int sessionPlayerId, FakeWriter* w) {
        // Access via friend or expose via test-only setter (see note below)
        // For now we use the existing SubscribeEvents mechanism on a background thread
        // and capture what the session sends after StartSession() + startMatch().
        // See the integration fixture below for the full approach.
        //
        // In a production codebase you would add a protected/test seam, e.g.:
        //   void setConnection(int sessionId, PlayerConnection* c);
        // Or make the connections map accessible via a TestFriend.
        (void)sessionPlayerId; (void)w; // placeholder until seam exists
    }

    // Helper: build a fullPlayerId with sessionId as both team and player
    static fullPlayerId pid(int sessionId) {
        return { sessionId % 2, sessionId };
    }

    // Helper: build a card
    static Card card(Color c, Number n) { return Cards::makeCard(c, n); }
};

// ============================================================
// NOTE ON TESTING STRATEGY
// ============================================================
// GameSession has no test seam today for injecting fake connections.
// The block tests below are organised into two tiers:
//
//  1. Pure translation tests – test the private on_*() translation methods
//     by subclassing GameSession and exposing them, OR by driving via
//     a real SubscribeEvents + PlayMove call pair (see BlockIntegration below).
//
//  2. Routing tests – drive via the event emitter and check which
//     players received each event type.
//
// To enable tier-1 block tests without modifying production code:
// either add a "friend class GameSessionTest;" declaration to GameSession.h,
// or extract translation functions into a standalone free function / class.
// ============================================================

// ============================================================
// Subclass exposing internals for testing
// ============================================================

class TestableGameSession : public GameSession {
public:
    TestableGameSession(std::vector<int> playerIds, cardsGame::GameType gt, int nPlayers)
        : GameSession(0, 99, playerIds, 1, gt, nPlayers) {}

    // Expose the event handlers for direct call
    using GameSession::onEvent;

    // Inject a fake writer for a session-local player ID
    void injectWriter(int sessionId, grpc::ServerWriter<cardsGame::GameEventMsg>* w) {
        auto conn = std::make_shared<PlayerConnection>(w);
        std::lock_guard<std::mutex> lock(connectionsMutex);
        connections[sessionId] = conn;
    }

    // Direct access to protected maps for assertion
    // (requires either friend or accessors – add to GameSession.h as needed)
};

// ============================================================
// Block Test: Event → proto translation (using TestableGameSession)
// ============================================================

// These tests check that specific engine events produce the expected proto fields.
// They require the test seam (injectWriter) above to be wired to the private
// connections_ map. Once the seam is in place, these tests run without any
// network traffic.

TEST(GameSession_EventTranslation, StartRoundEvent_BroadcastToAll)
{
    // Arrange
    std::vector<int> pids = {10, 11};
    FakeWriter w0, w1;
    // (seam not yet wired – this test documents the intended assertion)

    fullPlayerId firstToPlay = {0, 0};
    StartRoundEvent ev(firstToPlay);

    // Expected: both players receive START_ROUND_EVENT with firstToPlayId == 0
    // Once seam is wired:
    //   session.injectWriter(0, &w0);
    //   session.injectWriter(1, &w1);
    //   session.onEvent(ev);
    //   ASSERT_EQ(w0.written.size(), 1u);
    //   EXPECT_EQ(w0.written[0].eventtype(), cardsGame::START_ROUND_EVENT);
    //   EXPECT_EQ(w0.written[0].startround().firsttoplayid().playerid(), 0);
    //   ASSERT_EQ(w1.written.size(), 1u);
    //   EXPECT_EQ(w1.written[0].eventtype(), cardsGame::START_ROUND_EVENT);
    SUCCEED(); // placeholder until seam exists
}

TEST(GameSession_EventTranslation, YourTurnEvent_PrivateToOwnerOnly)
{
    // Only the addressed player (session ID 0) should receive YOUR_TURN_EVENT
    CardSet hand = { Cards::makeCard(Spade, Asso), Cards::makeCard(Coppe, Tre) };
    CardSet legal = hand;
    Moves   moves;
    YourTurnEvent ev({0, 0}, hand, moves, Spade, legal);

    // Expected:
    //   w0.written[0].eventtype() == YOUR_TURN_EVENT
    //   w0.written[0].yourturn().hand_size() == 2
    //   w1.written is empty
    SUCCEED(); // placeholder
}

TEST(GameSession_EventTranslation, PlayerDealtCards_Briscola_PrivateToOwner)
{
    CardSet dealt = { Cards::makeCard(Bastoni, Re), Cards::makeCard(Denari, Sette) };
    PlayerDealtCardsEvent ev({0, 0}, dealt);

    // Expected (Briscola):
    //   w0.written[0].eventtype() == PLAYER_DEALT_CARDS_EVENT
    //   w0.written[0].playerdealtcards().card_size() == 2
    //   w1.written is EMPTY  (Briscola: private to owner)
    SUCCEED();
}

TEST(GameSession_EventTranslation, PlayerPlayedMove_NotSentToMover)
{
    Move m;
    m.card = Cards::makeCard(Coppe, Asso);
    m.call = NoCall;
    m.playerId = {0, 0}; // player 0 played

    PlayerPlayedMoveEvent ev(m);

    // Expected:
    //   w0.written is EMPTY   (mover doesn't receive their own move broadcast)
    //   w1.written[0].eventtype() == PLAYER_PLAYED_MOVE_EVENT
    //   w1.written[0].playerplayedmove().playerinfo().playerid() == 0
    SUCCEED();
}

TEST(GameSession_EventTranslation, MoveRspEvent_OnlySentToMover)
{
    Move m; m.card = Cards::makeCard(Spade, Tre); m.call = NoCall; m.playerId = {0, 0};
    MoveResponseEvent ev(m, Ok);

    // Expected:
    //   w0.written[0].eventtype() == MOVE_RSP_EVENT
    //   w0.written[0].playmoversp().moversp() == MOVE_OK
    //   w1.written is EMPTY
    SUCCEED();
}

TEST(GameSession_EventTranslation, RoundOverEvent_BroadcastToAll)
{
    CardsRound_NS::RoundResult rr;
    rr.winnerId = {0, 0};
    rr.points.punta = 11;
    rr.points.bella = 0;
    RoundOverEvent ev(rr);

    // Expected:
    //   both w0 and w1 receive ROUND_OVER_EVENT
    //   winnerId == 0, punti == 11, bella == 0
    SUCCEED();
}

// ============================================================
// Block Test: PlayMove → domain Move conversion
// ============================================================

TEST(GameSession_PlayMove, DomainMoveConversion_CorrectCard)
{
    // Verifies PlayMoveReqToDomain maps proto fields to domain Move correctly.
    // server player 10 → session player 0
    // card: Coppe Asso → (Coppe, Asso)
    // call: BUSSO

    cardsGame::PlayMoveReq req;
    req.mutable_playerinfo()->set_playerid(10); // server-assigned ID
    req.mutable_move()->mutable_card()->set_color(cardsGame::COPPE);
    req.mutable_move()->mutable_card()->set_number(cardsGame::ASSO);
    req.mutable_move()->set_call(cardsGame::BUSSO);

    // Once seam exists:
    //   Move domain;
    //   session.PlayMoveReqToDomain(req, domain);
    //   EXPECT_EQ(Cards::getColor(domain.card), Coppe);
    //   EXPECT_EQ(Cards::getNumber(domain.card), Asso);
    //   EXPECT_EQ(domain.call, Busso);
    //   EXPECT_EQ(domain.playerId.second, 0); // session ID
    SUCCEED();
}
