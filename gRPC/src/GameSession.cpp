#include "GameSession.h"

void GameSession::StartSession() {
    std::cout << "Game session started!" << std::endl;

    EventEmitter eventEmitter;
    eventEmitter.subscribe(&Logger::GetInstance());

    
    switch(gameType) {
        case GameType::BriscolaGame:
            match = createBriscolaMatch(eventEmitter, numPlayers);
            break;
        case GameType::TressetteGame:
            match = createTressetteMatch(eventEmitter, numPlayers);
            break;
        default:
            std::cerr << "Unknown game type!" << std::endl;
            return;
    }
}

void GameSession::ApplyMove(const Move& move) {
    match->ApplyMove(move);
    if(match->IsFinished())
    {
        cntMatchesPlayed++;
        int winningTeam = match->matchResult.winnerId;
        teamWins[winningTeam]++;
        std::cout << "Match finished! Winning team: " << winningTeam << std::endl;
    }
}

bool GameSession::IsSessionOver() const {
    return cntMatchesPlayed >= numMatchesToPlay;
}

int GameSession::AddPlayer(int playerId) {
    int sessionId = players.size();
    players[sessionId] = playerId;
    return sessionId;
}

void GameSession::PrintResults() {
    std::cout << "Game session results:" << std::endl;
    std::cout << "Team 1 wins: " << teamWins[0] << std::endl;
    std::cout << "Team 2 wins: " << teamWins[1] << std::endl;
}