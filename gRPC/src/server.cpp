#include "Server.h"
#include <cstddef>
#include "ProtoToDomain.h"


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
        clients[assignedId] = {assignedId, request->name(), 0, .stream = nullptr}; // store client name and assigned ID
    }
    reply->set_success_id(assignedId);
    reply->set_message("Connected to server");

    std::cout << "Client connected: ID=" << assignedId 
                << ", name=" << request->name() << std::endl;

    return grpc::Status::OK;
}

grpc::Status cardsGameServiceImpl::GameEventStream(
    const cardsGame::GameEventMsg* request,
    grpc::ServerReaderWriter<cardsGame::GameEventMsg, cardsGame::GameEventMsg>* readWriter)
{
    const std::string& clientName = clients.at(request->playerid()).name; // or some identifier

    {
        std::lock_guard<std::mutex> lock(mtx);
        clients.at(request->playerid()).stream = readWriter; // store readerWriter for this client
    }

    // Optionally, block here if you want to push messages as they come:
    while (true) {
        // Wait for events for this client
        // writer->Write(eventProto);
    }

    return grpc::Status::OK;
}
grpc::
Status cardsGameServiceImpl::PlayMove(grpc::ServerContext* context, const cardsGame::PlayMoveReq* request, cardsGame::PlayMoveRsp* response) {
    std::string playerName = request->playerid();
    {
        if (!ifClientExists(playerName)) {
            response->set_moversp(cardsGame::INVALID_PLAYER_NAME);
            return grpc::Status(grpc::NOT_FOUND, "Player ID not found");
        }
    }

    Move move;
    PlayMoveReqToDomain(*request, move);
    

    response->set_moversp(cardsGame::MOVE_OK);

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

void cardsGameServiceImpl::RunServer() {
    std::string server_address("0.0.0.0:50051");
    cardsGameServiceImpl service;

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    server->Wait();
}

