#include <grpcpp/grpcpp.h>
#include "GameSession.h"
#include "cardsGame.grpc.pb.h"
#include "cardsGame.pb.h"
#include <condition_variable>
#include <mutex>
#include <unordered_map>
#include <vector>

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

// Routes SubscribeEvents / PlayMove to the correct GameSession by session_id.
// Registered once on the shared gRPC server alongside CardsGameServer.
class SessionDispatcher final : public cardsGame::CardsGameSession::Service {
public:
    void registerSession(int sessionId, GameSession_NS::GameSession* session);
    void unregisterSession(int sessionId);

    grpc::Status SubscribeEvents(grpc::ServerContext* context,
                                 const cardsGame::PlayerInfo* request,
                                 grpc::ServerWriter<cardsGame::GameEventMsg>* writer) override;
    grpc::Status PlayMove(grpc::ServerContext* context,
                          const cardsGame::PlayMoveReq* request,
                          google::protobuf::Empty* response) override;

private:
    GameSession_NS::GameSession* findSession(int sessionId);
    GameSession_NS::GameSession* waitForSession(int sessionId, int timeoutSeconds);

    std::unordered_map<int, GameSession_NS::GameSession*> sessions_;
    std::mutex mutex_;
    std::condition_variable cv_;
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

    SessionDispatcher dispatcher_;

    // Match results storage
    struct MatchResultRecord {
        int sessionId;
        int winnerTeamId;
        int teamWins[2];
    };
    std::vector<MatchResultRecord> matchResults_;
    std::mutex resultsMutex_;

    // to avoid having to define hashing function for GameFormat struct
    std::map<cardsGame::GameType, std::map<cardsGame::SingleOrMulti, std::vector<Client*>>> waitingClients;

    int nextClientId = 0;
    int nextSessionId = 1;
};

