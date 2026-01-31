#pragma once

#include "CardsRound.h"
#include "Deck.h"
#include "Types.h"
#include "EventEmitter.h"
#include "GameResult.h"

namespace CardsGame_NS
{
    struct IRuleState // rule specific state interface
    {
        virtual ~IRuleState() = default;
    };

    struct GameState
    {
        GameState(fullPlayerId _nextToPlayId, const CardsRound_NS::Players& _players);

        CardsRound_NS::Players players;
        Deck deck;
        int roundCnt;
        fullPlayerId nextToPlayId;
    };

    class CardsGame
    {
    public:
        CardsGame(const GameState& _gameState, std::unique_ptr<IRuleState> ruleState, int _handSize, int _numPlayers, const EventEmitter& _eventEmitter);
        virtual ~CardsGame() = default;

        GameState gameState;
        std::unique_ptr<IRuleState> ruleState;
        GameResult gameResult;
        std::unique_ptr<CardsRound_NS::CardsRound> currRound;

        MoveReturnValue ApplyMove(const Move&);
    protected:
        const EventEmitter& eventEmitter;
        int handSize;
        int numPlayers;

        virtual void InitGame();
        void EndGame();
        virtual bool IsFinished() = 0;

        CardSet drawCards(int8_t numCards);

        void dealCards(int8_t numCards);
        void dealCards(int8_t numCards, std::vector<CardSet>&);

        MoveReturnValue postMove(MoveReturnValue roundRetVal);

        virtual void updateGameResult() = 0;
        virtual void startNewRound() = 0;

        virtual bool isLastRound();

        virtual void postDealtCards(const std::vector<CardSet>& cards);
    private:
        void dealCardsImpl(int8_t numCards, std::vector<CardSet>* out);
    };
}



