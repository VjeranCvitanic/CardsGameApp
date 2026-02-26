#include "Server.h"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <random>
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
    assignedId = nextClientId++;

    // players supported gameFormats
    std::vector<GameFormat> gameFormats;
    gameFormats.reserve(request->gameformats_size());

    for (auto& gameForm : request->gameformats()) {
        gameFormats.push_back({gameForm.gametype(), gameForm.singleormulti()});
    }

    // store client name and assigned ID
    clients.emplace(assignedId, Client(assignedId, request->name(), gameFormats, nextSessionId));

    cardsGame::SuccessfullConn successConn;

    successConn.set_playerid(assignedId);
    successConn.set_address("localhost:" + std::to_string(nextPort));

    *reply->mutable_successmsg() = successConn;
    reply->set_message("Connected to server");

    std::cout << "Client connected: ID=" << assignedId << ", name=" << request->name() << " sessionId: " << nextSessionId << std::endl;
    addClientToWaitingLists(&clients.at(assignedId));

    for(auto& gf : clients.at(assignedId).gameFormats) {
        if(maybeCreateGameSession(gf))
            break; // if a game session is started break from loop
    }

    return grpc::Status::OK;
}

void cardsGameServiceImpl::addClientToWaitingLists(Client* c)
{
    for(auto& gf : c->gameFormats) {
        waitingClients[gf.gameType][gf.SingleOrMulti].push_back(c);
    }
}

void cardsGameServiceImpl::removeClientFromWaitingLists(Client* c) {
    for(auto& gf : c->gameFormats) {
        waitingClients[gf.gameType][gf.SingleOrMulti].erase(std::remove(waitingClients[gf.gameType][gf.SingleOrMulti].begin(), waitingClients[gf.gameType][gf.SingleOrMulti].end(), c), waitingClients[gf.gameType][gf.SingleOrMulti].end());
    }
}

bool cardsGameServiceImpl::ifClientExists(const std::string& name) {
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

bool cardsGameServiceImpl::maybeCreateGameSession(GameFormat format) {
    int numPlayersInSession = (format.SingleOrMulti == cardsGame::SINGLE) ? 2 : 4;
    std::cout << "Checking session creation for gameType=" << format.gameType 
              << ", SingleOrMulti=" << format.SingleOrMulti 
              << ", waiting=" << waitingClients[format.gameType][format.SingleOrMulti].size() 
              << ", needed=" << numPlayersInSession << std::endl;
    if(waitingClients[format.gameType][format.SingleOrMulti].size() == numPlayersInSession) {
        std::cout << "Creating new session" << std::endl;
        std::vector<int> clientsIds = std::vector<int>(numPlayersInSession);
        for(int i = 0; i < numPlayersInSession; i++) {
            clientsIds[i] = waitingClients[format.gameType][format.SingleOrMulti][i]->id;
        }
        
        // Randomize player order - teams are determined by session ID parity
        std::random_device rd;
        std::mt19937 gen(rd());
        std::shuffle(clientsIds.begin(), clientsIds.end(), gen);
        
        std::cout << "Randomized player order: ";
        for(int id : clientsIds) {
            std::cout << id << " ";
        }
        std::cout << std::endl;
        
        auto newSession = std::make_shared<GameSession_NS::GameSession>(nextPort, nextSessionId, 
            clientsIds, 
            1, format.gameType, numPlayersInSession);
        gameSessions[nextSessionId] = newSession;

        std::thread([newSession, numPlayersInSession, clientsIds, sessionId = nextSessionId, this]() {
            newSession->StartSession();
            std::cout << "Session ended, cleaning up." << std::endl;
            {
                std::lock_guard<std::mutex> lock(sessionMutex);
                gameSessions.erase(sessionId);
                for(int i = 0; i < numPlayersInSession; i++) {
                    clients.erase(clientsIds[i]);
                }
            }
        }).detach(); // detach so it runs independently*/
        nextSessionId++;
        for(auto c : waitingClients[format.gameType][format.SingleOrMulti]) {
            removeClientFromWaitingLists(c);
        }

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