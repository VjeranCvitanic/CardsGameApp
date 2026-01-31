#include <grpcpp/grpcpp.h>
#include "ProtoToDomain.h"
#include "cardsGame.grpc.pb.h"
#include "cardsGame.pb.h"
#include <mutex>
#include <iostream>
#include <thread>
#include <chrono>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using cardsGame::CardsGameServer;
using cardsGame::ConnectReq;
using cardsGame::ConnectRsp;

class cardsGameServiceImpl final : public CardsGameServer::Service {
    std::unordered_map<int, std::string> clients;
    std::unordered_map<std::string, grpc::ServerReaderWriter<cardsGame::GameEventMsg, cardsGame::GameEventMsg>*> clientsStreams;
    std::mutex mtx;
    int nextClientId = 1;
public:
    Status Connect(ServerContext* context, const ConnectReq* request, ConnectRsp* reply) override {
        if(request->name().empty()) {
            reply->set_fail_reason(cardsGame::ConnectRsp_ConnFailReason_EMPTY_NAME);
            return Status(grpc::INVALID_ARGUMENT, "Name cannot be empty");
        }
        else if(request->name().length() > 30) {
            reply->set_fail_reason(cardsGame::ConnectRsp_ConnFailReason_INVALID_NAME_SIZE);
            return Status(grpc::INVALID_ARGUMENT, "Name too long");
        }
        else if(ifClientExists(request->name())) {
            reply->set_fail_reason(cardsGame::ConnectRsp_ConnFailReason_NAME_ALREADY_TAKEN);
            return Status(grpc::ALREADY_EXISTS, "Name already taken");
        }

        // Successful connection
        int assignedId;
        {
            std::lock_guard<std::mutex> lock(mtx);
            assignedId = nextClientId++;
            clients[assignedId] = request->name(); // store client name and assigned ID
            clientsStreams[request->name()] = nullptr; // placeholder for stream
        }
        reply->set_success_id(assignedId);
        reply->set_message("Connected to server");

        std::cout << "Client connected: ID=" << assignedId 
                  << ", name=" << request->name() << std::endl;

        return Status::OK;
    }

    grpc::Status GameEventStream(
        const cardsGame::GameEventMsg* request,
        grpc::ServerReaderWriter<cardsGame::GameEventMsg, cardsGame::GameEventMsg>* readWriter)
    {
        const std::string& clientName = clients.at(request->playerid()); // or some identifier

        {
            std::lock_guard<std::mutex> lock(mtx);
            clientsStreams[clientName] = readWriter; // store readerWriter for this client
        }

        // Optionally, block here if you want to push messages as they come:
        while (true) {
            // Wait for events for this client
            // writer->Write(eventProto);
        }

        return grpc::Status::OK;
    }

    Status PlayMove(ServerContext* context, const cardsGame::PlayMoveReq* request, cardsGame::PlayMoveRsp* response) override {
        std::string playerName = request->playerid();
        {
            if (!ifClientExists(playerName)) {
                response->set_moversp(cardsGame::INVALID_PLAYER_NAME);
                return Status(grpc::NOT_FOUND, "Player ID not found");
            }
        }

        Move move;
        PlayMoveReqToDomain(*request, move);

        response->set_moversp(cardsGame::MOVE_OK);

        return Status::OK;
    }

    void PrintClients() {
        std::lock_guard<std::mutex> lock(mtx);
        std::cout << "Currently connected clients:" << std::endl;
        for (auto& [id, name] : clients) {
            std::cout << " - ID=" << id << ", name=" << name << std::endl;
        }
    }

    bool ifClientExists(const std::string& name) {
        std::lock_guard<std::mutex> lock(mtx);
        for (const auto& [id, n] : clients) {
            if (n == name) {
                return true;
            }
        }
        return false;
    }
};

void RunServer() {
    std::string server_address("0.0.0.0:50051");
    cardsGameServiceImpl service;

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    std::thread([&service]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(10));
            service.PrintClients();
        }
    }).detach();

    server->Wait();
}

int main() {
    RunServer();
    return 0;
}
