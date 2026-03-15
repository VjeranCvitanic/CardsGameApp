#include "client.h"
#include "cardsGame.grpc.pb.h"
#include "cardsGame.pb.h"
#include <chrono>
#include <iostream>
#include <memory>
#include <ostream>
#include <thread>

bool cardsGameClient::Connect(const std::string &name,
                              cardsGame::GameType gameType,
                              cardsGame::SingleOrMulti singleOrMulti) {
  cardsGame::ConnectReq request;
  request.set_name(name);
  auto *formats = request.add_gameformats();
  formats->set_gametype(gameType);
  formats->set_singleormulti(singleOrMulti);

  cardsGame::ConnectRsp reply;
  grpc::ClientContext context;

  grpc::Status status = lobbyStub_->Connect(&context, request, &reply);
  if (status.ok()) {
    id = reply.successmsg().playerid();
    sessionId_ = reply.successmsg().sessionid();
    std::cout << "My id: " << id << ", sessionId: " << sessionId_ << std::endl;
    return true;
  } else {
    std::cout << "RPC failed: " << status.error_message() << std::endl;
    return false;
  }
}

bool cardsGameClient::WaitForSessionStarted(
    grpc::ClientContext &sessionContext,
    std::unique_ptr<grpc::ClientReader<cardsGame::GameEventMsg>> &reader) {
  sessionContext.set_deadline(std::chrono::system_clock::now() +
                              std::chrono::minutes(30));
  sessionContext.set_wait_for_ready(true);
                      
  cardsGame::PlayerInfo playerInfo;
  playerInfo.set_playerid(id);
  playerInfo.set_sessionid(sessionId_);

  cardsGame::GameEventMsg sessionReply;
  reader = sessionStub_->SubscribeEvents(&sessionContext, playerInfo);
  
  if (!reader) {
    std::cerr << "Failed to create event stream" << std::endl;
    return false;
  }

  while (reader->Read(&sessionReply)) {
    if (sessionReply.eventtype() == cardsGame::EventType::START_MATCH_EVENT) {
      std::cout << "[EVENT] " << sessionReply.DebugString() << std::endl;
      return true;
    }
  }

  std::cerr << "Stream ended without receiving START_MATCH_EVENT" << std::endl;
  return false;
}

void cardsGameClient::PlayMove(std::unique_ptr<cardsGame::Move> move) {
  google::protobuf::Empty response;
  grpc::ClientContext moveCtx;

  cardsGame::PlayMoveReq request;
  cardsGame::PlayerInfo* playerInfo = request.mutable_playerinfo();
  playerInfo->set_playerid(id);
  playerInfo->set_sessionid(sessionId_);
  request.set_allocated_move(move.release());

  grpc::Status s = sessionStub_->PlayMove(&moveCtx, request, &response);

  if (!s.ok()) {
    std::cerr << "PlayMove failed: " << s.error_message() << std::endl;
  }
}

void cardsGameClient::StartClient() {
  if (!Connect(name, gameFormat.gametype(), gameFormat.singleormulti())) {
    std::cout << "Failed to connect" << std::endl;
    return;
  }

  std::thread eventThread([&]() {
    bool matchOver = false;

    while (!matchOver) {
      grpc::ClientContext context;
      context.set_deadline(std::chrono::system_clock::now() +
                           std::chrono::minutes(30));
      context.set_wait_for_ready(true);

      cardsGame::PlayerInfo playerInfo;
      playerInfo.set_playerid(id);
      playerInfo.set_sessionid(sessionId_);

      auto reader = sessionStub_->SubscribeEvents(&context, playerInfo);
      if (!reader) {
        std::cerr << "Failed to create event stream" << std::endl;
        break;
      }

      bool isReplay = false;
      cardsGame::GameEventMsg event;

      while (reader->Read(&event)) {
        if (event.eventtype() == cardsGame::RECONNECT_START_EVENT) {
          isReplay = true;
          std::cout << "[RECONNECT] Replaying events..." << std::endl;
          continue;
        }
        if (event.eventtype() == cardsGame::RECONNECT_END_EVENT) {
          isReplay = false;
          std::cout << "[RECONNECT] Replay complete, resuming." << std::endl;
          continue;
        }

        std::cout << "[EVENT] " << event.DebugString() << std::endl;

        if (event.eventtype() == cardsGame::MATCH_OVER_EVENT) {
          matchOver = true;
        }

        // During replay, don't respond to YOUR_TURN (those turns already happened)
        if (!isReplay) {
          processEvent(event);
          if (!isAi) {
            getchar();
          }
        }
      }

      grpc::Status status = reader->Finish();
      if (!status.ok()) {
        std::cerr << "Event stream closed with error: " << status.error_message()
                  << std::endl;
      }

      if (!matchOver) {
        std::cout << "Disconnected, attempting to reconnect in 2s..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
      } else {
        std::cout << "Match finished." << std::endl;
      }
    }
  });

  eventThread.join();
}

