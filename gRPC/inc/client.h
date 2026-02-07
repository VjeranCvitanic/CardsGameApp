#include <grpcpp/grpcpp.h>
#include <memory>
#include <string>
#include <thread>
#include "cardsGame.grpc.pb.h"
#include "cardsGame.pb.h"

class cardsGameClient {
public:
    cardsGameClient(std::shared_ptr<grpc::Channel> channel, std::string clientName = "", bool _ai = false)
        : lobbyStub_(cardsGame::CardsGameServer::NewStub(channel)), name(clientName), isAi(_ai) {}

    bool Connect(const std::string& name, cardsGame::GameType gameType);
    void PlayMove(std::unique_ptr<cardsGame::Move> move);
    void StartClient();
    bool WaitForSessionStarted(grpc::ClientContext& sessionContext, std::unique_ptr<grpc::ClientReader<cardsGame::GameEventMsg>>& reader);

private:
    std::unique_ptr<cardsGame::CardsGameServer::Stub> lobbyStub_;
    std::unique_ptr<cardsGame::CardsGameSession::Stub> sessionStub_; // i need to set this at start
    int id;
    std::string name;
    std::string gameSessionAddress;
    bool isAi;

    void processEvent(const cardsGame::GameEventMsg& event);
    void processMyTurn(const cardsGame::GameEventMsg& event);

    // human if
    void parseInput(cardsGame::Move* move, int i);
    int parse(std::string input, cardsGame::Move* move, int playerId);
};
