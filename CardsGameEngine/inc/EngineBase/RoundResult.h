#pragma once

#include "Types.h"
#include "Points.h"

namespace CardsRound_NS
{
    struct RoundResult
    {
        RoundResult();

        fullPlayerId winnerId;
        Points points;
    };
}
