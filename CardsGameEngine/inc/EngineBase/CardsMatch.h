#pragma once

#include "CardsGame.h"
#include "EventEmitter.h"
#include "Types.h"
#include "MatchResult.h"

namespace CardsMatch_NS
{
    typedef std::vector<fullPlayerId> Players;

    struct MatchState
    {
        MatchState(fullPlayerId _nextToStartId, const Players& _players);

        Players players;
        fullPlayerId nextToStartId;
        int gameCnt = 0;
    };

    class CardsMatch
    {
    public:
        CardsMatch(const MatchState& matchState, int numPlayers, const EventEmitter& eventEmitter);
        virtual bool IsFinished() = 0;
        virtual void startNewGame() = 0;
        MoveReturnValue ApplyMove(const Move&);
        MatchState matchState;
        MatchResult matchResult;
    protected:
        const EventEmitter& eventEmitter;
        std::unique_ptr<CardsGame_NS::CardsGame> currGame;
        int numPlayers;

        virtual void InitMatch() = 0;
        virtual void updateMatchResult() = 0;
        void EndMatch();
        MoveReturnValue PostMove(MoveReturnValue gameRetVal);
    };
}
