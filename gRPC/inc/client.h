#include <grpcpp/grpcpp.h>
#include <string>
#include <thread>
#include "cardsGame.grpc.pb.h"
#include "cardsGame.pb.h"

class cardsGameClient {
public:
    cardsGameClient(std::shared_ptr<grpc::Channel> channel, std::string clientName = "")
        : lobbyStub_(cardsGame::CardsGameServer::NewStub(channel)), name(clientName) {}

    std::string Connect(const std::string& name, cardsGame::GameType gameType);
    std::string PlayMove(cardsGame::MoveRsp& rsp);
    void StartClient();
    bool WaitForSessionStarted(grpc::ClientContext& sessionContext, std::unique_ptr<grpc::ClientReader<cardsGame::GameEventMsg>>& reader);

private:
    std::unique_ptr<cardsGame::CardsGameServer::Stub> lobbyStub_;
    std::unique_ptr<cardsGame::CardsGameSession::Stub> sessionStub_; // i need to set this at start
    int id;
    std::string name;
    std::string gameSessionAddress;
};
