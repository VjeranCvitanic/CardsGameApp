#pragma once

#include "Deck.h"
#include "Types.h"
#include "Points.h"
#include "RoundResult.h"
#include "EventEmitter.h"

namespace CardsRound_NS
{
    struct PlayerState
    {
        fullPlayerId playerId;
        Deck deck;
    };

    typedef std::vector<PlayerState> Players;

    struct RoundState
    {
        RoundState(fullPlayerId _nextToPlayId, const Players& _players);

        Players players;
        Moves playedMovesInRound;
        MoveConstraints moveConstraints;

        Color strongColor;

        fullPlayerId nextToPlayId;
        fullPlayerId firstToPlayId;
    };

    class RoundRules
    {
    public:
        RoundRules();
        virtual int8_t getNumberStrength(Number number) const = 0;
        virtual Points getNumberValue(Number number) const = 0;
        virtual Card StrongerCard(const Card& card1, const Card& card2, Color strongColor) const = 0;
        virtual bool IsMoveLegal(const Move&, const RoundState& state, MoveReturnValue& reason) const = 0;

        int8_t StrongestCard(const CardSet& playedHand, Card& winnerCard, Color strongColor) const;
    };

    class CardsRound
    {
    public:
        explicit CardsRound(RoundRules& rules, const RoundState& state, int _handSize, int _numPlayers, const EventEmitter& _eventEmitter);
        virtual ~CardsRound() = default;

        RoundState roundState;
        RoundResult roundResult;

        int handSize;
        int numPlayers;

        MoveReturnValue ApplyMove(const Move&);
    protected:
        const RoundRules& roundRules;
        const EventEmitter& eventEmitter;

        void logStartRound();

        void InitRound();
        void EndRound();
        virtual bool IsFinished();
        void playMove(const Move&);

        Points CalculateRoundResult();

        int8_t HandWinner(const CardSet& playedHand, Card& winnerCard, Color strongColor);

        virtual void preMoveSetup();
        virtual void postMoveSetup(const Move&);
        void emitNextYourTurnEvent();
        void emitMoveRspEvent(const Move& move, MoveReturnValue moveValidity);
    private:
    };
}
