/**
 * Unit tests – TressetteRules
 *
 * Tressette strength order (weakest → strongest):
 *   Quattro(1) < Cinque(2) < Sei(3) < Sette(4) < Fante(5) < Cavallo(6)
 *   < Re(7) < Asso(8) < Due(9) < Tre(10)
 *
 * Point values (bella system):
 *   Asso = 1 punto, Tre/Due/Re/Cavallo/Fante = 1 bella each, others = 0
 *
 * Key rule: must follow lead suit if able (suit-following constraint).
 * Calls (Busso/Striscio/ConQuestaBasta) only allowed on first card of trick.
 * No trump/briscola suit – StrongerCard only compares same-suit cards.
 */

#include <gtest/gtest.h>
#include "Cards.h"
#include "TressetteRules.h"
#include "Types.h"

// ============================================================
// Helpers
// ============================================================

static TressetteRules& rules() { return TressetteRules::instance(); }

static CardsRound_NS::RoundState makeState(
    CardSet       hand,
    Moves         movesPlayed    = {},
    Color         colorToPlay    = NoColor,
    fullPlayerId  nextToPlay     = {0, 0})
{
    CardsRound_NS::PlayerState ps;
    ps.playerId = {0, 0};
    ps.deck     = Deck();  // Empty deck
    for (const Card& c : hand)
        ps.deck.AddCard(c);

    CardsRound_NS::RoundState state(nextToPlay, {ps});
    state.playedMovesInRound           = movesPlayed;
    state.moveConstraints.colorToPlay  = colorToPlay;
    state.strongColor                  = NoColor; // Tressette has no trump
    return state;
}

static Move makeMove(Card card, Call call, fullPlayerId pid = {0, 0})
{
    Move m; m.card = card; m.call = call; m.playerId = pid;
    return m;
}

// ============================================================
// Strength order
// ============================================================

TEST(TressetteRules_Strength, TreIsStrongest)
{
    for (auto n : {Asso, Due, Re, Cavallo, Fante, Sette, Sei, Cinque, Quattro})
        EXPECT_GT(rules().getNumberStrength(Tre), rules().getNumberStrength(n));
}

TEST(TressetteRules_Strength, DueIsSecondStrongest)
{
    EXPECT_GT(rules().getNumberStrength(Due), rules().getNumberStrength(Asso));
    EXPECT_GT(rules().getNumberStrength(Due), rules().getNumberStrength(Re));
    EXPECT_LT(rules().getNumberStrength(Due), rules().getNumberStrength(Tre));
}

TEST(TressetteRules_Strength, AssoIsThirdStrongest)
{
    EXPECT_GT(rules().getNumberStrength(Asso), rules().getNumberStrength(Re));
    EXPECT_LT(rules().getNumberStrength(Asso), rules().getNumberStrength(Due));
    EXPECT_LT(rules().getNumberStrength(Asso), rules().getNumberStrength(Tre));
}

TEST(TressetteRules_Strength, QuattroIsWeakest)
{
    for (auto n : {Asso, Due, Tre, Re, Cavallo, Fante, Sette, Sei, Cinque})
        EXPECT_GT(rules().getNumberStrength(n), rules().getNumberStrength(Quattro));
}

TEST(TressetteRules_Strength, FullOrderIsMonotonic)
{
    // Verify the complete strict ordering
    const Number order[] = {Quattro, Cinque, Sei, Sette, Fante, Cavallo, Re, Asso, Due, Tre};
    for (int i = 0; i < 9; ++i)
        EXPECT_LT(rules().getNumberStrength(order[i]),
                  rules().getNumberStrength(order[i + 1]))
            << "Failed between index " << i << " and " << (i+1);
}

// ============================================================
// Point values (bella system)
// Asso = 1 punto (punta=1), Tre/Due/Re/Cavallo/Fante = 1 bella (bella=1)
// ============================================================

TEST(TressetteRules_Points, AssoWorth1Punto)
{
    EXPECT_EQ(rules().getNumberValue(Asso).punta, 1);
    EXPECT_EQ(rules().getNumberValue(Asso).bella, 0);
}

