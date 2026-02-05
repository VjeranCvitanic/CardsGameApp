#include <grpcpp/grpcpp.h>
#include "GameSession.h"
#include "cardsGame.grpc.pb.h"
#include "cardsGame.pb.h"
#include <mutex>
#include <unordered_map>

#define LISTENER_ADDRESS "0.0.0.0:50051"

#define NUM_PLAYERS_IN_SESSION 2 // TODO

struct Client {
    int id;
    std::string name;
    int sessionId;
};

class cardsGameServiceImpl final : public cardsGame::CardsGameServer::Service {
public:
    cardsGameServiceImpl() = default;

    grpc::Status Connect(grpc::ServerContext* context, const cardsGame::ConnectReq* request, cardsGame::ConnectRsp* reply) override;

    void RunListener();

private:
    bool ifClientExists(const std::string& name);
    bool maybeCreateGameSession();

    std::unordered_map<int, Client> clients;
    std::unordered_map<int, GameSession_NS::GameSession*> gameSessions;
    std::mutex mtx;
    int unallocatedClients = 0;
    int nextClientId = 0;
    int nextSessionId = 1;
    int nextPort = 50052;
};

