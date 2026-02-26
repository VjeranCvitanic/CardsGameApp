#include "GameSession.h"
#include "Cards.h"
#include "Events.h"
#include "Logger.h"
#include "Types.h"
#include "cardsGame.pb.h"
#include <iostream>
#include <memory>
#include <thread>

void GameSession_NS::GameSession::StartSession()
 {
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
        size_t expectedPlayers = players.size();
        while(true) {
            {
                std::lock_guard<std::mutex> lock(connectionsMutex);
                if(connections.size() >= expectedPlayers) {
                    break;
                }
            }
            std::cout << "Waiting for players to join..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        std::cout << "All players joined!" << std::endl;
        startMatch();
    });

    waitThread.detach();

    server->Wait();
}

void GameSession_NS::GameSession::dealCards(const PlayerDealtCardsEvent& event)
{
    cardsGame::PlayerDealtCardsMsg dealMsg;
    dealMsg.mutable_playerinfo()->set_playerid(event.playerId.second);
    for (const auto& card : event.cards) {
        cardsGame::Card* c = dealMsg.add_card();
        c->set_color(static_cast<cardsGame::CardColor>(Cards::getColor(card)));
        c->set_number(static_cast<cardsGame::CardNumber>(Cards::getNumber(card)));
    }

    cardsGame::GameEventMsg dealEvent;
    dealEvent.set_eventtype(cardsGame::EventType::PLAYER_DEALT_CARDS_EVENT);
    dealEvent.mutable_playerinfo()->set_playerid(event.playerId.second);
    *dealEvent.mutable_playerdealtcards() = dealMsg;
    {
        std::lock_guard<std::mutex> lock(connectionsMutex);

        auto it = connections.find(event.playerId.second);
        if (it != connections.end()) {
            it->second->send(dealEvent);
        } else {
            std::cerr << "No connection for player " << event.playerId.second << std::endl;
        }
    }
}

void GameSession_NS::GameSession::startRound(const StartRoundEvent& event)
{
    cardsGame::StartRoundMsg roundMsg;
    roundMsg.mutable_firsttoplayid()->set_playerid(event.firstToPlayId.second);

    cardsGame::GameEventMsg roundEvent;
    roundEvent.set_eventtype(cardsGame::EventType::START_ROUND_EVENT);
    *roundEvent.mutable_startround() = roundMsg;

    {
        std::lock_guard<std::mutex> lock(connectionsMutex);

        for (const auto& [playerId, connection] : connections) {
            roundEvent.mutable_playerinfo()->set_playerid(playerId);
            connection->send(roundEvent);
        }
    }
}

void GameSession_NS::GameSession::startGame(const StartGameEvent& event)
{
    cardsGame::StartGameMsg gameMsg;
    cardsGame::GameFormat* gameFormat = gameMsg.mutable_gameformat();
    gameFormat->set_gametype(gameType);
    cardsGame::SingleOrMulti sm = (players.size() == 2) ? cardsGame::SingleOrMulti::SINGLE : cardsGame::SingleOrMulti::MULTI;
    gameFormat->set_singleormulti(sm);
    gameMsg.mutable_firsttoplayid()->set_playerid(event.firstToPlayId.second);

    cardsGame::GameEventMsg gameEvent;
    gameEvent.set_eventtype(cardsGame::EventType::START_GAME_EVENT);
    *gameEvent.mutable_startgame() = gameMsg;

    {
        std::lock_guard<std::mutex> lock(connectionsMutex);

        for (const auto& [playerId, connection] : connections) {
            gameEvent.mutable_playerinfo()->set_playerid(playerId);
            connection->send(gameEvent);
        }
    }
}

