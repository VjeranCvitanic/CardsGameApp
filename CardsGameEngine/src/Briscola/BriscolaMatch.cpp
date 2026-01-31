#include "BriscolaMatch.h"
#include "BriscolaGame.h"
#include "CardsRound.h"
#include "Logger.h"

BriscolaMatch_NS::BriscolaMatch::BriscolaMatch(const CardsMatch_NS::MatchState& matchState, int _numPlayers, const EventEmitter& _eventEmitter) :
    CardsMatch_NS::CardsMatch(matchState, _numPlayers, _eventEmitter)
{
    LOG_DEBUG("ctor");
    matchResult.score = {
    { 0, {} },
    { 1, {} }
};
    InitMatch();
    startNewGame();
}

void BriscolaMatch_NS::BriscolaMatch::startNewGame()
{
    CardsRound_NS::Players gamePlayers = {};
    for(auto& p : matchState.players)
    {
        gamePlayers.push_back({p, {}});
    }

    BriscolaGame_NS::BriscolaGameState gameState(matchState.nextToStartId, gamePlayers);
    currGame = std::make_unique<BriscolaGame_NS::BriscolaGame>(
        gameState,
        numPlayers,
        eventEmitter
    );
}

void BriscolaMatch_NS::BriscolaMatch::updateMatchResult()
{
    matchResult.score[currGame->gameResult.winnerId].wonGames++;
}

bool BriscolaMatch_NS::BriscolaMatch::IsFinished()
{
    for(auto& score : matchResult.score)
    {
        if(score.second.wonGames >= 4)
        {
            matchResult.winnerId = score.first;
            return true;
        }
    }

    return false;
}

void BriscolaMatch_NS::BriscolaMatch::InitMatch()
{
    LOG_INFO("Start match");
    eventEmitter.emit(StartMatchEvent(matchState.nextToStartId, BriscolaGame));
}
