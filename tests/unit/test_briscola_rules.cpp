/**
 * Unit tests – BriscolaRules
 *
 * Tests the pure rule logic in isolation: no game state, no network, no events.
 * These are the fastest tests and should catch logic regressions immediately.
 *
 * Framework: GoogleTest
 * Build: added via tests/CMakeLists.txt
 */

#include <gtest/gtest.h>
#include "Cards.h"
#include "BriscolaRules.h"
#include "Types.h"

// ============================================================
// Helpers
// ============================================================

static BriscolaRules& rules() { return BriscolaRules::instance(); }

// Build a minimal RoundState with one player owning a given hand
static CardsRound_NS::RoundState makeState(
    CardSet hand,
    Moves   movesPlayed = {},
    Color   strong      = Spade,
    fullPlayerId nextToPlay = {0, 0})
{
    CardsRound_NS::PlayerState ps;
    ps.playerId = {0, 0};
    ps.deck     = Deck();  // Empty deck
    for (const Card& c : hand)
        ps.deck.AddCard(c);

    CardsRound_NS::RoundState state(nextToPlay, {ps});
    state.playedMovesInRound = movesPlayed;
    state.strongColor        = strong;
    return state;
}

// ============================================================
// Card strength ordering (Briscola)
// Weakest → strongest: Due(1) < Quattro(2) < Cinque(3) < Sei(4) < Sette(5)
//                      < Fante(6) < Cavallo(7) < Re(8) < Tre(9) < Asso(10)
// Note: Due=2 is the WEAKEST card in Briscola (unusual vs other games)
// ============================================================

TEST(BriscolaRules_Strength, AceIsStrongest)
{
    EXPECT_GT(rules().getNumberStrength(Asso),   rules().getNumberStrength(Re));
    EXPECT_GT(rules().getNumberStrength(Asso),   rules().getNumberStrength(Due));
    EXPECT_GT(rules().getNumberStrength(Asso),   rules().getNumberStrength(Tre));
    EXPECT_GT(rules().getNumberStrength(Asso),   rules().getNumberStrength(Sette));
}

TEST(BriscolaRules_Strength, ThreeIsSecondStrongest)
{
    // Tre beats everything except Asso
    EXPECT_GT(rules().getNumberStrength(Tre), rules().getNumberStrength(Re));
    EXPECT_GT(rules().getNumberStrength(Tre), rules().getNumberStrength(Cavallo));
    EXPECT_GT(rules().getNumberStrength(Tre), rules().getNumberStrength(Due));
    EXPECT_LT(rules().getNumberStrength(Tre), rules().getNumberStrength(Asso));
}

TEST(BriscolaRules_Strength, DueIsWeakest)
{
    // Due has strength 1 – weakest card in Briscola
    for (auto n : {Asso, Tre, Re, Cavallo, Fante, Sette, Sei, Cinque, Quattro})
        EXPECT_GT(rules().getNumberStrength(n), rules().getNumberStrength(Due));
}

TEST(BriscolaRules_Strength, QuattroIsSecondWeakest)
{
    // Quattro has strength 2 – only Due is weaker
    EXPECT_GT(rules().getNumberStrength(Quattro), rules().getNumberStrength(Due));
    for (auto n : {Asso, Tre, Re, Cavallo, Fante, Sette, Sei, Cinque})
        EXPECT_GT(rules().getNumberStrength(n), rules().getNumberStrength(Quattro));
}

// ============================================================
// Card point values (Briscola)
// A=11, 3=10, K=4, Q=3, J=2, others=0
// ============================================================

TEST(BriscolaRules_Points, AceWorth11)   { EXPECT_EQ(rules().getNumberValue(Asso).punta,   11); }
TEST(BriscolaRules_Points, ThreeWorth10) { EXPECT_EQ(rules().getNumberValue(Tre).punta,    10); }
TEST(BriscolaRules_Points, KingWorth4)   { EXPECT_EQ(rules().getNumberValue(Re).punta,      4); }
TEST(BriscolaRules_Points, QueenWorth3)  { EXPECT_EQ(rules().getNumberValue(Cavallo).punta, 3); }
TEST(BriscolaRules_Points, JackWorth2)   { EXPECT_EQ(rules().getNumberValue(Fante).punta,   2); }
TEST(BriscolaRules_Points, SevenWorth0)  { EXPECT_EQ(rules().getNumberValue(Sette).punta,   0); }
TEST(BriscolaRules_Points, FourWorth0)   { EXPECT_EQ(rules().getNumberValue(Quattro).punta, 0); }