void GameSession_NS::GameSession::startBriscolaGame(const StartBriscolaGameEvent& event)
{
    cardsGame::StartGameMsg gameMsg;
    auto* gameFormat = gameMsg.mutable_gameformat();
    gameFormat->set_gametype(cardsGame::BRISCOLA);
    cardsGame::SingleOrMulti sm = (players.size() == 2) ? cardsGame::SingleOrMulti::SINGLE : cardsGame::SingleOrMulti::MULTI;
    gameFormat->set_singleormulti(sm);
    gameMsg.mutable_firsttoplayid()->set_playerid(event.firstToPlayId.second);
    gameMsg.mutable_lastcard()->set_color(static_cast<cardsGame::CardColor>(Cards::getColor(event.lastCard)));
    gameMsg.mutable_lastcard()->set_number(static_cast<cardsGame::CardNumber>(Cards::getNumber(event.lastCard)));

    cardsGame::GameEventMsg gameEvent;
    gameEvent.set_eventtype(cardsGame::EventType::START_GAME_EVENT);
    *gameEvent.mutable_startgame() = gameMsg;

    {
        std::lock_guard<std::mutex> lock(connectionsMutex);

        for (const auto& [playerId, connection] : connections) {
            gameEvent.mutable_playerinfo()->set_playerid(playerId);
            connection->send(gameEvent);
        }
    }
}

void GameSession_NS::GameSession::yourTurn(const YourTurnEvent& event)
{
    cardsGame::YourTurnMsg turnMsg;
    turnMsg.mutable_playerinfo()->set_playerid(event.playerId.second);
    for (const auto& card : event.yourHand) {
        cardsGame::Card* c = turnMsg.add_hand();
        c->set_color(static_cast<cardsGame::CardColor>(Cards::getColor(card)));
        c->set_number(static_cast<cardsGame::CardNumber>(Cards::getNumber(card)));
    }
    turnMsg.set_strongcolor(static_cast<cardsGame::CardColor>(event.strongColor));
    for(auto card : event.legalCards) {
        cardsGame::Card* c = turnMsg.add_legalcards();
        c->set_color(static_cast<cardsGame::CardColor>(Cards::getColor(card)));
        c->set_number(static_cast<cardsGame::CardNumber>(Cards::getNumber(card)));
    }

    for (const auto& move : event.movesPlayedInRound) {
        auto* m = turnMsg.add_movesplayedinround();
        cardsGame::Card *c = m->mutable_card();
        c->set_color(static_cast<cardsGame::CardColor>(Cards::getColor(move.card)));
        c->set_number(static_cast<cardsGame::CardNumber>(Cards::getNumber(move.card)));
        m->set_call(static_cast<cardsGame::Call>(move.call));
    }

    cardsGame::GameEventMsg turnEvent;
    turnEvent.set_eventtype(cardsGame::EventType::YOUR_TURN_EVENT);
    turnEvent.mutable_playerinfo()->set_playerid(event.playerId.second);
    *turnEvent.mutable_yourturn() = turnMsg;

    {
        std::lock_guard<std::mutex> lock(connectionsMutex);

        auto it = connections.find(event.playerId.second);
        if (it != connections.end()) {
            it->second->send(turnEvent);
        } else {
            std::cerr << "No connection for player " << event.playerId.second << std::endl;
        }
    }
}

