#pragma once
#include <grpcpp/grpcpp.h>
#include <memory>
#include <string>
#include <thread>
#include "cardsGame.grpc.pb.h"
#include "cardsGame.pb.h"

// BotClient is a simple random-move AI that connects to the unified server
// on localhost:50051 and plays out a full game autonomously.
// Spawned in-process by the server when bot slot filling is triggered.
namespace Bot_NS {

class BotClient {
public:
    BotClient(std::shared_ptr<grpc::Channel> channel, std::string name,
              cardsGame::GameType gameType, cardsGame::SingleOrMulti singleOrMulti)
        : channel_(channel),
          stub_(cardsGame::CardsGameServer::NewStub(channel)),
          name_(std::move(name)), gameType_(gameType), singleOrMulti_(singleOrMulti) {}

    // Connect and play the game asynchronously on a detached thread.
    void StartAsync();

private:
    void Run();
    void PlayMove(int sessionPlayerId, int sessionId, const cardsGame::YourTurnMsg& turn,
                  cardsGame::CardsGameSession::Stub* sessionStub);

    std::shared_ptr<grpc::Channel> channel_;
    std::unique_ptr<cardsGame::CardsGameServer::Stub> stub_;
    std::string name_;
    cardsGame::GameType gameType_;
    cardsGame::SingleOrMulti singleOrMulti_;

    int playerId_  = -1;
    int sessionId_ = -1;
};

} // namespace Bot_NS