TEST(TressetteRules_Points, TreWorth1Bella)
{
    EXPECT_EQ(rules().getNumberValue(Tre).punta, 0);
    EXPECT_EQ(rules().getNumberValue(Tre).bella, 1);
}

TEST(TressetteRules_Points, DueWorth1Bella)
{
    EXPECT_EQ(rules().getNumberValue(Due).punta, 0);
    EXPECT_EQ(rules().getNumberValue(Due).bella, 1);
}

TEST(TressetteRules_Points, ReWorth1Bella)
{
    EXPECT_EQ(rules().getNumberValue(Re).punta, 0);
    EXPECT_EQ(rules().getNumberValue(Re).bella, 1);
}

TEST(TressetteRules_Points, CavalloWorth1Bella)
{
    EXPECT_EQ(rules().getNumberValue(Cavallo).punta, 0);
    EXPECT_EQ(rules().getNumberValue(Cavallo).bella, 1);
}

TEST(TressetteRules_Points, FanteWorth1Bella)
{
    EXPECT_EQ(rules().getNumberValue(Fante).punta, 0);
    EXPECT_EQ(rules().getNumberValue(Fante).bella, 1);
}

TEST(TressetteRules_Points, NumeralsWorthNothing)
{
    for (auto n : {Quattro, Cinque, Sei, Sette}) {
        EXPECT_EQ(rules().getNumberValue(n).punta, 0) << "punta for number " << n;
        EXPECT_EQ(rules().getNumberValue(n).bella, 0) << "bella for number " << n;
    }
}

// ============================================================
// StrongerCard – Tressette has no trump; only same-suit matters
// ============================================================

TEST(TressetteRules_StrongerCard, TreBeatsAsso_SameSuit)
{
    Card tre  = Cards::makeCard(Spade, Tre);
    Card asso = Cards::makeCard(Spade, Asso);
    EXPECT_EQ(rules().StrongerCard(tre, asso, NoColor), tre);
    EXPECT_EQ(rules().StrongerCard(asso, tre, NoColor), tre);
}

TEST(TressetteRules_StrongerCard, FirstCardWins_DifferentSuits)
{
    // In Tressette, off-suit never beats lead suit – first card always "wins"
    Card leadSpadeAce  = Cards::makeCard(Spade, Asso);
    Card offCoppeTre   = Cards::makeCard(Coppe, Tre); // strongest number but wrong suit
    EXPECT_EQ(rules().StrongerCard(leadSpadeAce, offCoppeTre, NoColor), leadSpadeAce);
    EXPECT_EQ(rules().StrongerCard(offCoppeTre, leadSpadeAce, NoColor), offCoppeTre);
}

TEST(TressetteRules_StrongerCard, DueBeatsRe_SameSuit)
{
    Card due = Cards::makeCard(Bastoni, Due);
    Card re  = Cards::makeCard(Bastoni, Re);
    EXPECT_EQ(rules().StrongerCard(due, re, NoColor), due);
}

// ============================================================
// IsMoveLegal – suit-following constraint
// ============================================================

TEST(TressetteRules_IsMoveLegal, NoConstraint_AnyCardIsLegal)
{
    CardSet hand = {
        Cards::makeCard(Spade, Asso),
        Cards::makeCard(Coppe, Tre),
        Cards::makeCard(Denari, Re),
    };
    auto state = makeState(hand, {}, NoColor);

    for (const Card& c : hand) {
        MoveReturnValue reason = Ok;
        EXPECT_TRUE(rules().IsMoveLegal(makeMove(c, NoCall), state, reason));
        EXPECT_EQ(reason, Ok);
    }
}

TEST(TressetteRules_IsMoveLegal, MustFollowSuit_WhenAble)
{
    // Lead color = Spade. Player has Spade and Coppe. Must play Spade.
    CardSet hand = {
        Cards::makeCard(Spade, Sette),
        Cards::makeCard(Coppe, Asso),
    };
    auto state = makeState(hand, {}, Spade);

    // Playing Spade → legal
    {
        MoveReturnValue reason = Ok;
        EXPECT_TRUE(rules().IsMoveLegal(
            makeMove(Cards::makeCard(Spade, Sette), NoCall), state, reason));
        EXPECT_EQ(reason, Ok);
    }

    // Playing Coppe when Spade is available → illegal
    {
        MoveReturnValue reason = Ok;
        EXPECT_FALSE(rules().IsMoveLegal(
            makeMove(Cards::makeCard(Coppe, Asso), NoCall), state, reason));
        EXPECT_EQ(reason, ColorConstraintNotMet);
    }
}

