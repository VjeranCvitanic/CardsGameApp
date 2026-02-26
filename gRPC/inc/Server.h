#include <grpcpp/grpcpp.h>
#include "GameSession.h"
#include "cardsGame.grpc.pb.h"
#include "cardsGame.pb.h"
#include <mutex>
#include <unordered_map>

#define LISTENER_ADDRESS "0.0.0.0:50051"

struct GameFormat {
    cardsGame::GameType gameType;
    cardsGame::SingleOrMulti SingleOrMulti;
};

struct Client {
    int id;
    std::string name;
    std::vector<GameFormat> gameFormats;
    int sessionId;

    Client(int _id, std::string _name, std::vector<GameFormat> _gameFormats, int _sessionId)
        : id(_id), name(_name),
          gameFormats(_gameFormats),
          sessionId(_sessionId) {};
};

class cardsGameServiceImpl final : public cardsGame::CardsGameServer::Service {
public:
    cardsGameServiceImpl() = default;

    grpc::Status Connect(grpc::ServerContext* context, const cardsGame::ConnectReq* request, cardsGame::ConnectRsp* reply) override;

    void RunListener();

private:
    bool ifClientExists(const std::string& name);
    bool maybeCreateGameSession(GameFormat format);
    void addClientToWaitingLists(Client* c);
    void removeClientFromWaitingLists(Client* c);

    std::unordered_map<int, Client> clients;
    std::unordered_map<int, std::shared_ptr<GameSession_NS::GameSession>> gameSessions;
    std::mutex sessionMutex;

    // to avoid having to define hashing function for GameFormat struct
    std::map<cardsGame::GameType, std::map<cardsGame::SingleOrMulti, std::vector<Client*>>> waitingClients;

    int nextClientId = 0;
    int nextSessionId = 1;
    int nextPort = 50052;
};

