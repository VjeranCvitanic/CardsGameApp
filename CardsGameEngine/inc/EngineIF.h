#pragma once

#include "BriscolaMatch.h"
#include "TressetteMatch.h"

std::unique_ptr<BriscolaMatch_NS::BriscolaMatch>
createBriscolaMatch(EventEmitter& emitter,
                    int numPlayers);

std::unique_ptr<TressetteMatch_NS::TressetteMatch>
createTressetteMatch(EventEmitter& emitter,
                    int numPlayers);