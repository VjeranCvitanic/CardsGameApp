#pragma once

#include <unordered_map>
#include "EngineIF.h"
#include "Logger.h"
#include "Types.h"

class GameSession
{
public:
    GameSession(int _numMatches = 1, int _numPlayers = 2, GameType _gameType = GameType::BriscolaGame)
        : numMatchesToPlay(_numMatches), numPlayers(_numPlayers), cntMatchesPlayed(0), teamWins{0, 0}, gameType(_gameType) {}

    void StartSession();
    void ApplyMove(const Move& move);
    bool IsSessionOver() const;
    int AddPlayer(int playerId);
    void PrintResults();

private:
    std::unique_ptr<CardsMatch_NS::CardsMatch> match;

    GameType gameType;
    int numMatchesToPlay;
    int cntMatchesPlayed;
    int numPlayers;

    int teamWins[2];

    std::unordered_map<int, int> players; // player´s server id to player´s session id (0-3)
};