// ============================================================
// StrongerCard – same suit
// ============================================================

TEST(BriscolaRules_StrongerCard, AceBeatsSeven_SameSuit)
{
    Card ace   = Cards::makeCard(Coppe, Asso);
    Card seven = Cards::makeCard(Coppe, Sette);
    EXPECT_EQ(rules().StrongerCard(ace, seven, Spade), ace);
    EXPECT_EQ(rules().StrongerCard(seven, ace, Spade), ace);
}

TEST(BriscolaRules_StrongerCard, KingBeatsDue_SameSuit)
{
    // Due is the WEAKEST card (strength 1); Re (strength 8) beats it
    Card two  = Cards::makeCard(Denari, Due);
    Card king = Cards::makeCard(Denari, Re);
    EXPECT_EQ(rules().StrongerCard(two, king, Spade), king);
    EXPECT_EQ(rules().StrongerCard(king, two, Spade), king);
}

TEST(BriscolaRules_StrongerCard, AceBeatsKing_SameSuit)
{
    Card ace  = Cards::makeCard(Coppe, Asso);
    Card king = Cards::makeCard(Coppe, Re);
    EXPECT_EQ(rules().StrongerCard(ace, king, Spade), ace);
}

// ============================================================
// StrongerCard – briscola beats off-suit
// ============================================================

TEST(BriscolaRules_StrongerCard, BriscolaBeatsSameSuitAce)
{
    Card briscolaFour  = Cards::makeCard(Spade, Quattro); // weakest briscola
    Card offSuitAce    = Cards::makeCard(Coppe, Asso);    // strongest non-briscola
    Color strong       = Spade;
    EXPECT_EQ(rules().StrongerCard(briscolaFour, offSuitAce, strong), briscolaFour);
}

TEST(BriscolaRules_StrongerCard, BriscolaAceBeatsAllBriscola)
{
    Card briscolaAce  = Cards::makeCard(Spade, Asso);
    Card briscolaKing = Cards::makeCard(Spade, Re);
    EXPECT_EQ(rules().StrongerCard(briscolaAce, briscolaKing, Spade), briscolaAce);
}

// ============================================================
// IsMoveLegal – Briscola has no constraint (any card is legal)
// ============================================================

TEST(BriscolaRules_IsMoveLegal, AnyCardLegal_FirstMove)
{
    CardSet hand = {
        Cards::makeCard(Coppe, Asso),
        Cards::makeCard(Spade, Tre),
        Cards::makeCard(Denari, Quattro),
    };
    auto state = makeState(hand);

    for (const Card& c : hand) {
        Move m; m.card = c; m.call = NoCall; m.playerId = {0, 0};
        MoveReturnValue reason = Ok;
        EXPECT_TRUE(rules().IsMoveLegal(m, state, reason));
        EXPECT_EQ(reason, Ok);
    }
}

TEST(BriscolaRules_IsMoveLegal, CardNotInHand_Rejected)
{
    CardSet hand = { Cards::makeCard(Coppe, Asso) };
    auto state   = makeState(hand);

    Move m;
    m.card     = Cards::makeCard(Spade, Re); // NOT in hand
    m.call     = NoCall;
    m.playerId = {0, 0};

    MoveReturnValue reason = Ok;
    EXPECT_FALSE(rules().IsMoveLegal(m, state, reason));
    EXPECT_EQ(reason, CardNotInHand);
}

// ============================================================
// calculateLegalMoves – Briscola always returns full hand
// ============================================================

TEST(BriscolaRules_LegalMoves, FullHandIsLegal_NoConstraint)
{
    CardSet hand = {
        Cards::makeCard(Coppe, Asso),
        Cards::makeCard(Spade, Tre),
        Cards::makeCard(Denari, Sette),
        Cards::makeCard(Bastoni, Re),
    };
    auto state = makeState(hand);
    state.moveConstraints.colorToPlay = NoColor; // no constraint

    CardSet legal;
    rules().calculateLegalMoves(state, legal);
    EXPECT_EQ(legal.size(), hand.size());
}