cardsGame::AcussoMsg::AcussoType GameSession_NS::GameSession::translateAcussoType(AcussoType engineType)
{
    switch(engineType) {
        case NapolitanaSpade: return cardsGame::AcussoMsg::NAPOLITANA_SPADE;
        case NapolitanaCoppe: return cardsGame::AcussoMsg::NAPOLITANA_COPPE;
        case NapolitanaDenari: return cardsGame::AcussoMsg::NAPOLITANA_DENARI;
        case NapolitanaBastoni: return cardsGame::AcussoMsg::NAPOLITANA_BASTONI;
        case AssoAcusso: return cardsGame::AcussoMsg::ASSO_ACUSSO;
        case DueAcusso: return cardsGame::AcussoMsg::DUE_ACUSSO;
        case TreAcusso: return cardsGame::AcussoMsg::TRE_ACUSSO;
        case AssoSenzaSpade: return cardsGame::AcussoMsg::ASSO_SENZA_SPADE;
        case AssoSenzaCoppe: return cardsGame::AcussoMsg::ASSO_SENZA_COPPE;
        case AssoSenzaDenari: return cardsGame::AcussoMsg::ASSO_SENZA_DENARI;
        case AssoSenzaBastoni: return cardsGame::AcussoMsg::ASSO_SENZA_BASTONI;
        case DueSenzaSpade: return cardsGame::AcussoMsg::DUE_SENZA_SPADE;
        case DueSenzaCoppe: return cardsGame::AcussoMsg::DUE_SENZA_COPPE;
        case DueSenzaDenari: return cardsGame::AcussoMsg::DUE_SENZA_DENARI;
        case DueSenzaBastoni: return cardsGame::AcussoMsg::DUE_SENZA_BASTONI;
        case TreSenzaSpade: return cardsGame::AcussoMsg::TRE_SENZA_SPADE;
        case TreSenzaCoppe: return cardsGame::AcussoMsg::TRE_SENZA_COPPE;
        case TreSenzaDenari: return cardsGame::AcussoMsg::TRE_SENZA_DENARI;
        case TreSenzaBastoni: return cardsGame::AcussoMsg::TRE_SENZA_BASTONI;
        case NoAcusso: return cardsGame::AcussoMsg::NO_ACUSSO;
        default: return cardsGame::AcussoMsg::NO_ACUSSO;
    }
}

void GameSession_NS::GameSession::acussoEvent(const AcussoEvent& event)
{
    cardsGame::AcussosMsg msgList;
    msgList.mutable_acussoplayerid()->set_playerid(event.playerId.second);

    for(const auto& acusso : event.acussos)
    {
        cardsGame::AcussoMsg* acussoMsg = msgList.add_acusso();
        acussoMsg->set_acussotype(translateAcussoType(acusso));
    }

    cardsGame::GameEventMsg gameEvent;
    gameEvent.set_eventtype(cardsGame::EventType::ACUSSO_EVENT);
    *gameEvent.mutable_acussomsg() = msgList;

    {
        std::lock_guard<std::mutex> lock(connectionsMutex);

        for (const auto& [playerId, connection] : connections) {
            if(playerId != event.playerId.second)
            {
                gameEvent.mutable_playerinfo()->set_playerid(playerId);
                connection->send(gameEvent);
            }
        }
    }
}

void GameSession_NS::GameSession::playerPlayedMoveEvent(const PlayerPlayedMoveEvent& event)
{
    cardsGame::PlayerPlayedMoveMsg msg;

    cardsGame::Move* move = msg.mutable_move();
    cardsGame::Card* card = move->mutable_card();
    card->set_color(static_cast<cardsGame::CardColor>(Cards::getColor(event.move.card)));
    card->set_number(static_cast<cardsGame::CardNumber>(Cards::getNumber(event.move.card)));
    move->set_call(static_cast<cardsGame::Call>(event.move.call));
    
    msg.mutable_playerinfo()->set_playerid(event.move.playerId.second);

    cardsGame::GameEventMsg gameEvent;
    gameEvent.set_eventtype(cardsGame::EventType::PLAYER_PLAYED_MOVE_EVENT);
    *gameEvent.mutable_playerplayedmove() = msg;

    {
        std::lock_guard<std::mutex> lock(connectionsMutex);

        for (const auto& [playerId, connection] : connections) {
            if(playerId != event.move.playerId.second)
            {
                gameEvent.mutable_playerinfo()->set_playerid(playerId);
                connection->send(gameEvent);
            }
        }
    }
}

void GameSession_NS::GameSession::endRound(const RoundOverEvent& event)
{
    cardsGame::RoundOverMsg roundMsg;
    roundMsg.mutable_winnerid()->set_playerid(event.roundResult.winnerId.second);

    cardsGame::Points* points = roundMsg.mutable_points();
    points->set_punti(event.roundResult.points.punta);
    points->set_bella(event.roundResult.points.bella);

    cardsGame::GameEventMsg gameEvent;
    gameEvent.set_eventtype(cardsGame::EventType::ROUND_OVER_EVENT);
    *gameEvent.mutable_roundover() = roundMsg;

    {
        std::lock_guard<std::mutex> lock(connectionsMutex);

        for (const auto& [playerId, connection] : connections) {
            gameEvent.mutable_playerinfo()->set_playerid(playerId);
            connection->send(gameEvent);
        }
    }
}

