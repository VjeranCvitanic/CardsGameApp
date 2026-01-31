#pragma once

#include "CardsGame.h"
#include "EventSink.h"
#include "Types.h"
#include "Events.h"
#include <unordered_set>

namespace TressetteGame_NS
{
    struct TressetteRuleState final : CardsGame_NS::IRuleState
    {
        fullPlayerId bastaCalled;
    };

    class TressetteGameState : public CardsGame_NS::GameState
    {
    public:
        TressetteGameState(fullPlayerId _nextToPlayId, const CardsRound_NS::Players& _players) :
            CardsGame_NS::GameState(_nextToPlayId, _players)
        {}
    };

    class TressetteGame : public CardsGame_NS::CardsGame, public EventSink
    {
    public:
        TressetteGame(const TressetteGameState& gameState, int _numPlayers, const EventEmitter& _eventEmitter);
        TressetteRuleState& rule()
        {
            return static_cast<TressetteRuleState&>(*ruleState);
        }
    protected:
        void updateGameResult() override;

        bool IsFinished() override;

        void startNewRound() override;

        void onEvent(const GameEvent& e) override;
        void postDealtCards(const std::vector<CardSet>& cards) override;
    private:
        void handleBeforeFirstMove(const BeforeFirstMoveEvent& e);
        std::unordered_set<PlayerId> acussoHandled;
    };
}
