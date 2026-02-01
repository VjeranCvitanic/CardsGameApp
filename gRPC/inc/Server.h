#include <grpcpp/grpcpp.h>
#include "GameSession.h"
#include "cardsGame.grpc.pb.h"
#include "cardsGame.pb.h"
#include <mutex>
#include <unordered_map>

struct Client {
    int id;
    std::string name;
    int sessionId;
    grpc::ServerReaderWriter<cardsGame::GameEventMsg, cardsGame::GameEventMsg>* stream;
};

class cardsGameServiceImpl final : public cardsGame::CardsGameServer::Service {
    std::unordered_map<int, Client> clients;
    std::unordered_map<int, GameSession*> gameSessions;
    std::mutex mtx;
    int nextClientId = 1;
    int nextSessionId = 1;
public:
    void RunServer();
    grpc::Status Connect(grpc::ServerContext* context, const cardsGame::ConnectReq* request, cardsGame::ConnectRsp* reply) override;

    grpc::Status GameEventStream(
        const cardsGame::GameEventMsg* request,
        grpc::ServerReaderWriter<cardsGame::GameEventMsg, cardsGame::GameEventMsg>* readWriter);

    grpc::Status PlayMove(grpc::ServerContext* context, const cardsGame::PlayMoveReq* request, cardsGame::PlayMoveRsp* response) override;

private:
    bool ifClientExists(const std::string& name);
};

