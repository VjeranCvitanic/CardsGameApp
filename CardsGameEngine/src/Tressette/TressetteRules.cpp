#include "TressetteRules.h"
#include "Logger.h"
#include "Cards.h"

bool TressetteRules::IsMoveLegal(const Move& move, const CardsRound_NS::RoundState& state, MoveReturnValue& reason) const
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
    if(checkConstraints(state.players[playerId].deck.getDeck(), move.card, state.moveConstraints.colorToPlay) == false)
    {
        LOG_INFO("Color constraint not met: played card: ");
        Cards::logCard(move.card);
        LOG_DEBUG("Color to play: ", state.moveConstraints.colorToPlay);
        LOG_DEBUG("Current hand: ");
        Cards::logCards(state.players[playerId].deck.getDeck());
        reason = ColorConstraintNotMet;
        return false;
    }

    if(state.playedMovesInRound.size() != 0 && move.call != NoCall)
    {
        LOG_INFO("Cant call if not first of trick!");
        reason = CantCallIfNotFirstOfHand;
        return false;
    }

    reason = Ok;
    return true;
}

void TressetteRules::calculateLegalMoves(const CardsRound_NS::RoundState& state, CardSet& legalCards) const
{
    if(state.moveConstraints.colorToPlay != NoColor)
    {
        for(auto& card : state.players[state.nextToPlayId.second].deck.getDeck())
        {
            if(Cards::getColor(card) == state.moveConstraints.colorToPlay)
            {
                legalCards.push_back(card);
            }
        }
    }
    else
        legalCards = state.players[state.nextToPlayId.second].deck.getDeck();

    if(legalCards.size() == 0)
    {
        legalCards = state.players[state.nextToPlayId.second].deck.getDeck();
    }
}

bool TressetteRules::checkConstraints(const CardSet& hand, Card card, Color leadColor) const
{
    if(leadColor != NoColor && Cards::getColor(card) != leadColor)
    {
        for(auto& card : hand)
        {
            if(Cards::getColor(card) == leadColor)
            {
                return false;
            }
        }
    }

    return true;
}


Card TressetteRules::StrongerCard(const Card& card1, const Card& card2, Color strongColor) const
{
    if(Cards::getColor(card1) == Cards::getColor(card2))
    {
        if(getNumberStrength(Cards::getNumber(card1)) > getNumberStrength(Cards::getNumber(card2)))
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
        return card1; // first card played wins
    }
}

int8_t TressetteRules::getNumberStrength(Number number) const
{
    switch (number) {
        case Tre:     return 10;
        case Due:      return 9;
        case Asso:       return 8;
        case Re:  return 7;
        case Cavallo:    return 6;
        case Fante:    return 5;
        case Sette:      return 4;
        case Sei:   return 3;
        case Cinque:  return 2;
        case Quattro:      return 1;
        default:      return 0;
    }
}

Points TressetteRules::getNumberValue(Number number) const
{
    switch (number) {
        case Asso:     return Points(1, 0);
        case Tre:      
        case Due:      
        case Re:       
        case Cavallo:  
        case Fante:   return Points(0, 1);
        default:      return Points(0, 0);
    }
}