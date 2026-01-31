#include "TressetteRound.h"
#include "TressetteRules.h"
#include "Logger.h"
#include "Cards.h"

TressetteRound_NS::TressetteRound::TressetteRound(const TressetteRoundState& _state, int _numPlayers, const EventEmitter& _eventEmitter) :
    CardsRound_NS::CardsRound(TressetteRules::instance(), _state, _numPlayers, _numPlayers, _eventEmitter)
{}

TressetteRound_NS::TressetteRoundState::TressetteRoundState(fullPlayerId _nextToPlayId, const CardsRound_NS::Players& _players) :
    CardsRound_NS::RoundState(_nextToPlayId, _players)
{
    LOG_DEBUG("TressetteRoundState ctor");
}

void TressetteRound_NS::TressetteRound::preMoveSetup()
{
    LOG_DEBUG("Tressette before move");
}

void TressetteRound_NS::TressetteRound::postMoveSetup(const Move& move)
{
    CardsRound_NS::CardsRound::postMoveSetup(move);
    if(roundState.playedMovesInRound.size() == 1)
        roundState.moveConstraints.colorToPlay = Cards::getColor(move.card);
    if(move.call == ConQuestaBasta)
        bastaCalled = move.playerId;
}

bool TressetteRound_NS::TressetteRound::IsFinished()
{
    if(CardsRound_NS::CardsRound::IsFinished())
        return true;

    return false;
}