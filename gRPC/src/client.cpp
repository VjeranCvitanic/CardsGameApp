#include "client.h"
#include "cardsGame.grpc.pb.h"
#include "cardsGame.pb.h"
#include <unistd.h>



std::string cardsGameClient::Connect(const std::string& name, cardsGame::GameType gameType) {
    cardsGame::ConnectReq request;
    request.set_name(name);
    request.set_gametype(gameType);

    cardsGame::ConnectRsp reply;
    grpc::ClientContext context;

    grpc::Status status = lobbyStub_->Connect(&context, request, &reply);
    if (status.ok()) {
        std::cout << "My id: " << reply.successmsg().playerid() << std::endl;
        id = reply.successmsg().playerid();
        gameSessionAddress = reply.successmsg().address();
        grpc::ChannelArguments args;
        args.SetInt("grpc.wait_for_ready", 1);
        sessionStub_ = std::unique_ptr<cardsGame::CardsGameSession::Stub>(cardsGame::CardsGameSession::NewStub(
            grpc::CreateCustomChannel(gameSessionAddress, grpc::InsecureChannelCredentials(), args)
        ));
        return reply.message();
    } else {
        return "RPC failed";
    }
}

bool cardsGameClient::WaitForSessionStarted(grpc::ClientContext& sessionContext, std::unique_ptr<grpc::ClientReader<cardsGame::GameEventMsg>>& reader) {    
    // Optional: Add deadline to avoid hanging
    sessionContext.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(10));
    sessionContext.set_wait_for_ready(true);
    cardsGame::PlayerInfo playerInfo;
    playerInfo.set_playerid(id);
    
    cardsGame::GameEventMsg sessionReply;
    grpc::Status status;
    for (int i = 0; i < 10; ++i) {        
        reader = sessionStub_->SubscribeEvents(&sessionContext, playerInfo); 
        
        if (reader && reader->Read(&sessionReply)) {
            if (sessionReply.eventtype() == cardsGame::EventType::START_MATCH_EVENT) {
                return true;
        }
        sleep(1);
    }
    std::cout << "Session started successfully" << std::endl;
    return true;
    }

    return false;
}

std::string cardsGameClient::PlayMove(cardsGame::MoveRsp& rsp) {
    cardsGame::Card card;
    card.set_color(cardsGame::CardColor::DENARI);
    card.set_number(cardsGame::CardNumber::CAVALLO);
    cardsGame::Move move;
    move.set_allocated_card(&card);
    move.set_call(cardsGame::Call::NO_CALL);
    cardsGame::PlayMoveRsp moveResp;
    grpc::ClientContext moveCtx;

    cardsGame::PlayMoveReq request;
    request.set_playerid(id);
    request.set_allocated_move(&move);

    if(!sessionStub_) {
        return "Session stub is null";
    }
    grpc::Status s = sessionStub_->PlayMove(&moveCtx, request, &moveResp);

    if(s.ok()) {
        std::cout << "Move response: " << moveResp.DebugString() << std::endl;
    } else {
        std::cerr << "PlayMove failed: " << s.error_message() << std::endl;
    }

    return moveResp.DebugString();
}

void cardsGameClient::StartClient() {

    Connect(name, cardsGame::GameType::BRISCOLA);
    std::cout << "return: " << gameSessionAddress << std::endl;

    std::thread eventThread([&]() {
        grpc::ClientContext context;
        cardsGame::PlayerInfo playerInfo;
        playerInfo.set_playerid(id); // assume returned from lobby

        std::unique_ptr<grpc::ClientReader<cardsGame::GameEventMsg>> reader;
        WaitForSessionStarted(context, reader);

        cardsGame::GameEventMsg event;
        std::cout << "before loop" << std::endl;

        while(reader->Read(&event)) {
            std::cout << "IN LOOP" << std::endl;

            // TODO process Event function
            std::cout << "[EVENT] " << event.DebugString() << std::endl;
        }

        grpc::Status status = reader->Finish();
        if(!status.ok()) {
            std::cerr << "Event stream closed: " << status.error_message() << std::endl;
        }
    });
    eventThread.join();
}

int main(int argc, char** argv) {
    if(argc != 2) {
        std::cout << "Usage: client <name>" << std::endl;
        return 1;
    }

    std::string name = argv[1];

    cardsGameClient lobbyClient(grpc::CreateChannel(
        "localhost:50051", grpc::InsecureChannelCredentials()), name);

    lobbyClient.StartClient();

    //cardsGameClient lobbyClient2(grpc::CreateChannel(
    //    "localhost:50051", grpc::InsecureChannelCredentials()), "player2");

    //lobbyClient2.Connect("player2", cardsGame::GameType::BRISCOLA);

    //cardsGame::MoveRsp rsp;
    //lobbyClient.PlayMove(rsp);
    
    return 0;
}