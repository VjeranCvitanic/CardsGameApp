#pragma once

#include <tuple>
#include <utility>
#include <vector>
#include <unordered_map>

enum Color
{
    Spade = 1, // has to be 1 to match with proto
    Coppe,
    Denari,
    Bastoni,
    InvalidColor,
    NoColor
};

enum Number
{
    Asso = 1,
    Due,
    Tre,
    Quattro,
    Cinque,
    Sei,
    Sette,
    Fante,
    Cavallo,
    Re,
    InvalidNumber
};

enum NumPlayers
{
    Two = 2,
    Four = 4
};

enum GameType
{
    InvalidGameType = 0,
    BriscolaGame,
    TressetteGame,
};

enum Call
{
    NoCall = 0,
    Busso,
    Striscio,
    ConQuestaBasta
};

enum AcussoType
{
    NapolitanaSpade = 0,
    NapolitanaCoppe,
    NapolitanaDenari,
    NapolitanaBastoni,
    AssoAcusso,
    DueAcusso,
    TreAcusso,
    AssoSenzaSpade,
    AssoSenzaCoppe,
    AssoSenzaDenari,
    AssoSenzaBastoni,
    DueSenzaSpade,
    DueSenzaCoppe,
    DueSenzaDenari,
    DueSenzaBastoni,
    TreSenzaSpade,
    TreSenzaCoppe,
    TreSenzaDenari,
    TreSenzaBastoni,
    NoAcusso
};

typedef int ExternalPlayerId;

typedef int PortNumber;

typedef int TeamId;
typedef int PlayerId;

typedef std::pair<PlayerId, TeamId> fullPlayerId;

typedef std::tuple<Color, Number> Card;
typedef std::vector<Card> CardSet;

typedef std::vector<AcussoType> Acussos;

typedef std::unordered_map<fullPlayerId, Acussos> AcussosMap;

struct Move
{
public:
    Card card;
    Call call;
    fullPlayerId playerId;
};

typedef std::vector<Move> Moves;

struct MoveConstraints
{
    MoveConstraints() :
        colorToPlay(NoColor)
    {}
    Color colorToPlay;
};

enum MoveReturnValue
{
    InvalidMoveReturnValue = 0,
    Ok = 1,
    NotYourTurn = 2,
    CardNotInHand = 3,
    ColorConstraintNotMet = 4,
    CantCallIfNotFirstOfHand = 5,
    Finish = 6 // not in proto
};

enum EventVisibility
{
    Private = 0,
    Teammate = 1,
    Broadcast,
    EngineInternal
};