#include <grpcpp/grpcpp.h>
#include <memory>
#include <string>
#include <thread>
#include "cardsGame.grpc.pb.h"
#include "cardsGame.pb.h"

class cardsGameClient {
public:
    cardsGameClient(std::shared_ptr<grpc::Channel> channel, std::string clientName = "", bool _ai = false,
        cardsGame::GameFormat gameFormat = cardsGame::GameFormat())
        : lobbyStub_(cardsGame::CardsGameServer::NewStub(channel)), name(clientName), isAi(_ai),
        gameFormat(gameFormat) {}

    bool Connect(const std::string& name, cardsGame::GameType gameType, cardsGame::SingleOrMulti singleOrMulti);
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

    cardsGame::GameFormat gameFormat;

    void processEvent(const cardsGame::GameEventMsg& event);
    void processMyTurn(const cardsGame::GameEventMsg& event);

    // human if
    void parseInput(cardsGame::Move* move, int i);
    int parse(std::string input, cardsGame::Move* move, int playerId);
};
