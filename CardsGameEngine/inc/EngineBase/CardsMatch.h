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
        virtual ~CardsMatch() = default;
        MoveReturnValue ApplyMove(const Move&);

        MatchResult GetMatchResult() const { return matchResult; }

    protected:
        const EventEmitter& eventEmitter;
        std::unique_ptr<CardsGame_NS::CardsGame> currGame;
        int numPlayers;
        MatchState matchState;
        MatchResult matchResult;

        virtual void InitMatch() = 0;
        void EndMatch();

        virtual void updateMatchResult() = 0;
        MoveReturnValue PostMove(MoveReturnValue gameRetVal);
        virtual void startNewGame() = 0;
        virtual bool IsFinished() = 0;
    };
}