void GameSession_NS::GameSession::endGame(const GameOverEvent& event)
{
    cardsGame::GameOverMsg gameMsg;
    gameMsg.set_teamwinnerid(event.gameResult.winnerId);

    for(int i = 0; i <= 1; i++)
    {
        auto* teamPoints = gameMsg.add_points();
        teamPoints->set_teamid(i);
        cardsGame::Points* points = teamPoints->mutable_points();
        points->set_punti(event.gameResult.points.at(i).punta);
        points->set_bella(event.gameResult.points.at(i).bella);
    }


    cardsGame::GameEventMsg gameEvent;
    gameEvent.set_eventtype(cardsGame::EventType::GAME_OVER_EVENT);
    *gameEvent.mutable_gameover() = gameMsg;

    {
        std::lock_guard<std::mutex> lock(connectionsMutex);

        for (const auto& [playerId, connection] : connections) {
            gameEvent.mutable_playerinfo()->set_playerid(playerId);
            connection->send(gameEvent);
        }
    }
}

void GameSession_NS::GameSession::endMatch(const MatchOverEvent& event)
{
    cardsGame::MatchOverMsg matchMsg;
    matchMsg.set_teamwinnerid(event.matchResult.winnerId);

    for(int i = 0; i <= 1; i++)
    {
        auto* matchScore = matchMsg.add_score();
        matchScore->set_teamid(i);

        matchScore->set_numwongames(event.matchResult.score.at(i).wonGames);
    }

    cardsGame::GameEventMsg gameEvent;
    gameEvent.set_eventtype(cardsGame::EventType::MATCH_OVER_EVENT);
    *gameEvent.mutable_matchover() = matchMsg;

    {
        std::lock_guard<std::mutex> lock(connectionsMutex);

        for (const auto& [playerId, connection] : connections) {
            gameEvent.mutable_playerinfo()->set_playerid(playerId);
            connection->send(gameEvent);
        }
    }
}

void GameSession_NS::GameSession::moveRsp(const MoveResponseEvent& event)
{
    cardsGame::PlayMoveRspMsg moveMsg;
    cardsGame::MoveRsp moveRsp;
    MoveRspToProto(event.moveValidity, moveRsp);

    std::cout << "moveValidity: " << event.moveValidity << std::endl;
    std::cout << "MoveRsp: " << moveRsp << std::endl;

    moveMsg.set_moversp(moveRsp);
    moveMsg.mutable_playerinfo()->set_playerid(event.move.playerId.second);

    cardsGame::GameEventMsg gameEvent;
    gameEvent.set_eventtype(cardsGame::EventType::MOVE_RSP_EVENT);
    *gameEvent.mutable_playmoversp() = moveMsg;

    {
        std::lock_guard<std::mutex> lock(connectionsMutex);

        for (const auto& [playerId, connection] : connections) {
            if(playerId == event.move.playerId.second)
            {
                gameEvent.mutable_playerinfo()->set_playerid(playerId);
                connection->send(gameEvent);
            }
        }
    }
}

void GameSession_NS::GameSession::tressetteDealtCards(const TressetteDealtCardsEvent& event)
{
    std::cout << "Id: " << event.playerId.second << std::endl;
    cardsGame::PlayerDealtCardsMsg dealMsg;
    dealMsg.mutable_playerinfo()->set_playerid(event.playerId.second);
    for (const auto& card : event.dealtCards) {
        cardsGame::Card* c = dealMsg.add_card();
        c->set_color(static_cast<cardsGame::CardColor>(Cards::getColor(card)));
        c->set_number(static_cast<cardsGame::CardNumber>(Cards::getNumber(card)));
    }

    cardsGame::GameEventMsg dealEvent;
    dealEvent.set_eventtype(cardsGame::EventType::PLAYER_DEALT_CARDS_EVENT);
    *dealEvent.mutable_playerdealtcards() = dealMsg;
    {
        std::lock_guard<std::mutex> lock(connectionsMutex);

        for (const auto& [playerId, connection] : connections) {
            if(playerId != playerIdToSessionPlayerId(event.playerId.second))
            {
                dealEvent.mutable_playerinfo()->set_playerid(playerId);
                connection->send(dealEvent);
            }
        }
    }
}

