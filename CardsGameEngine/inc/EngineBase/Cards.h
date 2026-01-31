#pragma once

#include "Types.h"
#include <string>

namespace Cards
{
    Card makeCard(Color color, Number number);

    Color getColor(Card);
    Number getNumber(Card);

    std::string CardToString(Card card);
    std::string ColorToString(Color color);
    std::string NumberToString(Number number);
    Number intToNumber(int8_t number);

    void logCard(Card card);
    void logCards(CardSet cards);

    bool isCardInCardSet(CardSet cards, Card card);

    void getCardsFromMoves(const Moves& moves, CardSet& cards);
}
