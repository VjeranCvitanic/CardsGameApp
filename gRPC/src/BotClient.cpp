#include "BotClient.h"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <grpcpp/grpcpp.h>

namespace Bot_NS {

void BotClient::StartAsync() {
    std::thread([this]() { Run(); }).detach();
}

void BotClient::Run() {
    // 1. Connect
    cardsGame::ConnectReq req;
    req.set_name(name_);
    auto* fmt = req.add_gameformats();
    fmt->set_gametype(gameType_);
    fmt->set_singleormulti(singleOrMulti_);

    cardsGame::ConnectRsp rsp;
    grpc::ClientContext connectCtx;
    grpc::Status status = stub_->Connect(&connectCtx, req, &rsp);
    if (!status.ok() || !rsp.has_successmsg()) {
        std::cerr << "[Bot:" << name_ << "] Connect failed: " << status.error_message() << std::endl;
        return;
    }
    playerId_  = rsp.successmsg().playerid();
    sessionId_ = rsp.successmsg().sessionid();
    std::cout << "[Bot:" << name_ << "] Connected with playerId=" << playerId_
              << ", sessionId=" << sessionId_ << std::endl;

    // 2. Subscribe with reconnect loop
    auto sessionStub = cardsGame::CardsGameSession::NewStub(channel_);
    bool matchOver = false;

    while (!matchOver) {
        grpc::ClientContext subCtx;
        subCtx.set_deadline(std::chrono::system_clock::now() + std::chrono::minutes(30));
        subCtx.set_wait_for_ready(true);

        cardsGame::PlayerInfo info;
        info.set_playerid(playerId_);
        info.set_sessionid(sessionId_);
        auto reader = sessionStub->SubscribeEvents(&subCtx, info);

        bool isReplay = false;
        cardsGame::GameEventMsg event;
        while (reader->Read(&event)) {
            if (event.eventtype() == cardsGame::RECONNECT_START_EVENT) {
                isReplay = true;
                std::cout << "[Bot:" << name_ << "] Reconnect replay started" << std::endl;
                continue;
            }
            if (event.eventtype() == cardsGame::RECONNECT_END_EVENT) {
                isReplay = false;
                std::cout << "[Bot:" << name_ << "] Reconnect replay ended" << std::endl;
                continue;
            }
            if (event.eventtype() == cardsGame::START_MATCH_EVENT) {
                std::cout << "[Bot:" << name_ << "] Match started: sessionId=" << sessionId_
                          << " localId=" << event.playerinfo().playerid() << std::endl;
            } else if (event.eventtype() == cardsGame::MATCH_OVER_EVENT) {
                matchOver = true;
            } else if (event.eventtype() == cardsGame::YOUR_TURN_EVENT && !isReplay) {
                PlayMove(playerId_, sessionId_, event.yourturn(), sessionStub.get());
            }
        }

        grpc::Status finish = reader->Finish();
        std::cout << "[Bot:" << name_ << "] Stream finished: "
                  << (finish.ok() ? "OK" : finish.error_message()) << std::endl;

        if (!matchOver) {
            std::cout << "[Bot:" << name_ << "] Disconnected, retrying in 2s..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(2));
            // Need a new stub for the new context
            sessionStub = cardsGame::CardsGameSession::NewStub(channel_);
        }
    }
}

void BotClient::PlayMove(int sessionPlayerId, int sessionId,
                          const cardsGame::YourTurnMsg& turn,
                          cardsGame::CardsGameSession::Stub* sessionStub) {
    // Pick a random legal card
    const auto& legal = turn.legalcards();
    if (legal.empty()) return;

    int idx = std::rand() % legal.size();
    const auto& chosen = legal[idx];

    cardsGame::PlayMoveReq req;
    req.mutable_playerinfo()->set_playerid(sessionPlayerId);
    req.mutable_playerinfo()->set_sessionid(sessionId);
    req.mutable_move()->mutable_card()->set_color(chosen.color());
    req.mutable_move()->mutable_card()->set_number(chosen.number());
    req.mutable_move()->set_call(cardsGame::NO_CALL);

    google::protobuf::Empty response;
    grpc::ClientContext ctx;
    grpc::Status s = sessionStub->PlayMove(&ctx, req, &response);
    if (!s.ok()) {
        std::cerr << "[Bot:" << name_ << "] PlayMove failed: " << s.error_message() << std::endl;
    }
}

} // namespace Bot_NS
