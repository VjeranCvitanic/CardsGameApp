#include "Server.h"
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <thread>
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
    successConn.set_sessionid(nextSessionId); // client uses this to route session RPCs

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
    builder.RegisterService(static_cast<cardsGame::CardsGameServer::Service*>(&service));
    builder.RegisterService(static_cast<cardsGame::CardsGameSession::Service*>(&service.dispatcher_));

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
        
        auto newSession = std::make_shared<GameSession_NS::GameSession>(nextSessionId,
            clientsIds,
            1, format.gameType, numPlayersInSession);
        gameSessions[nextSessionId] = newSession;
        dispatcher_.registerSession(nextSessionId, newSession.get());

        std::thread([newSession, numPlayersInSession, clientsIds, sessionId = nextSessionId, this]() {
            newSession->StartSession();

            // Store match results
            auto result = newSession->GetResult();
            {
                std::lock_guard<std::mutex> lock(resultsMutex_);
                matchResults_.push_back({result.sessionId, result.winnerTeamId,
                                         {result.teamWins[0], result.teamWins[1]}});
                std::cout << "[Results] Session " << result.sessionId
                          << ": winner=team" << result.winnerTeamId
                          << " (" << result.teamWins[0] << "-" << result.teamWins[1] << ")" << std::endl;
            }

            // Cleanup
            {
                std::lock_guard<std::mutex> lock(sessionMutex);
                dispatcher_.unregisterSession(sessionId);
                gameSessions.erase(sessionId);
                for(int i = 0; i < numPlayersInSession; i++) {
                    clients.erase(clientsIds[i]);
                }
            }
            std::cout << "Session " << sessionId << " cleaned up." << std::endl;
        }).detach();
        nextSessionId++;
        for(auto c : waitingClients[format.gameType][format.SingleOrMulti]) {
            removeClientFromWaitingLists(c);
        }

        return true;
    }

    return false;
}

// ============================================================
// SessionDispatcher implementation
// ============================================================

void SessionDispatcher::registerSession(int sessionId, GameSession_NS::GameSession* session) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        sessions_[sessionId] = session;
    }
    cv_.notify_all();
    std::cout << "[Dispatcher] Registered session " << sessionId << std::endl;
}

void SessionDispatcher::unregisterSession(int sessionId) {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.erase(sessionId);
    std::cout << "[Dispatcher] Unregistered session " << sessionId << std::endl;
}

GameSession_NS::GameSession* SessionDispatcher::findSession(int sessionId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(sessionId);
    return (it != sessions_.end()) ? it->second : nullptr;
}

GameSession_NS::GameSession* SessionDispatcher::waitForSession(int sessionId, int timeoutSeconds) {
    std::unique_lock<std::mutex> lock(mutex_);
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(timeoutSeconds);
    bool found = cv_.wait_until(lock, deadline, [&]() {
        return sessions_.find(sessionId) != sessions_.end();
    });
    if (!found) return nullptr;
    return sessions_[sessionId];
}

grpc::Status SessionDispatcher::SubscribeEvents(
    grpc::ServerContext* context,
    const cardsGame::PlayerInfo* request,
    grpc::ServerWriter<cardsGame::GameEventMsg>* writer)
{
    if (!request->has_sessionid()) {
        return grpc::Status(grpc::INVALID_ARGUMENT, "session_id is required");
    }
    // Wait up to 120s for the session to be created (other players still connecting)
    GameSession_NS::GameSession* session = waitForSession(request->sessionid(), 120);
    if (!session) {
        return grpc::Status(grpc::NOT_FOUND, "Session not found (timed out): " + std::to_string(request->sessionid()));
    }
    return session->SubscribeEvents(context, request, writer);
}

grpc::Status SessionDispatcher::PlayMove(
    grpc::ServerContext* context,
    const cardsGame::PlayMoveReq* request,
    google::protobuf::Empty* response)
{
    if (!request->playerinfo().has_sessionid()) {
        return grpc::Status(grpc::INVALID_ARGUMENT, "session_id is required");
    }
    GameSession_NS::GameSession* session = findSession(request->playerinfo().sessionid());
    if (!session) {
        return grpc::Status(grpc::NOT_FOUND, "Session not found: " + std::to_string(request->playerinfo().sessionid()));
    }
    return session->PlayMove(context, request, response);
}

grpc::Status SessionDispatcher::SpectateSession(
    grpc::ServerContext* context,
    const cardsGame::SpectateReq* request,
    grpc::ServerWriter<cardsGame::GameEventMsg>* writer)
{
    GameSession_NS::GameSession* session = findSession(request->sessionid());
    if (!session) {
        return grpc::Status(grpc::NOT_FOUND, "Session not found: " + std::to_string(request->sessionid()));
    }
    return session->SpectateSession(context, request, writer);
}

int main() {
    cardsGameServiceImpl server;
    server.RunListener();
    return 0;
}