void GameSession_NS::GameSession::briscolaLastRound(const BriscolaLastRoundEvent& event)
{
    cardsGame::GameEventMsg gameEvent;
    gameEvent.set_eventtype(cardsGame::EventType::BRISCOLA_LAST_ROUND_EVENT);

    cardsGame::BriscolaLastRoundMsg blreMsg;

    blreMsg.mutable_teammateid()->set_playerid(event.senderTeammatePlayerId.second);
    for (const auto& card : event.senderTeammateHand) {
        cardsGame::Card* c = blreMsg.add_cards();
        c->set_color(static_cast<cardsGame::CardColor>(Cards::getColor(card)));
        c->set_number(static_cast<cardsGame::CardNumber>(Cards::getNumber(card)));
    }

    *gameEvent.mutable_briscolalastround() = blreMsg;
    {
        std::lock_guard<std::mutex> lock(connectionsMutex);

        for (const auto& [playerId, connection] : connections) {
            if(playerId == playerIdToSessionPlayerId(event.receiverPlayerId.second))
            {
                gameEvent.mutable_playerinfo()->set_playerid(playerId);
                connection->send(gameEvent);
            }
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
    tressetteDealtCards(event);
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
    //startMatch(event);
}

void GameSession_NS::GameSession::onEvent(
    const RoundOverEvent& event)
{
    std::cout << "RoundOverEvent received\n" << std::endl;
    endRound(event);
}

void GameSession_NS::GameSession::onEvent(
    const GameOverEvent& event)
{
    std::cout << "Game over!\n" << std::endl;
    endGame(event);
}

void GameSession_NS::GameSession::onEvent(
    const MatchOverEvent& event)
{
    std::cout << "MatchOverEvent received\n" << std::endl;
    endMatch(event);
}

void GameSession_NS::GameSession::onEvent(
    const YourTurnEvent& event)
{
    std::cout << "YourTurnEvent received\n" << std::endl;
    yourTurn(event);
}

void GameSession_NS::GameSession::onEvent(
    const AcussoEvent& event)
{
    std::cout << "AcussoEvent received\n" << std::endl;
    acussoEvent(event);
}

void GameSession_NS::GameSession::onEvent(
    const BriscolaLastRoundEvent& event)
{
    std::cout << "BriscolaLastRoundEvent received\n" << std::endl;
    briscolaLastRound(event);
}

void GameSession_NS::GameSession::onEvent(
    const MoveResponseEvent& event)
{
    std::cout << "MoveResponseEvent received\n" << std::endl;
    moveRsp(event);
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

MoveReturnValue GameSession_NS::GameSession::ApplyMove(const Move& move) {
    MoveReturnValue matchRetVal = match->ApplyMove(move);
    if(matchRetVal == Finish)
    {
        cntMatchesPlayed++;
        int winningTeam = match->GetMatchResult().winnerId;
        teamWins[winningTeam]++;
        std::cout << "Match finished! Winning team: " << winningTeam << std::endl;
    }

    return matchRetVal;
}

bool GameSession_NS::GameSession::IsSessionOver() const {
    return cntMatchesPlayed >= numMatchesToPlay;
}

void GameSession_NS::GameSession::PrintResults() {
    std::cout << "Game session results:" << std::endl;
    std::cout << "Team 1 wins: " << teamWins[0] << std::endl;
    std::cout << "Team 2 wins: " << teamWins[1] << std::endl;
}

grpc::
Status GameSession_NS::GameSession::PlayMove(grpc::ServerContext* context, const cardsGame::PlayMoveReq* request, ::google::protobuf::Empty* response) {
    Move move;
    PlayMoveReqToDomain(*request, move);

    ApplyMove(move);

    return grpc::Status::OK;
}


grpc::Status GameSession_NS::GameSession::SubscribeEvents(grpc::ServerContext* context, 
                                const cardsGame::PlayerInfo* request,
                                grpc::ServerWriter<cardsGame::GameEventMsg>* writer)
{
    std::cout << "SubscribeEvents called for player id: " << request->playerid() << std::endl;
    int serverPlayerId = request->playerid();

    // Look up session player ID from pre-populated map
    auto it = players.find(serverPlayerId);
    if (it == players.end()) {
        std::cerr << "Player " << serverPlayerId << " not found in session!" << std::endl;
        return grpc::Status(grpc::INVALID_ARGUMENT, "Player not registered in this session");
    }
    int sessionPlayerId = it->second;

    cardsGame::GameEventMsg initialEvent;
    cardsGame::StartMatchMsg startMatch;
    startMatch.mutable_firsttoplayid()->set_playerid(0); // first player to play
    cardsGame::GameFormat* gameFormat = startMatch.mutable_gameformat();
    gameFormat->set_gametype(gameType);
    cardsGame::SingleOrMulti sm = (players.size() == 2) ? cardsGame::SingleOrMulti::SINGLE : cardsGame::SingleOrMulti::MULTI;
    gameFormat->set_singleormulti(sm);

    int teammateId = -1;
    if(sm == cardsGame::SingleOrMulti::MULTI)
        teammateId = (sessionPlayerId + 2) % 4;

    startMatch.set_teammateid(teammateId);

    initialEvent.set_eventtype(cardsGame::EventType::START_MATCH_EVENT);
    initialEvent.mutable_playerinfo()->set_playerid(sessionPlayerId);
    *initialEvent.mutable_startmatch() = startMatch;

    std::cout << "Sending START_MATCH_EVENT to player " << serverPlayerId 
              << " (session ID " << sessionPlayerId << ") with teammate ID " << teammateId << std::endl;
    
    bool writeSuccess = writer->Write(initialEvent); // send initial message
    std::cout << "Write result: " << (writeSuccess ? "SUCCESS" : "FAILED") << std::endl;

    {
        std::lock_guard<std::mutex> lock(connectionsMutex);
        connections[sessionPlayerId] = std::make_shared<PlayerConnection>(writer);
        numPlayers++;
    }

    while (!context->IsCancelled() && !IsSessionOver()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    {
        std::lock_guard<std::mutex> lock(connectionsMutex);
        connections.erase(sessionPlayerId);
        numPlayers--;
    }

    std::cout << "Session with player " << serverPlayerId << " (session ID " << sessionPlayerId << ") over" << std::endl;


    return grpc::Status::OK;    
}

int GameSession_NS::GameSession::playerIdToSessionPlayerId(int serverPlayerId)
{
    auto it = players.find(serverPlayerId);
    if (it != players.end()) {
        return it->second;
    }
    return -1;
}

void GameSession_NS::GameSession::PlayMoveReqToDomain(const cardsGame::PlayMoveReq& req, Move& move)
{
    // Convert player ID
    move.playerId.second = playerIdToSessionPlayerId(req.playerinfo().playerid());
    move.playerId.first = move.playerId.second % 2;

    // Convert card
    const cardsGame::Card& protoCard = req.move().card();
    move.card = Cards::makeCard(static_cast<Color>(protoCard.color()), static_cast<Number>(protoCard.number()));

    // Convert call
    move.call = static_cast<Call>(req.move().call());

    std::cout << "Move: player=" << move.playerId.second << ", card=" << Cards::CardToString(move.card) << ", call=" << move.call << std::endl;
}

void GameSession_NS::GameSession::MoveRspToProto(const MoveReturnValue& moveRsp, cardsGame::MoveRsp& rsp)
{
    MoveReturnValue val = moveRsp;
    if(moveRsp == Finish) // proto diff
    {
        val = MoveReturnValue::Ok;
    }

    rsp = static_cast<cardsGame::MoveRsp>(val);
}