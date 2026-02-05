#include "GameSession.h"
#include "Cards.h"
#include "Events.h"
#include "ProtoToDomain.h"
#include "Logger.h"
#include "Types.h"
#include "cardsGame.pb.h"
#include <iostream>
#include <memory>
#include <thread>
#include <unistd.h>


void GameSession_NS::GameSession::StartSession() {
    std::cout << "Game session started!" << std::endl;
    isStarted = true;

    eventEmitter = std::make_unique<EventEmitter>();
    eventEmitter->subscribe(&Logger::GetInstance());
    eventEmitter->subscribe(this);

    std::string server_address("0.0.0.0:" + std::to_string(port));

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(this);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Session running on " << server_address << std::endl;

    std::thread waitThread([this]() {
        while(players.size() < 2) {
            std::cout << "Waiting for players to join..." << std::endl;
            sleep(1);
        }
        std::cout << "Both players joined!" << std::endl;
        startMatch();
    });

    waitThread.detach();

    server->Wait();
}

void GameSession_NS::GameSession::dealCards(const PlayerDealtCardsEvent& event)
{
    std::cout << "player: " << event.playerId << " dealt cards: ";
    for (const auto& card : event.cards) {
        std::cout << "[" << Cards::CardToString(card) << "] ";
    }
    std::cout << std::endl;

    cardsGame::PlayerDealtCardsMsg* dealMsg = new cardsGame::PlayerDealtCardsMsg();
    dealMsg->set_playerid(event.playerId.first);
    for (const auto& card : event.cards) {
        cardsGame::Card* c = dealMsg->add_card();
        c->set_color(static_cast<cardsGame::CardColor>(Cards::getColor(card)));
        c->set_number(static_cast<cardsGame::CardNumber>(Cards::getNumber(card)));
    }

    cardsGame::GameEventMsg dealEvent;
    dealEvent.set_eventtype(cardsGame::EventType::PLAYER_DEALT_CARDS_EVENT);
    dealEvent.set_playerid(event.playerId.first);
    dealEvent.set_allocated_playerdealtcards(dealMsg);
    std::cout << "before write\n" << std::endl;
    {
    std::lock_guard<std::mutex> lock(connectionsMutex);

    auto it = connections.find(event.playerId.first);
    if (it != connections.end()) {
        it->second->send(dealEvent);
    } else {
        std::cerr << "No connection for player " << event.playerId.first << std::endl;
    }
    }
    std::cout << "after write\n" << std::endl;
}

void GameSession_NS::GameSession::startRound(const StartRoundEvent& event)
{
    cardsGame::StartRoundMsg* roundMsg = new cardsGame::StartRoundMsg();
    roundMsg->set_firsttoplayid(event.firstToPlayId.second);

    cardsGame::GameEventMsg roundEvent;
    roundEvent.set_eventtype(cardsGame::EventType::START_ROUND_EVENT);
    roundEvent.set_allocated_startround(roundMsg);

    {
    std::lock_guard<std::mutex> lock(connectionsMutex);

    for (const auto& [playerId, connection] : connections) {
        roundEvent.set_playerid(playerId);
        connection->send(roundEvent);
    }
    }
}

void GameSession_NS::GameSession::startGame(const StartGameEvent& event)
{
    cardsGame::StartGameMsg* gameMsg = new cardsGame::StartGameMsg();
    gameMsg->set_gametype(cardsGame::BRISCOLA); // TODO set correct game type
    gameMsg->set_firsttoplayid(event.firstToPlayId.second);

    cardsGame::GameEventMsg gameEvent;
    gameEvent.set_eventtype(cardsGame::EventType::START_GAME_EVENT);
    gameEvent.set_allocated_startgame(gameMsg);

    {
        std::lock_guard<std::mutex> lock(connectionsMutex);

        for (const auto& [playerId, connection] : connections) {
            gameEvent.set_playerid(playerId);
            connection->send(gameEvent);
        }
    }
}

void GameSession_NS::GameSession::startBriscolaGame(const StartBriscolaGameEvent& event)
{
    cardsGame::StartGameMsg* gameMsg = new cardsGame::StartGameMsg();
    gameMsg->set_gametype(cardsGame::BRISCOLA);
    gameMsg->set_firsttoplayid(event.firstToPlayId.second);
    gameMsg->set_allocated_lastcard(new cardsGame::Card());
    gameMsg->mutable_lastcard()->set_color(static_cast<cardsGame::CardColor>(Cards::getColor(event.lastCard)));
    gameMsg->mutable_lastcard()->set_number(static_cast<cardsGame::CardNumber>(Cards::getNumber(event.lastCard)));

    cardsGame::GameEventMsg gameEvent;
    gameEvent.set_eventtype(cardsGame::EventType::START_GAME_EVENT);
    gameEvent.set_allocated_startgame(gameMsg);

    {
        std::lock_guard<std::mutex> lock(connectionsMutex);

        for (const auto& [playerId, connection] : connections) {
            gameEvent.set_playerid(playerId);
            connection->send(gameEvent);
        }
    }
}

