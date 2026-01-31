#pragma once

#include "Events.h"

class EventSink {
public:
    virtual ~EventSink() = default;
    virtual void onEvent(const GameEvent& event) = 0;
};