#pragma once
#include <grpcpp/grpcpp.h>
#include "Events.h"
#include "cardsGame.grpc.pb.h"
#include "cardsGame.pb.h"
#include "EngineIF.h"
#include <memory>
#include <unordered_map>
#include "EventEmitter.h"
#include "Types.h"

namespace GameSession_NS {
struct Client {
    int id; // server assigned id
    int engineId; // 0-3
    //stream
};

class GameSession : public cardsGame::CardsGameSession::Service, public EventSink
{
public:
    GameSession(int Port, int sessionId, std::vector<int> _playersIdList, int _numMatches = 1, cardsGame::GameType _gameType = cardsGame::GameType::BRISCOLA)
        : port(Port), sessionId(sessionId), numMatchesToPlay(_numMatches), cntMatchesPlayed(0), teamWins{0, 0}, gameType(_gameType)
        {
            for(size_t i = 0; i < _playersIdList.size(); i++) {
                players[i] = _playersIdList[i];
            }
        }

    void StartSession();
    grpc::Status PlayMove(grpc::ServerContext* context, const cardsGame::PlayMoveReq* request, cardsGame::PlayMoveRsp* response) override;
    grpc::Status SubscribeEvents(grpc::ServerContext* context, 
                                const cardsGame::PlayerInfo* request,
                                grpc::ServerWriter<cardsGame::GameEventMsg>* writer) override;
        

private:
    std::unique_ptr<CardsMatch_NS::CardsMatch> match;

    int port;
    cardsGame::GameType gameType;
    int numMatchesToPlay;
    int cntMatchesPlayed;
    int sessionId;

    bool isStarted = false;

    int teamWins[2];

    std::unique_ptr<EventEmitter> eventEmitter;

    std::unordered_map<int, int> players; // player´s server id to player´s session id (0-3)

    void ApplyMove(const Move& move);
    bool IsSessionOver() const;
    int AddPlayer(int playerId);
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
    void onEvent(const BeforeFirstMoveEvent& event);


    void onEvent(const GameEvent& event) override;
};
}