void GameSession_NS::GameSession::startMatch(const StartMatchEvent& event)
{
    cardsGame::StartMatchMsg* matchMsg = new cardsGame::StartMatchMsg();
    matchMsg->set_gametype(static_cast<cardsGame::GameType>(event.gameType));
    matchMsg->set_firsttoplayid(event.firstToPlayId.second);
    matchMsg->set_teammateid(-1); // TODO set teammate id if applicable

    cardsGame::GameEventMsg gameEvent;
    gameEvent.set_eventtype(cardsGame::EventType::START_MATCH_EVENT);
    gameEvent.set_allocated_startmatch(matchMsg);

    {
        std::lock_guard<std::mutex> lock(connectionsMutex);

        for (const auto& [playerId, connection] : connections) {
            gameEvent.set_playerid(playerId);
            connection->send(gameEvent);
        }
    }
}

void GameSession_NS::GameSession::yourTurn(const YourTurnEvent& event)
{
    cardsGame::YourTurnMsg* turnMsg = new cardsGame::YourTurnMsg();
    turnMsg->set_playerid(event.playerId.second);
    for (const auto& card : event.yourHand) {
        cardsGame::Card* c = turnMsg->add_hand();
        c->set_color(static_cast<cardsGame::CardColor>(Cards::getColor(card)));
        c->set_number(static_cast<cardsGame::CardNumber>(Cards::getNumber(card)));
    }
    turnMsg->set_strongcolor(static_cast<cardsGame::CardColor>(event.strongColor));
    // TODO add moves played in round
    for (const auto& move : event.movesPlayedInRound) {
        cardsGame::Card* c = turnMsg->add_cardsplayedinround();
        c->set_color(static_cast<cardsGame::CardColor>(Cards::getColor(move.card)));
        c->set_number(static_cast<cardsGame::CardNumber>(Cards::getNumber(move.card)));
    }

    cardsGame::GameEventMsg turnEvent;
    turnEvent.set_eventtype(cardsGame::EventType::YOUR_TURN_EVENT);
    turnEvent.set_playerid(event.playerId.first);
    turnEvent.set_allocated_yourturn(turnMsg);

    {
        std::lock_guard<std::mutex> lock(connectionsMutex);

        auto it = connections.find(event.playerId.first);
        if (it != connections.end()) {
            it->second->send(turnEvent);
        } else {
            std::cerr << "No connection for player " << event.playerId.first << std::endl;
        }
    }
}

void GameSession_NS::GameSession::playerPlayedMoveEvent(const PlayerPlayedMoveEvent& event)
{
    cardsGame::PlayerPlayedMoveMsg* msg = new cardsGame::PlayerPlayedMoveMsg();

    cardsGame::Move* move = new cardsGame::Move();
    cardsGame::Card* card = new cardsGame::Card();
    card->set_color(static_cast<cardsGame::CardColor>(Cards::getColor(event.move.card)));
    card->set_number(static_cast<cardsGame::CardNumber>(Cards::getNumber(event.move.card)));
    move->set_allocated_card(card);
    move->set_call(static_cast<cardsGame::Call>(event.move.call));
    
    msg->set_allocated_move(move);
    msg->set_playerid(event.move.playerId.second);

    cardsGame::GameEventMsg gameEvent;
    gameEvent.set_eventtype(cardsGame::EventType::PLAYER_PLAYED_MOVE_EVENT);
    gameEvent.set_allocated_playerplayedmove(msg);

    {
        std::lock_guard<std::mutex> lock(connectionsMutex);

        for (const auto& [playerId, connection] : connections) {
            gameEvent.set_playerid(playerId);
            connection->send(gameEvent);
        }
    }
}




void GameSession_NS::GameSession::onEvent(const PlayerPlayedMoveEvent& event)
{
    std::cout << "PlayerPlayedMoveEvent received\n" << std::endl;
    playerPlayedMoveEvent(event);
}

void GameSession_NS::GameSession::onEvent(
    const PlayerDealtCardsEvent& event)
{
    std::cout << "PlayerDealtCardsEvent received\n" << std::endl;
    dealCards(event);
}

void GameSession_NS::GameSession::onEvent(
    const StartRoundEvent& event)
{
    std::cout << "StartRoundEvent received\n" << std::endl;
    startRound(event);
}

void GameSession_NS::GameSession::onEvent(
    const TressetteDealtCardsEvent& event)
{
    std::cout << "TressetteDealtCardsEvent received\n" << std::endl;
}

void GameSession_NS::GameSession::onEvent(
    const StartGameEvent& event)
{
    std::cout << "StartGameEvent received\n" << std::endl;
    startGame(event);
}

