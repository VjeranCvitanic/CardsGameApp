#pragma once

#include "Types.h"
#include <string>

namespace Acussos_NS
{
    int AcussoCheck(const CardSet& hand, Acussos& acussosList);
    void CalculateAcussoPoints(const CardSet& hand, int& points, std::vector<AcussoType>& Acussos);
    int Napolitana(const CardSet& hand, std::vector<AcussoType>& Acussos);
    int SameNumberAcusso(const CardSet& hand, std::vector<AcussoType>& Acussos);

    std::string AcussoToString(AcussoType a);
};