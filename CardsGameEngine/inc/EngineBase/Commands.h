#pragma once

#include "Types.h"
#include <string>
#include <variant>

struct PlayCardCommand {
    fullPlayerId playerId;
    Move     move;
};

struct OtherCommand {
    fullPlayerId playerId;
    std::string msg;
};

using GameCommand = std::variant<PlayCardCommand, OtherCommand>;