void GameSession_NS::GameSession::onEvent(
    const StartBriscolaGameEvent& event)
{
    std::cout << "StartBriscolaGameEvent received\n" << std::endl;
    startBriscolaGame(event);
}

void GameSession_NS::GameSession::onEvent(
    const StartMatchEvent& event)
{
    std::cout << "StartMatchEvent received\n" << std::endl;
    startMatch(event);
}

void GameSession_NS::GameSession::onEvent(
    const RoundOverEvent& event)
{
    std::cout << "RoundOverEvent received\n" << std::endl;
}

void GameSession_NS::GameSession::onEvent(
    const GameOverEvent& event)
{
    std::cout << "Game over!\n" << std::endl;
}

void GameSession_NS::GameSession::onEvent(
    const MatchOverEvent& event)
{
    std::cout << "MatchOverEvent received\n" << std::endl;
}

void GameSession_NS::GameSession::onEvent(
    const YourTurnEvent& event)
{
    std::cout << "YourTurnEvent received\n" << std::endl;
    yourTurn(event);
}

void GameSession_NS::GameSession::onEvent(
    const MoveResponseEvent& event)
{
    std::cout << "MoveResponseEvent received\n" << std::endl;
}

void GameSession_NS::GameSession::onEvent(
    const AcussoEvent& event)
{
    std::cout << "AcussoEvent received\n" << std::endl;
}

void GameSession_NS::GameSession::onEvent(
    const BriscolaLastRoundEvent& event)
{
    std::cout << "BriscolaLastRoundEvent received\n" << std::endl;
}

void GameSession_NS::GameSession::onEvent(
    const BeforeFirstMoveEvent& event)
{
    std::cout << "BeforeFirstMoveEvent received\n" << std::endl;
}

void GameSession_NS::GameSession::onEvent(const GameEvent& event) {
    std::visit([this](auto&& e) { 
        this->onEvent(e);   // overload resolution happens here
    }, event);
}



void GameSession_NS::GameSession::startMatch() {
    switch(gameType) {
        case cardsGame::GameType::BRISCOLA:
            match = createBriscolaMatch(*eventEmitter, players.size());
            break;
        case cardsGame::GameType::TRESSETTE:
            match = createTressetteMatch(*eventEmitter, players.size());
            break;
        default:
            std::cerr << "Unknown game type!" << std::endl;
            return;
    }
}

void GameSession_NS::GameSession::ApplyMove(const Move& move) {
    match->ApplyMove(move);
    if(match->IsFinished())
    {
        cntMatchesPlayed++;
        int winningTeam = match->matchResult.winnerId;
        teamWins[winningTeam]++;
        std::cout << "Match finished! Winning team: " << winningTeam << std::endl;
    }
}

bool GameSession_NS::GameSession::IsSessionOver() const {
    return cntMatchesPlayed >= numMatchesToPlay;
}

int GameSession_NS::GameSession::AddPlayer(int playerId) {
    int sessionId = players.size();
    players[sessionId] = playerId;
    return sessionId++;
}

void GameSession_NS::GameSession::PrintResults() {
    std::cout << "Game session results:" << std::endl;
    std::cout << "Team 1 wins: " << teamWins[0] << std::endl;
    std::cout << "Team 2 wins: " << teamWins[1] << std::endl;
}

grpc::
Status GameSession_NS::GameSession::PlayMove(grpc::ServerContext* context, const cardsGame::PlayMoveReq* request, cardsGame::PlayMoveRsp* response) {
    Move move;
    PlayMoveReqToDomain(*request, move);

    ApplyMove(move);

    //response->set_moversp(cardsGame::MOVE_OK);

    return grpc::Status::OK;
}

grpc::Status GameSession_NS::GameSession::SubscribeEvents(grpc::ServerContext* context, 
                                const cardsGame::PlayerInfo* request,
                                grpc::ServerWriter<cardsGame::GameEventMsg>* writer)
{
    std::cout << "SubscribeEvents called for player id: " << request->playerid() << std::endl;
    int playerId = request->playerid();

    AddPlayer(playerId);

    cardsGame::GameEventMsg initialEvent;
    cardsGame::StartMatchMsg* startMatch = new cardsGame::StartMatchMsg();
    startMatch->set_firsttoplayid(players.begin()->second); // first player to play
    startMatch->set_gametype(gameType);
    startMatch->set_teammateid(-1); // TODO set teammate id if applicable


    initialEvent.set_eventtype(cardsGame::EventType::START_MATCH_EVENT);
    initialEvent.set_playerid(playerId);
    initialEvent.set_allocated_startmatch(startMatch);

    writer->Write(initialEvent); // send initial message

    {
        std::lock_guard<std::mutex> lock(connectionsMutex);
        connections[playerId] = std::make_shared<PlayerConnection>(writer);
    }

    while (!context->IsCancelled() && !IsSessionOver()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }


    return grpc::Status::OK;    
}


