#include "Server.h"
#include <cstddef>
#include <string>
#include <thread>
#include "GameSession.h"
#include "Logger.h"
#include "cardsGame.pb.h"


grpc::Status cardsGameServiceImpl::Connect(grpc::ServerContext* context, const cardsGame::ConnectReq* request, cardsGame::ConnectRsp* reply)
{
    if(request->name().empty()) {
        reply->set_fail_reason(cardsGame::ConnectRsp_ConnFailReason_EMPTY_NAME);
        return grpc::Status(grpc::INVALID_ARGUMENT, "Name cannot be empty");
    }
    else if(request->name().length() > 30) {
        reply->set_fail_reason(cardsGame::ConnectRsp_ConnFailReason_INVALID_NAME_SIZE);
        return grpc::Status(grpc::INVALID_ARGUMENT, "Name too long");
    }
    else if(ifClientExists(request->name())) {
        reply->set_fail_reason(cardsGame::ConnectRsp_ConnFailReason_NAME_ALREADY_TAKEN);
        return grpc::Status(grpc::ALREADY_EXISTS, "Name already taken");
    }

    // Successful connection

    int assignedId;
    {
        std::lock_guard<std::mutex> lock(mtx);
        assignedId = nextClientId++;
        clients[assignedId] = {assignedId, request->name(), nextSessionId}; // store client name and assigned ID
    }

    cardsGame::SuccessfullConn* successConn = new cardsGame::SuccessfullConn();

    successConn->set_playerid(assignedId);
    successConn->set_address("localhost:" + std::to_string(nextPort)); //TODO full address

    reply->set_allocated_successmsg(successConn);
    reply->set_message("Connected to server");

    LOG_DEBUG("Client connected: ID=", assignedId, ", name=", request->name(), " sessionId: ", nextSessionId);
    unallocatedClients++;

    maybeCreateGameSession();

    return grpc::Status::OK;
}

bool cardsGameServiceImpl::ifClientExists(const std::string& name) {
    std::lock_guard<std::mutex> lock(mtx);
    for (const auto& [id, client] : clients) {
        if (client.name == name) {
            return true;
        }
    }
    return false;
}

void cardsGameServiceImpl::RunListener() {
    std::string server_address(LISTENER_ADDRESS);
    cardsGameServiceImpl service;

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    server->Wait();
}

bool cardsGameServiceImpl::maybeCreateGameSession() {
    if(unallocatedClients == NUM_PLAYERS_IN_SESSION) {
        std::cout << "Creating new session" << std::endl;
        GameSession_NS::GameSession* newSession = new GameSession_NS::GameSession(nextPort, nextSessionId, 
            {clients[nextClientId-1].id, clients[nextClientId-2].id}, 
            1, cardsGame::GameType::BRISCOLA);
        gameSessions[nextSessionId] = newSession;

        std::thread([newSession, this]() {
            newSession->StartSession();  // assuming GameSession has a run() method
            // Optionally clean up after session ends
            std::cout << "Session ended, cleaning up." << std::endl;
            delete newSession;
            // If you want, remove it from map:
            gameSessions.erase(nextSessionId); 
            clients.erase(nextClientId-1);
            clients.erase(nextClientId-2);
        }).detach(); // detach so it runs independently
        nextSessionId++;
        unallocatedClients = 0;
        nextPort++;

        return true;
    }

    return false;
}

int main() {
    cardsGameServiceImpl server;
    server.RunListener();
    return 0;
}