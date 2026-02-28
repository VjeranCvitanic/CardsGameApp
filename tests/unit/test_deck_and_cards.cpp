/**
 * Unit tests – Deck and Cards utilities
 *
 * Tests deck creation, card manipulation, and Cards utility functions
 * in complete isolation from game logic.
 *
 * Framework: GoogleTest
 */

#include <gtest/gtest.h>
#include "Deck.h"
#include "Cards.h"
#include "Types.h"
#include <algorithm>
#include <set>

// ============================================================
// Cards utility functions
// ============================================================

TEST(Cards_MakeCard, ColorAndNumberRoundTrip)
{
    for (int c = Spade; c < NoColor; ++c) {
        for (int n = Asso; n < InvalidNumber; ++n) {
            Card card = Cards::makeCard(static_cast<Color>(c), static_cast<Number>(n));
            EXPECT_EQ(Cards::getColor(card),  static_cast<Color>(c));
            EXPECT_EQ(Cards::getNumber(card), static_cast<Number>(n));
        }
    }
}

TEST(Cards_IsCardInCardSet, Found)
{
    Card a = Cards::makeCard(Spade, Asso);
    Card b = Cards::makeCard(Coppe, Tre);
    CardSet s = {a, b};
    EXPECT_TRUE(Cards::isCardInCardSet(s, a));
    EXPECT_TRUE(Cards::isCardInCardSet(s, b));
}

TEST(Cards_IsCardInCardSet, NotFound)
{
    CardSet s = { Cards::makeCard(Spade, Asso) };
    EXPECT_FALSE(Cards::isCardInCardSet(s, Cards::makeCard(Coppe, Asso)));
    EXPECT_FALSE(Cards::isCardInCardSet(s, Cards::makeCard(Spade, Due)));
}

TEST(Cards_IsCardInCardSet, EmptySet)
{
    EXPECT_FALSE(Cards::isCardInCardSet({}, Cards::makeCard(Spade, Asso)));
}

TEST(Cards_GetCardsFromMoves, ExtractsCards)
{
    Move m1; m1.card = Cards::makeCard(Spade, Asso);   m1.call = NoCall;
    Move m2; m2.card = Cards::makeCard(Coppe, Tre);    m2.call = Busso;
    Moves moves = {m1, m2};

    CardSet out;
    Cards::getCardsFromMoves(moves, out);

    ASSERT_EQ(out.size(), 2u);
    EXPECT_TRUE(Cards::isCardInCardSet(out, m1.card));
    EXPECT_TRUE(Cards::isCardInCardSet(out, m2.card));
}

TEST(Cards_GetCardsFromMoves, EmptyMoves)
{
    CardSet out;
    Cards::getCardsFromMoves({}, out);
    EXPECT_TRUE(out.empty());
}

// ============================================================
// Deck – empty construction
// ============================================================

TEST(Deck_Construction, DefaultIsEmpty)
{
    Deck d;
    EXPECT_TRUE(d.getDeck().empty());
}

TEST(Deck_Construction, BoolFalseIsEmpty)
{
    Deck d(false);
    EXPECT_TRUE(d.getDeck().empty());
}

// ============================================================
// Deck – full deck creation
// ============================================================

TEST(Deck_FullDeck, HasExactly40Cards)
{
    Deck d(true);
    EXPECT_EQ(d.getDeck().size(), 40u);
}

TEST(Deck_FullDeck, AllCardsUnique)
{
    Deck d(true);
    CardSet cards = d.getDeck();
    std::set<Card> unique(cards.begin(), cards.end());
    EXPECT_EQ(unique.size(), 40u) << "Deck contains duplicate cards";
}

TEST(Deck_FullDeck, ContainsAllColorsAndNumbers)
{
    Deck d(true);
    CardSet cards = d.getDeck();

    for (int c = Spade; c < NoColor; ++c) {
        for (int n = Asso; n < InvalidNumber; ++n) {
            Card expected = Cards::makeCard(static_cast<Color>(c), static_cast<Number>(n));
            EXPECT_TRUE(Cards::isCardInCardSet(cards, expected))
                << "Missing: color=" << c << " number=" << n;
        }
    }
}

TEST(Deck_FullDeck, HasFourSuits)
{
    Deck d(true);
    std::set<Color> suits;
    for (const Card& c : d.getDeck())
        suits.insert(Cards::getColor(c));
    EXPECT_EQ(suits.size(), 4u);
    EXPECT_TRUE(suits.count(Spade));
    EXPECT_TRUE(suits.count(Coppe));
    EXPECT_TRUE(suits.count(Denari));
    EXPECT_TRUE(suits.count(Bastoni));
}

