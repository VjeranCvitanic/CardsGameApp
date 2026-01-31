#include "Cards.h"
#include "Logger.h"

bool Cards::isCardInCardSet(CardSet cards, Card card)
{
    return std::find(cards.begin(), cards.end(), card) == cards.end() ? false : true;
}

Card Cards::makeCard(Color color, Number number)
{
    return std::make_tuple(color, number);
}

void Cards::logCard(Card card)
{
    LOG_DEBUG(CardToString(card));
}

void Cards::logCards(CardSet cards)
{
    LOG_DEBUG("Cards:");
    for(int i = cards.size()-1; i >= 0; i--)
    {
        Cards::logCard(cards[i]);
    }
}

Number Cards::intToNumber(int8_t number)
{
    if(number >= Asso && number <= Sette)
        return static_cast<Number>(number);
    if(number >= 11 && number <= 13)
        return static_cast<Number>(number - 3);
    return InvalidNumber;
}

Color Cards::getColor(Card card)
{
    return std::get<0>(card);
}
Number Cards::getNumber(Card card)
{
    return std::get<1>(card);
}

std::string Cards::CardToString(Card card)
{
    std::string colorStr;
    std::string numberStr;

    colorStr = ColorToString(getColor(card));
    numberStr = NumberToString(getNumber(card));

    return numberStr + " de " + colorStr;
}

std::string Cards::ColorToString(Color color)
{
    switch(color)
    {
        case Spade:
            return "Spade";
        case Coppe:
            return "Coppe";
        case Denari:
            return "Denari";
        case Bastoni:
            return "Bastoni";
        case NoColor:
            return "NoColor";
        default:
            LOG_WARNING("Color: ", color);
            return "InvalidColor";
    }
}
std::string Cards::NumberToString(Number number)
{
    switch(number)
    {
        case Asso:
            return "Asso";
        case Due:
            return "Due";
        case Tre:
            return "Tre";
        case Quattro:
            return "Quattro";
        case Cinque:
            return "Cinque";
        case Sei:
            return "Sei";
        case Sette:
            return "Sette";
        case Fante:
            return "Fante";
        case Cavallo:
            return "Cavallo";
        case Re:
            return "Re";
        default:
            LOG_WARNING("Number: ", number);
            return "InvalidNumber";
    }
}

void Cards::getCardsFromMoves(const Moves& moves, CardSet& cards)
{
    for(auto& m : moves)
    {
        cards.push_back(m.card);
    }
}