TEST(TressetteRules_IsMoveLegal, CanPlayAnySuit_WhenVoidInLeadSuit)
{
    // Lead color = Spade. Player has NO Spade. Any card is legal.
    CardSet hand = {
        Cards::makeCard(Coppe, Asso),
        Cards::makeCard(Denari, Tre),
    };
    auto state = makeState(hand, {}, Spade);

    for (const Card& c : hand) {
        MoveReturnValue reason = Ok;
        EXPECT_TRUE(rules().IsMoveLegal(makeMove(c, NoCall), state, reason));
        EXPECT_EQ(reason, Ok);
    }
}

TEST(TressetteRules_IsMoveLegal, CardNotInHand_Rejected)
{
    CardSet hand = { Cards::makeCard(Coppe, Asso) };
    auto state   = makeState(hand, {}, NoColor);

    MoveReturnValue reason = Ok;
    EXPECT_FALSE(rules().IsMoveLegal(
        makeMove(Cards::makeCard(Spade, Re), NoCall), state, reason));
    EXPECT_EQ(reason, CardNotInHand);
}

TEST(TressetteRules_IsMoveLegal, Call_OnlyLegalOnFirstCardOfTrick)
{
    CardSet hand = { Cards::makeCard(Spade, Asso) };
    auto state   = makeState(hand, {}, NoColor);

    // First card of trick → call is legal
    {
        MoveReturnValue reason = Ok;
        EXPECT_TRUE(rules().IsMoveLegal(
            makeMove(Cards::makeCard(Spade, Asso), Busso), state, reason));
    }

    // Later in the trick (movesPlayed not empty) → call is illegal
    Move played; played.card = Cards::makeCard(Coppe, Tre); played.call = NoCall;
    played.playerId = {1, 1};
    auto stateWithMoves = makeState(hand, {played}, Coppe);

    {
        MoveReturnValue reason = Ok;
        EXPECT_FALSE(rules().IsMoveLegal(
            makeMove(Cards::makeCard(Spade, Asso), Busso), stateWithMoves, reason));
        EXPECT_EQ(reason, CantCallIfNotFirstOfHand);
    }
}

// ============================================================
// calculateLegalMoves – suit-following filter
// ============================================================

TEST(TressetteRules_LegalMoves, NoConstraint_FullHand)
{
    CardSet hand = {
        Cards::makeCard(Spade, Asso),
        Cards::makeCard(Coppe, Tre),
        Cards::makeCard(Denari, Re),
    };
    auto state = makeState(hand, {}, NoColor);

    CardSet legal;
    rules().calculateLegalMoves(state, legal);
    EXPECT_EQ(legal.size(), hand.size());
}

TEST(TressetteRules_LegalMoves, Constraint_OnlyMatchingSuit)
{
    CardSet hand = {
        Cards::makeCard(Spade, Asso),
        Cards::makeCard(Spade, Sette),
        Cards::makeCard(Coppe, Tre),
    };
    auto state = makeState(hand, {}, Spade);

    CardSet legal;
    rules().calculateLegalMoves(state, legal);
    ASSERT_EQ(legal.size(), 2u);
    for (const Card& c : legal)
        EXPECT_EQ(Cards::getColor(c), Spade);
}

TEST(TressetteRules_LegalMoves, VoidInLeadSuit_FullHandLegal)
{
    // Player has no Spade → falls back to full hand
    CardSet hand = {
        Cards::makeCard(Coppe, Asso),
        Cards::makeCard(Denari, Tre),
    };
    auto state = makeState(hand, {}, Spade);

    CardSet legal;
    rules().calculateLegalMoves(state, legal);
    EXPECT_EQ(legal.size(), hand.size());
}
