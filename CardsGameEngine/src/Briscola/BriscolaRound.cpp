#include "BriscolaRound.h"
#include "BriscolaRules.h"
#include "Logger.h"

BriscolaRound_NS::BriscolaRound::BriscolaRound(const BriscolaRoundState& _state, int _handSize, int _numPlayers, const EventEmitter& _eventEmitter) :
    CardsRound_NS::CardsRound(BriscolaRules::instance(), _state, _handSize, _numPlayers, _eventEmitter)
{}

BriscolaRound_NS::BriscolaRoundState::BriscolaRoundState(Color _strongColor, fullPlayerId _nextToPlayId, const CardsRound_NS::Players& _players) :
    CardsRound_NS::RoundState(_nextToPlayId, _players)
{
    strongColor = _strongColor;
    LOG_DEBUG("BriscolaRoundState ctor");
}

void BriscolaRound_NS::BriscolaRound::preMoveSetup()
{
    LOG_DEBUG("Briscola before move");
}