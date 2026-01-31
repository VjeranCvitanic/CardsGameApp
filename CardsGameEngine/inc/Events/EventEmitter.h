#pragma once

#include "EventSink.h"
#include "Events.h"

class EventEmitter {
public:
    void subscribe(EventSink* sink) {
        sinks.push_back(sink);
    }

    void emit(GameEvent event) const {
        for (auto* s : sinks) {
            s->onEvent(event);
        }
    }

private:
    std::vector<EventSink*> sinks;
};
