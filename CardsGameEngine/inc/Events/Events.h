#pragma once

#include <variant>
#include "Types.h"
#include "RoundResult.h"
#include "GameResult.h"
#include "MatchResult.h"

/* end fwd declares */

struct PrivateEvent
{
    static constexpr EventVisibility visibility = EventVisibility::Private;
};

struct TeamEvent
{
    static constexpr EventVisibility visibility = EventVisibility::Teammate;
};

struct BroadcastEvent
{
    static constexpr EventVisibility visibility = EventVisibility::Broadcast;
};

struct Engineinternal
{
    static constexpr EventVisibility visibility = EventVisibility::EngineInternal;
};

struct PlayerDealtCardsEvent : TeamEvent
{
    fullPlayerId playerId;
    CardSet cards;

    PlayerDealtCardsEvent(fullPlayerId pid, CardSet dealt)
        : playerId(pid), cards(std::move(dealt)) {}
};


struct PlayerPlayedMoveEvent : BroadcastEvent
{
    Move move;

    explicit PlayerPlayedMoveEvent(Move m)
        : move(std::move(m)) {}
};


struct StartRoundEvent : BroadcastEvent
{
    fullPlayerId firstToPlayId;

    explicit StartRoundEvent(fullPlayerId pid)
        : firstToPlayId(pid) {}
};

struct StartGameEvent : BroadcastEvent
{
    fullPlayerId firstToPlayId;

    explicit StartGameEvent(fullPlayerId pid)
        : firstToPlayId(pid) {}
};

struct StartBriscolaGameEvent : BroadcastEvent
{
    fullPlayerId firstToPlayId;
    Card lastCard;

    explicit StartBriscolaGameEvent(fullPlayerId pid, Card _lastCard)
        : firstToPlayId(pid),
          lastCard(_lastCard) {}
};

struct StartMatchEvent : BroadcastEvent
{
    fullPlayerId firstToPlayId;
    GameType gameType;

    explicit StartMatchEvent(fullPlayerId pid, GameType _gameType)
        : firstToPlayId(pid),
          gameType(_gameType)
        {}
};

struct RoundOverEvent : BroadcastEvent
{
    CardsRound_NS::RoundResult roundResult;

    explicit RoundOverEvent(CardsRound_NS::RoundResult result)
        : roundResult(std::move(result)) {}
};


struct GameOverEvent : BroadcastEvent
{
    CardsGame_NS::GameResult gameResult;

    explicit GameOverEvent(CardsGame_NS::GameResult result)
        : gameResult(std::move(result)) {}
};


struct MatchOverEvent : BroadcastEvent
{
    CardsMatch_NS::MatchResult matchResult;

    explicit MatchOverEvent(CardsMatch_NS::MatchResult result)
        : matchResult(std::move(result)) {}
};


struct YourTurnEvent : PrivateEvent
{
    fullPlayerId playerId;
    CardSet yourHand;
    Moves movesPlayedInRound;
    Color strongColor;

    //legal moves

    YourTurnEvent(fullPlayerId pid,
                  CardSet hand,
                  Moves moves,
                  Color color)
        : playerId(pid)
        , yourHand(std::move(hand))
        , movesPlayedInRound(std::move(moves))
        , strongColor(color)
    {}
};

struct MoveResponseEvent : PrivateEvent
{
    Move move;

    MoveReturnValue moveValidity;

    MoveResponseEvent(Move m, MoveReturnValue validity)
        : move(std::move(m))
        , moveValidity(validity)
    {}
};


struct AcussoEvent : BroadcastEvent
{
    fullPlayerId playerId;
    Acussos acussos;

    AcussoEvent(fullPlayerId pid, Acussos a)
        : playerId(pid)
        , acussos(a)
    {}
};

struct BeforeFirstMoveEvent : Engineinternal
{
    fullPlayerId playerId;
};

struct BriscolaLastRoundEvent : TeamEvent
{
    fullPlayerId receiverPlayerId;
    fullPlayerId senderTeammatePlayerId;
    CardSet senderTeammateHand;
    Points teamPoints;

    BriscolaLastRoundEvent(fullPlayerId receiver,
                           fullPlayerId sender,
                           CardSet hand,
                           Points pts)
        : receiverPlayerId(receiver)
        , senderTeammatePlayerId(sender)
        , senderTeammateHand(std::move(hand))
        , teamPoints(pts)
    {}
};

struct TressetteDealtCardsEvent : BroadcastEvent
{
    CardSet dealtCards;
    fullPlayerId playerId;

    TressetteDealtCardsEvent(fullPlayerId _playerId,
                             CardSet _dealtCards)
        : playerId(_playerId)
        , dealtCards(std::move(_dealtCards))
    {}
};

using GameEvent = std::variant<
    PlayerPlayedMoveEvent,
    PlayerDealtCardsEvent,
    StartRoundEvent,
    TressetteDealtCardsEvent,
    StartGameEvent,
    StartBriscolaGameEvent,
    StartMatchEvent,
    RoundOverEvent,
    GameOverEvent,
    MatchOverEvent,
    YourTurnEvent,
    MoveResponseEvent,
    AcussoEvent,
    BriscolaLastRoundEvent,
    BeforeFirstMoveEvent
>;
