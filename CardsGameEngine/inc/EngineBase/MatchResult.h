#pragma once

#include "Types.h"
#include "Points.h"

namespace CardsMatch_NS
{
    union Score
    {
        Points points;
        int wonGames;
        Score() : points(0) {}
    };
    struct MatchResult
    {
        MatchResult() :
            winnerId(-1),
            score(0)
        {}

        TeamId winnerId;
        std::unordered_map<TeamId, Score> score;
    };
}