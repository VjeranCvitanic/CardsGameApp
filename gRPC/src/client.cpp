#include "client.h"
#include "cardsGame.grpc.pb.h"
#include "cardsGame.pb.h"
#include <chrono>
#include <iostream>
#include <memory>
#include <ostream>
#include <unistd.h>



bool cardsGameClient::Connect(const std::string& name, cardsGame::GameType gameType) {
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
        return true;
    } else {
         std::cout << "RPC failed" << status.error_message() << std::endl;
;        return false;
    }
}

bool cardsGameClient::WaitForSessionStarted(grpc::ClientContext& sessionContext, std::unique_ptr<grpc::ClientReader<cardsGame::GameEventMsg>>& reader) {    
    // Optional: Add deadline to avoid hanging
    sessionContext.set_deadline(std::chrono::system_clock::now() + std::chrono::minutes(30));
    sessionContext.set_wait_for_ready(true);
    cardsGame::PlayerInfo playerInfo;
    playerInfo.set_playerid(id);
    
    cardsGame::GameEventMsg sessionReply;
    grpc::Status status;
    while(true) {        
        reader = sessionStub_->SubscribeEvents(&sessionContext, playerInfo); 
        
        if (reader && reader->Read(&sessionReply)) {
            if (sessionReply.eventtype() == cardsGame::EventType::START_MATCH_EVENT) {
                return true;
        }
        usleep(1000000);
        }
    }

    return false;
}

void cardsGameClient::PlayMove(std::unique_ptr<cardsGame::Move> move) {
    google::protobuf::Empty response;
    grpc::ClientContext moveCtx;

    cardsGame::PlayMoveReq request;
    request.set_playerid(id);
    request.set_allocated_move(move.release());

    if(!sessionStub_) {
        std::cout << "Session stub is null" << std::endl;
    }
    grpc::Status s = sessionStub_->PlayMove(&moveCtx, request, &response);

    if(!s.ok()) {
        std::cerr << "PlayMove failed: " << s.error_message() << std::endl;
    }
}

void cardsGameClient::StartClient() {
    if(!Connect(name, cardsGame::GameType::BRISCOLA))
    {
        std::cout << "Failed to connect" << std::endl;
        return;
    }

    std::thread eventThread([&]() {
        grpc::ClientContext context;
        cardsGame::PlayerInfo playerInfo;
        playerInfo.set_playerid(id); // assume returned from lobby

        std::unique_ptr<grpc::ClientReader<cardsGame::GameEventMsg>> reader;
        WaitForSessionStarted(context, reader);

        cardsGame::GameEventMsg event;

        while(reader->Read(&event)) {
            std::cout << "IN LOOP" << std::endl;

            // TODO process Event function
            std::cout << "[EVENT] " << event.DebugString() << std::endl;
            processEvent(event);
            //getchar();
        }

        grpc::Status status = reader->Finish();
        if(!status.ok()) {
            std::cerr << "Event stream closed: " << status.error_message() << std::endl;
        }
        else {
            std::cout << "Event stream finished." << std::endl;
        }
    });

    eventThread.join();
}

int main(int argc, char** argv) {
    if(argc != 3) {
        std::cout << "Usage: client <name> <human|ai>" << std::endl;
        return 1;
    }

    std::string name = argv[1];
    bool ai = std::string(argv[2]) == "ai";

    cardsGameClient lobbyClient(grpc::CreateChannel(
        "localhost:50051", grpc::InsecureChannelCredentials()), name, ai);

    lobbyClient.StartClient();
    
    return 0;
}

void cardsGameClient::processEvent(const cardsGame::GameEventMsg& event)
{
    if(event.eventtype() == cardsGame::YOUR_TURN_EVENT)
    {
        processMyTurn(event);
    }
    else if(event.eventtype() == cardsGame::GAME_OVER_EVENT)
    {
        //getchar();
    }
}

void cardsGameClient::processMyTurn(const cardsGame::GameEventMsg& event)
{
    std::cout << "My turn YEAH!" << std::endl;

    if(!isAi)
    {
        std::unique_ptr<cardsGame::Move> move = std::make_unique<cardsGame::Move>();
        parseInput(move.get(), event.yourturn().playerid());

        PlayMove(std::move(move));

        return;
    }

    auto& hand = event.yourturn().hand();

    for(auto& c : hand)
    {
        std::cout << c.color() << " " << c.number() << std::endl;
    }

    std::unique_ptr<cardsGame::Move> move = std::make_unique<cardsGame::Move>();
    std::unique_ptr<cardsGame::Card> card = std::make_unique<cardsGame::Card>();
    card->set_color(hand[0].color());
    card->set_number(hand[0].number());

    move->set_allocated_card(card.release());
    move->set_call(cardsGame::NO_CALL);
    
    PlayMove(std::move(move));
}

int cardsGameClient::parse(std::string input, cardsGame::Move* move, int playerId)
{
    cardsGame::CardColor color;
    cardsGame::CardNumber number = cardsGame::INVALID_NUMBER;
    std::unique_ptr<cardsGame::Card> card = std::make_unique<cardsGame::Card>();
    cardsGame::Call call = cardsGame::NO_CALL;

    if (input.size() < 2 || input.size() > 4)
    {
        return -1;
    } 

    switch(std::toupper(input[0]))
    {
        case 'S' :
            color = cardsGame::SPADE;
            break;
        case 'D' :
            color = cardsGame::DENARI;
            break;
        case 'B' :
            color = cardsGame::BASTONI;
            break;
        case 'C' :
            color = cardsGame::COPPE;
            break;
        default:
            return -2;
    }

    for (size_t i = 1; i < input.size(); ++i) {
        if (std::isdigit(input[i]))
        {
            number = static_cast<cardsGame::CardNumber>(number * 10 + (input[i] - '0'));
            if(number > 10)
            {
                number = static_cast<cardsGame::CardNumber>(number - 3);
            }
        }
        else
        {
            switch(std::toupper(input[i]))
            {
                case 'B' :
                    call = cardsGame::BUSSO;
                    break;
                case 'S' :
                    call = cardsGame::STRISCIO;
                    break;
                case 'Q' :
                    call = cardsGame::CON_QUESTA_BASTA;
                    break;
                default:
                    call = cardsGame::NO_CALL;
                    break;
            }
            i = input.size(); // exit loop
        }
    }

    card->set_color(color);
    card->set_number(number);

    move->set_call(cardsGame::NO_CALL);
    move->set_allocated_card(card.release());

    return 0;
}

void cardsGameClient::parseInput(cardsGame::Move* move, int i)
{
    std::string input;

    do
    {
        std::cin >> input;
        getchar();
    }while(parse(input, move, i) != 0);
}
