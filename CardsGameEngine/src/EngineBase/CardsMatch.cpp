#include "CardsMatch.h"
#include "Logger.h"
#include "Types.h"

CardsMatch_NS::CardsMatch::CardsMatch(const CardsMatch_NS::MatchState& _matchState, int _numPlayers, const EventEmitter& _eventEmitter) :
    eventEmitter(_eventEmitter), currGame(), numPlayers(_numPlayers), matchState(_matchState), matchResult()
{
    LOG_DEBUG("ctor");
}

CardsMatch_NS::MatchState::MatchState(fullPlayerId _nextToStartId, const Players& _players) :
    players(_players), nextToStartId(_nextToStartId)
{
    LOG_DEBUG("ctor");
}

MoveReturnValue CardsMatch_NS::CardsMatch::ApplyMove(const Move& move)
{
    MoveReturnValue gameRetVal = currGame->ApplyMove(move);
    return PostMove(gameRetVal);
}

MoveReturnValue CardsMatch_NS::CardsMatch::PostMove(MoveReturnValue gameRetVal)
{
    if(gameRetVal == Finish)
    {
        updateMatchResult();
        LOG_INFO("Match result so far: ");
        for(auto& p : matchState.players)
        {
            LOG_INFO(matchResult.score.at(p.first).wonGames);
        }
        matchState.gameCnt++;
        PlayerId playerId = (matchState.nextToStartId.second + 1) % numPlayers;
        matchState.nextToStartId = {playerId%2, playerId};

        if(IsFinished())
        {
            LOG_INFO("Match finished");
            EndMatch();
            return Finish;
        }
        startNewGame();

        return Ok;
    }

    return gameRetVal;
}

void CardsMatch_NS::CardsMatch::EndMatch()
{
    LOG_INFO("End match");
    eventEmitter.emit(MatchOverEvent(std::move(matchResult)));
}

