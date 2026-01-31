#pragma once

#include "CardsRound.h"
#include "Types.h"

namespace TressetteRound_NS
{
    class TressetteRoundState : public CardsRound_NS::RoundState
    {
    public:
        TressetteRoundState(fullPlayerId _nextToPlayId, const CardsRound_NS::Players& _players);
    };

    class TressetteRound : public CardsRound_NS::CardsRound
    {
    public:
        TressetteRound(const TressetteRoundState& _state, int _numPlayers, const EventEmitter& _eventEmitter);
        TressetteRound(const TressetteRound&) = default;
        void preMoveSetup() override;
        bool IsFinished() override;
        void postMoveSetup(const Move&) override;

        // todo tressette only
        fullPlayerId bastaCalled = {-1,-1};
    };
}
