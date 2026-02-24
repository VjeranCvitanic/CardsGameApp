#include "Deck.h"
#include "Cards.h"
#include "Logger.h"
#include <vector>

Deck::Deck()
{
    LOG_DEBUG("ctor");
    cards = {};
}

Deck::Deck(bool full)
{
    LOG_DEBUG("ctor 2");
    if(full)
    {
        CreateDeck();
        logDeck();
    }
    else {
        cards = {};
    }
}

Deck::Deck(int numCards)
{
    LOG_DEBUG("ctor 3");
    for(int i = 0; i < numCards; i++)
    {
        cards.push_back(Cards::makeCard(static_cast<Color>(rand() % 4), static_cast<Number>(rand() % 10 + 1)));
    }
}

CardSet Deck::getDeck() const
{
    return cards;
}

Card Deck::getCard(int8_t pos)
{
    return cards[pos];
}

int8_t Deck::findFreeSlot(std::vector<int> flags)
{
    int randNum = rand() % DECK_SIZE;

    while(flags[randNum] != 0)
    {
        randNum = (randNum + 1) % DECK_SIZE;
    }

    return randNum;
}

void Deck::CreateDeck()
{
    LOG_DEBUG("Creating deck");
    cards.resize(DECK_SIZE);
    std::vector<int> flags(DECK_SIZE, 0);
    for (int c = Spade; c < NoColor; c++)
    {
        for (int n = Asso; n < InvalidNumber; n++)
        {
            int pos = findFreeSlot(flags);
            flags[pos] = 1;
            cards[pos] = std::make_tuple(static_cast<Color>(c), static_cast<Number>(n));
        }
    }
}

bool Deck::isCardInDeck(Card card)
{
    return Cards::isCardInCardSet(cards, card);
}

void Deck::logDeck()
{
    Cards::logCards(cards);
}

Card Deck::popCard()
{
    if(cards.empty())
    {
        LOG_ERROR("Deck is empty!");
        return Cards::makeCard(InvalidColor, InvalidNumber);
    }

    Card topCard = cards.back();
    cards.pop_back();
    return topCard;
}

void Deck::eraseCard(Card card)
{
    auto rem = std::remove(cards.begin(), cards.end(), card);
    cards.erase(rem, cards.end());
}

void Deck::eraseDeck()
{
    cards.clear();
}

void Deck::AddCard(Card card)
{
    cards.push_back(card);
}