void runSpectator(int sessionId) {
  auto channel = grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());
  auto stub = cardsGame::CardsGameSession::NewStub(channel);

  grpc::ClientContext ctx;
  ctx.set_wait_for_ready(true);

  cardsGame::SpectateReq req;
  req.set_sessionid(sessionId);

  auto reader = stub->SpectateSession(&ctx, req);
  if (!reader) {
    std::cerr << "Failed to start spectating session " << sessionId << std::endl;
    return;
  }

  std::cout << "[SPECTATOR] Watching session " << sessionId << "..." << std::endl;

  cardsGame::GameEventMsg event;
  while (reader->Read(&event)) {
    std::cout << "[SPECTATOR] " << event.DebugString() << std::endl;
  }

  grpc::Status status = reader->Finish();
  if (!status.ok()) {
    std::cerr << "[SPECTATOR] Stream ended: " << status.error_message() << std::endl;
  } else {
    std::cout << "[SPECTATOR] Session finished." << std::endl;
  }
}

int main(int argc, char **argv) {
  srand(time(nullptr)); // Seed random number generator for AI

  // Spectator mode: client spectate <sessionId>
  if (argc == 3 && std::string(argv[1]) == "spectate") {
    int sessionId = std::stoi(argv[2]);
    runSpectator(sessionId);
    return 0;
  }
  
  if (argc != 5) {
    std::cout << "Usage: client <name> <gameType> <numPlayers> <human|ai>\n"
              << "       client spectate <sessionId>"
              << std::endl;
    return 1;
  }

  std::string name = argv[1];
  bool ai = std::string(argv[4]) == "ai";
  std::string gameType = argv[2];
  cardsGame::GameType gt;
  if (gameType == "briscola") {
    gt = cardsGame::GameType::BRISCOLA;
  } else if (gameType == "tressette") {
    gt = cardsGame::GameType::TRESSETTE;
  } else {
    std::cout << "Unknown game type: " << gameType << "." << std::endl;
    return 1;
  }
  int numPlayers = std::stoi(argv[3]);
  cardsGame::SingleOrMulti sm;
  if (numPlayers == 2) {
    sm = cardsGame::SingleOrMulti::SINGLE;
  } else if (numPlayers == 4) {
    sm = cardsGame::SingleOrMulti::MULTI;
  } else {
    std::cout << "Unknown number of players" << std::endl;
    return 1;
  }

  cardsGame::GameFormat gf = cardsGame::GameFormat();
  gf.set_gametype(gt);
  gf.set_singleormulti(sm);

  cardsGameClient lobbyClient(
      grpc::CreateChannel("localhost:50051",
                          grpc::InsecureChannelCredentials()),
      name, ai, gf);

  lobbyClient.StartClient();

  return 0;
}

void cardsGameClient::processEvent(const cardsGame::GameEventMsg &event) {
  if (event.eventtype() == cardsGame::YOUR_TURN_EVENT) {
    processMyTurn(event);
  }
}

void cardsGameClient::processMyTurn(const cardsGame::GameEventMsg &event) {
  std::cout << "My turn!" << std::endl;

  if (!isAi) {
    std::unique_ptr<cardsGame::Move> move = std::make_unique<cardsGame::Move>();
    parseInput(move.get(), event.yourturn().playerinfo().playerid());

    PlayMove(std::move(move));

    return;
  }

  auto &hand = event.yourturn().hand();
  int size = hand.size();

  int randomNum = rand() % size;

  std::unique_ptr<cardsGame::Move> move = std::make_unique<cardsGame::Move>();
  std::unique_ptr<cardsGame::Card> card = std::make_unique<cardsGame::Card>();
  card->set_color(hand[randomNum].color());
  card->set_number(hand[randomNum].number());

  move->set_allocated_card(card.release());
  move->set_call(cardsGame::NO_CALL);

  PlayMove(std::move(move));
}

int cardsGameClient::parse(std::string input, cardsGame::Move *move,
                           int playerId) {
  cardsGame::CardColor color;
  cardsGame::CardNumber number = cardsGame::INVALID_NUMBER;
  std::unique_ptr<cardsGame::Card> card = std::make_unique<cardsGame::Card>();
  cardsGame::Call call = cardsGame::NO_CALL;

  if (input.size() < 2 || input.size() > 4) {
    return -1;
  }

  switch (std::toupper(input[0])) {
  case 'S':
    color = cardsGame::SPADE;
    break;
  case 'D':
    color = cardsGame::DENARI;
    break;
  case 'B':
    color = cardsGame::BASTONI;
    break;
  case 'C':
    color = cardsGame::COPPE;
    break;
  default:
    return -2;
  }

  for (size_t i = 1; i < input.size(); ++i) {
    if (std::isdigit(input[i])) {
      number =
          static_cast<cardsGame::CardNumber>(number * 10 + (input[i] - '0'));
      if (number > 10) {
        number = static_cast<cardsGame::CardNumber>(number - 3);
      }
    } else {
      switch (std::toupper(input[i])) {
      case 'B':
        call = cardsGame::BUSSO;
        break;
      case 'S':
        call = cardsGame::STRISCIO;
        break;
      case 'Q':
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

  move->set_call(call);
  move->set_allocated_card(card.release());

  return 0;
}

void cardsGameClient::parseInput(cardsGame::Move *move, int i) {
  std::string input;

  do {
    std::cin >> input;
    getchar();
  } while (parse(input, move, i) != 0);
}