TEST(Deck_FullDeck, TenCardsPerSuit)
{
    Deck d(true);
    std::map<Color, int> counts;
    for (const Card& c : d.getDeck())
        counts[Cards::getColor(c)]++;
    for (int col = Spade; col < NoColor; ++col)
        EXPECT_EQ(counts[static_cast<Color>(col)], 10);
}

// ============================================================
// Deck – add / erase / pop
// ============================================================

TEST(Deck_AddCard, SizeIncrements)
{
    Deck d;
    EXPECT_EQ(d.getDeck().size(), 0u);
    d.AddCard(Cards::makeCard(Spade, Asso));
    EXPECT_EQ(d.getDeck().size(), 1u);
    d.AddCard(Cards::makeCard(Coppe, Tre));
    EXPECT_EQ(d.getDeck().size(), 2u);
}

TEST(Deck_AddCard, CardIsInDeckAfterAdd)
{
    Deck d;
    Card c = Cards::makeCard(Denari, Re);
    d.AddCard(c);
    EXPECT_TRUE(d.isCardInDeck(c));
}

TEST(Deck_EraseCard, CardRemovedAfterErase)
{
    Deck d;
    Card c = Cards::makeCard(Bastoni, Sette);
    d.AddCard(c);
    ASSERT_TRUE(d.isCardInDeck(c));
    d.eraseCard(c);
    EXPECT_FALSE(d.isCardInDeck(c));
    EXPECT_TRUE(d.getDeck().empty());
}

TEST(Deck_EraseCard, EraseNonExistentIsNoop)
{
    Deck d;
    d.AddCard(Cards::makeCard(Spade, Asso));
    d.eraseCard(Cards::makeCard(Coppe, Tre)); // not in deck
    EXPECT_EQ(d.getDeck().size(), 1u);        // unchanged
}

TEST(Deck_EraseDeck, DeckIsEmptyAfterErase)
{
    Deck d(true);
    ASSERT_EQ(d.getDeck().size(), 40u);
    d.eraseDeck();
    EXPECT_TRUE(d.getDeck().empty());
}

TEST(Deck_PopCard, SizeDecrementsAndCardReturned)
{
    Deck d;
    Card c1 = Cards::makeCard(Spade, Asso);
    Card c2 = Cards::makeCard(Coppe, Tre);
    d.AddCard(c1);
    d.AddCard(c2);

    Card popped = d.popCard();
    EXPECT_EQ(d.getDeck().size(), 1u);
    // popCard returns the back element
    EXPECT_EQ(popped, c2);
}

TEST(Deck_PopCard, EmptyDeckReturnsInvalidCard)
{
    Deck d;
    Card result = d.popCard();
    EXPECT_EQ(Cards::getColor(result),  InvalidColor);
    EXPECT_EQ(Cards::getNumber(result), InvalidNumber);
}

// ============================================================
// Deck – copy semantics (players share no state)
// ============================================================

TEST(Deck_Copy, CopyIsIndependent)
{
    Deck d1;
    d1.AddCard(Cards::makeCard(Spade, Asso));

    Deck d2 = d1;          // copy
    d2.AddCard(Cards::makeCard(Coppe, Tre));

    // d1 is unchanged
    EXPECT_EQ(d1.getDeck().size(), 1u);
    // d2 has both
    EXPECT_EQ(d2.getDeck().size(), 2u);
}

// ============================================================
// Deck – dealing pattern (interleaved round-robin)
// Verifies the dealing logic used by CardsGame::dealCardsImpl:
// N cards dealt round-robin starting from nextToPlayId
// ============================================================

TEST(Deck_DealPattern, FullDeckPopOrderIsLIFO)
{
    // Deck::popCard() pops from the back → LIFO order
    Deck d;
    Card c1 = Cards::makeCard(Spade, Asso);
    Card c2 = Cards::makeCard(Spade, Due);
    Card c3 = Cards::makeCard(Spade, Tre);
    d.AddCard(c1); d.AddCard(c2); d.AddCard(c3);

    EXPECT_EQ(d.popCard(), c3);
    EXPECT_EQ(d.popCard(), c2);
    EXPECT_EQ(d.popCard(), c1);
    EXPECT_TRUE(d.getDeck().empty());
}

TEST(Deck_FullDeck, ShuffledVariants_AreStillComplete)
{
    // Two full decks should have the same set of unique cards
    // (randomness in CreateDeck shouldn't drop or duplicate cards)
    Deck d1(true);
    Deck d2(true);

    std::set<Card> s1(d1.getDeck().begin(), d1.getDeck().end());
    std::set<Card> s2(d2.getDeck().begin(), d2.getDeck().end());
    EXPECT_EQ(s1, s2) << "Two full decks contain different card sets";
}
