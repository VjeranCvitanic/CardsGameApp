#include "BriscolaRules.h"
#include "Logger.h"
#include "Cards.h"

bool BriscolaRules::IsMoveLegal(const Move& move, const CardsRound_NS::RoundState& state, MoveReturnValue& reason) const
{
    if(move.playerId != state.nextToPlayId)
    {
        LOG_DEBUG("Not your turn (id: ", move.playerId.first, ") (to play: ", state.nextToPlayId, ")");
        reason = NotYourTurn;
        return false;
    }
    // if card in cardset
    PlayerId playerId = move.playerId.second;
    if(Cards::isCardInCardSet(state.players[playerId].deck.getDeck(), move.card) != true)
    {
        LOG_DEBUG("Card not in players hand (id: ", move.playerId, ")");
        Cards::logCards(state.players[playerId].deck.getDeck());
        reason = CardNotInHand;
        return false;
    }
    return true;
}

Card BriscolaRules::StrongerCard(const Card& card1, const Card& card2, Color strongColor) const
{
    Color color1 = Cards::getColor(card1);
    Color color2 = Cards::getColor(card2);
    Number number1 = Cards::getNumber(card1);
    Number number2 = Cards::getNumber(card2);

    if(color1 == color2)
    {
        if(getNumberStrength(number1) > getNumberStrength(number2))
        {
            return card1;
        }
        else
        {
            return card2;
        }
    }
    else
    {
        if(color2 == strongColor)
        {
            return card2;
        }
        else if(color1 == strongColor)
        {
            return card1;
        }
        else
        {
            return card1; // first card played wins
        }
    } 
}

int8_t BriscolaRules::getNumberStrength(Number number) const
{
    switch (number) {
        case Asso:     return 10;
        case Tre:      return 9;
        case Re:       return 8;
        case Cavallo:  return 7;
        case Fante:    return 6;
        case Sette:    return 5;
        case Sei:      return 4;
        case Cinque:   return 3;
        case Quattro:  return 2;
        case Due:      return 1;
        default:      return 0;
    }
}

Points BriscolaRules::getNumberValue(Number number) const
{
    switch (number) {
        case Asso:     return Points(11, 0);
        case Tre:      return Points(10, 0);
        case Re:       return Points(4, 0);
        case Cavallo:  return Points(3, 0);
        case Fante:    return Points(2, 0);
        default:      return Points(0, 0);
    }
}