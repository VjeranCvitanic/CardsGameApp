#pragma once
#include <grpcpp/grpcpp.h>
#include "Events.h"
#include "cardsGame.grpc.pb.h"
#include "cardsGame.pb.h"
#include "EngineIF.h"
#include <chrono>
#include <memory>
#include <unordered_map>
#include "EventEmitter.h"
#include "Types.h"

// Forward declaration for test friend class
class TestableGameSession;

namespace GameSession_NS {

class PlayerConnection {
public:
    explicit PlayerConnection(grpc::ServerWriter<cardsGame::GameEventMsg>* w)
        : writer(w) {}

    bool send(const cardsGame::GameEventMsg& msg) {
        std::lock_guard<std::mutex> lock(mtx);
        return writer && writer->Write(msg);
    }

private:
    grpc::ServerWriter<cardsGame::GameEventMsg>* writer; // non-owning
    std::mutex mtx;
};

class GameSession : public cardsGame::CardsGameSession::Service, public EventSink
{
    friend class ::TestableGameSession;  // Allow block tests to access private members
public:
    GameSession(int sessionId, std::vector<int> _playersIdList, int _numMatches = 1, cardsGame::GameType _gameType = cardsGame::GameType::BRISCOLA, int _numPlayers = 2)
        : sessionId(sessionId), numMatchesToPlay(_numMatches), cntMatchesPlayed(0), teamWins{0, 0}, gameType(_gameType), numPlayers(_numPlayers)
        {
            std::cout << "GameSession constructor: registering " << _playersIdList.size() << " players" << std::endl;
            for(size_t i = 0; i < _playersIdList.size(); i++) {
                std::cout << "  Mapping server player ID " << _playersIdList[i] << " -> session ID " << i << std::endl;
                players[_playersIdList[i]] = i;
            }
        }

    struct SessionResult {
        int sessionId = -1;
        int winnerTeamId = -1;
        int teamWins[2] = {0, 0};
    };

    void StartSession();
    SessionResult GetResult() const;
    grpc::Status PlayMove(grpc::ServerContext* context, const cardsGame::PlayMoveReq* request, ::google::protobuf::Empty* response) override;
    grpc::Status SubscribeEvents(grpc::ServerContext* context, 
                                const cardsGame::PlayerInfo* request,
                                grpc::ServerWriter<cardsGame::GameEventMsg>* writer) override;
    grpc::Status SpectateSession(grpc::ServerContext* context,
                                const cardsGame::SpectateReq* request,
                                grpc::ServerWriter<cardsGame::GameEventMsg>* writer) override;
        

private:
    std::unique_ptr<CardsMatch_NS::CardsMatch> match;

    std::unordered_map<int, std::shared_ptr<PlayerConnection>> connections;
    std::mutex connectionsMutex;

    cardsGame::GameType gameType;
    int numMatchesToPlay;
    int cntMatchesPlayed;
    int sessionId;

    bool isStarted = false;

    int teamWins[2];

    std::unique_ptr<EventEmitter> eventEmitter;

    std::unordered_map<int, int> players; // server player ID -> session ID (engine player ID 0-3)
    int numPlayers = 0;

    // Reconnection support
    static constexpr int RECONNECT_TIMEOUT_SECONDS = 30;
    std::unordered_map<int, std::vector<cardsGame::GameEventMsg>> eventHistory_;
    std::unordered_map<int, std::chrono::steady_clock::time_point> disconnectedPlayers_;
    bool forfeited_ = false;
    int forfeitWinnerTeam_ = -1;

    // Spectator support
    std::unordered_map<int, std::shared_ptr<PlayerConnection>> spectatorConnections_;
    std::mutex spectatorMutex_;
    int nextSpectatorId_ = 0;

    void sendEvent(int sessionPlayerId, const cardsGame::GameEventMsg& event);
    void broadcastEvent(cardsGame::GameEventMsg& event, int excludeSessionId = -1);
    void sendToSpectators(const cardsGame::GameEventMsg& event);

    MoveReturnValue ApplyMove(const Move& move);
    bool IsSessionOver() const;
    void PrintResults();
    void startMatch();


    void onEvent(const PlayerPlayedMoveEvent& event);
    void onEvent(const PlayerDealtCardsEvent& event);
    void onEvent(const StartRoundEvent& event);
    void onEvent(const TressetteDealtCardsEvent& event);
    void onEvent(const StartGameEvent& event);
    void onEvent(const StartBriscolaGameEvent& event);
    void onEvent(const StartMatchEvent& event);
    void onEvent(const RoundOverEvent& event);
    void onEvent(const GameOverEvent& event);
    void onEvent(const MatchOverEvent& event);
    void onEvent(const YourTurnEvent& event);
    void onEvent(const MoveResponseEvent& event);
    void onEvent(const AcussoEvent& event);
    void onEvent(const BriscolaLastRoundEvent& event);


    void onEvent(const GameEvent& event) override;

    void dealCards(const PlayerDealtCardsEvent& event);
    void startRound(const StartRoundEvent& event);
    void startGame(const StartGameEvent& event);
    void startBriscolaGame(const StartBriscolaGameEvent& event);
    void startMatch(const StartMatchEvent& event);
    void yourTurn(const YourTurnEvent& event);
    void playerPlayedMoveEvent(const PlayerPlayedMoveEvent& event);
    void endRound(const RoundOverEvent& event);
    void endGame(const GameOverEvent& event);
    void endMatch(const MatchOverEvent& event);
    void moveRsp(const MoveResponseEvent& event);
    void tressetteDealtCards(const TressetteDealtCardsEvent& event);
    void acussoEvent(const AcussoEvent& event);
    void briscolaLastRound(const BriscolaLastRoundEvent& event);

    cardsGame::AcussoMsg::AcussoType translateAcussoType(AcussoType engineType);

    int playerIdToSessionPlayerId(int playerId);

    void PlayMoveReqToDomain(const cardsGame::PlayMoveReq& req, Move& move);
    void MoveRspToProto(const MoveReturnValue& move, cardsGame::MoveRsp& moveValidity);
};
}
