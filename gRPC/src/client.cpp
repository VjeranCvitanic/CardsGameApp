#include <grpcpp/grpcpp.h>
#include "cardsGame.grpc.pb.h"
#include "cardsGame.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using cardsGame::CardsGameServer;
using cardsGame::ConnectReq;
using cardsGame::ConnectRsp;

class cardsGameClient {
public:
    cardsGameClient(std::shared_ptr<Channel> channel)
        : stub_(CardsGameServer::NewStub(channel)) {}

    std::string SayHello(const std::string& name, cardsGame::GameType gameType, int& id) {
        ConnectReq request;
        request.set_name(name);
        request.set_gametype(gameType);

        ConnectRsp reply;
        ClientContext context;

        Status status = stub_->Connect(&context, request, &reply);
        if (status.ok()) {
            id = reply.success_id();
            return reply.message();
        } else {
            return "RPC failed";
        }
    }

    std::string PlayMove(const std::string& name, cardsGame::MoveRsp& id) {
        cardsGame::PlayMoveReq request;
        cardsGame::Move* move = new cardsGame::Move();
        
        cardsGame::Card* card = new cardsGame::Card();
        card->set_color(cardsGame::CardColor::DENARI);
        card->set_number(cardsGame::CardNumber::CAVALLO);
        move->set_allocated_card(card);

        cardsGame::Call call = cardsGame::Call::NO_CALL;
        move->set_call(call);
        request.set_allocated_move(move);

        request.set_playerid(name);

        cardsGame::PlayMoveRsp reply;
        ClientContext context;

        Status status = stub_->PlayMove(&context, request, &reply);
        if (status.ok()) {
            id = reply.moversp();
            return "move ok";
        } else {
            return "move bad";
        }
    }

private:
    std::unique_ptr<CardsGameServer::Stub> stub_;
};

int main(int argc, char** argv) {
    if(argc != 3) {
        std::cout << "Usage: client <name> <command>" << std::endl;
        return 1;
    }

    std::string name = argv[1];;
    std::string command = argv[2];
    cardsGameClient client(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));
    int id = 0;
    std::string reply = client.SayHello(name, cardsGame::GameType::BRISCOLA, id);
    std::cout << "Server replied: " << reply << std::endl;
    std::cout << "ID: " << id << std::endl;

    std::cout << "Playing move..." << std::endl;
    cardsGame::MoveRsp moveResp;
    std::string moveReply = client.PlayMove(name,moveResp);
    std::cout << "Server replied: " << moveReply << std::endl;
    std::cout << "Move Response: " << moveResp << std::endl;
    return 0;
}
