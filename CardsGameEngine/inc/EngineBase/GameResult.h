#pragma once

#include "Types.h"
#include "Points.h"

namespace CardsGame_NS
{
    struct GameResult
    {
        GameResult() :
            winnerId(-1),
            points(0)
        {}

        TeamId winnerId;
        std::unordered_map<TeamId, Points> points;
    };
}