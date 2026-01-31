#pragma once

#include "CardsMatch.h"

namespace TressetteMatch_NS
{

    class TressetteMatch : public CardsMatch_NS::CardsMatch
    {
    public:
        TressetteMatch(const CardsMatch_NS::MatchState& matchState, int _numPlayers, const EventEmitter& _eventEmitter);

    protected:
        bool IsFinished() override;

        void startNewGame() override;
        void updateMatchResult() override;
        void InitMatch() override;
    };
}
