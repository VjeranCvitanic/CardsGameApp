#pragma once

#include "CardsRound.h"
#include "Types.h"

namespace BriscolaRound_NS
{
    class BriscolaRoundState : public CardsRound_NS::RoundState
    {
    public:
        BriscolaRoundState(Color _strongColor, fullPlayerId _nextToPlayId, const CardsRound_NS::Players& _players);
    };

    class BriscolaRound : public CardsRound_NS::CardsRound
    {
    public:
        BriscolaRound(const BriscolaRoundState& _state, int _handsize, int _numPlayers, const EventEmitter& _eventEmitter);
        BriscolaRound(const BriscolaRound&) = default;
        void preMoveSetup() override;
    };
}
