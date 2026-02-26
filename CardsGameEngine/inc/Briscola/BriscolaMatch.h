#pragma once

#include "CardsMatch.h"

namespace BriscolaMatch_NS
{

    class BriscolaMatch : public CardsMatch_NS::CardsMatch
    {
    public:
        BriscolaMatch(const CardsMatch_NS::MatchState& matchState, int _numPlayers, const EventEmitter& _eventEmitter);

        bool IsFinished() override;
    protected:
        void startNewGame() override;
        void updateMatchResult() override;
        void printMatchResult() override;
        void InitMatch() override;
    };
}
