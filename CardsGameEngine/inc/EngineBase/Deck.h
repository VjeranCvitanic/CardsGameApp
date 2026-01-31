#pragma once

#include "Types.h"
#include <vector>

class CardsGame;

#define DECK_SIZE 40

class Deck
{
public:
    Deck();
    Deck(bool full);
    Deck(int numCards);

    Deck(const Deck& other) : cards(other.cards) {}
    Deck& operator=(const Deck& other) { cards = other.cards; return *this; }

    //getters
    Card getCard(int8_t pos);
    CardSet getDeck() const;
    bool isCardInDeck(Card card);

    //setters
    void AddCard(Card card);
    void eraseCard(Card card);
    void eraseDeck();
    Card popCard();

    // print & log
    void logDeck();

private:
    CardSet cards;

    int8_t findFreeSlot(std::vector<int> flags);
    void CreateDeck();
};

