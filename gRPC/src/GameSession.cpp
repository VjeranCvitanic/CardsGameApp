#include "GameSession.h"
#include "Events.h"
#include "ProtoToDomain.h"
#include "Logger.h"
#include "cardsGame.pb.h"
#include <iostream>


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

    startMatch();

    server->Wait();
}

void GameSession_NS::GameSession::onEvent(const PlayerPlayedMoveEvent& event)
{
    std::cout << "PlayerPlayedMoveEvent received\n" << std::endl;
}

void GameSession_NS::GameSession::onEvent(
    const PlayerDealtCardsEvent& event)
{
    std::cout << "PlayerDealtCardsEvent received\n" << std::endl;
}

void GameSession_NS::GameSession::onEvent(
    const StartRoundEvent& event)
{
    std::cout << "StartRoundEvent received\n" << std::endl;
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
}

void GameSession_NS::GameSession::onEvent(
    const StartBriscolaGameEvent& event)
{
    std::cout << "StartBriscolaGameEvent received\n" << std::endl;
}

void GameSession_NS::GameSession::onEvent(
    const StartMatchEvent& event)
{
    std::cout << "StartMatchEvent received\n" << std::endl;
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
    return sessionId;
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

    response->set_moversp(cardsGame::MOVE_OK);

    return grpc::Status::OK;
}

grpc::Status GameSession_NS::GameSession::SubscribeEvents(grpc::ServerContext* context, 
                                const cardsGame::PlayerInfo* request,
                                grpc::ServerWriter<cardsGame::GameEventMsg>* writer)
{
    int playerId = request->playerid();

    cardsGame::GameEventMsg initialEvent;
    cardsGame::StartMatchMsg* startMatch = new cardsGame::StartMatchMsg();
    startMatch->set_firsttoplayid(players.begin()->second); // first player to play
    startMatch->set_gametype(gameType);
    startMatch->set_teammateid(-1); // TODO set teammate id if applicable


    initialEvent.set_eventtype(cardsGame::EventType::START_MATCH_EVENT);
    initialEvent.set_playerid(playerId);
    initialEvent.set_allocated_startmatch(startMatch);

    writer->Write(initialEvent); // send initial message

    return grpc::Status::OK;    
}
        