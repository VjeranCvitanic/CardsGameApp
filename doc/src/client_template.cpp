/**
 * CardsGame – C++ Template Client
 *
 * This file implements the complete interface towards the CardsGame server.
 * Replace the on_*() handler stubs with your game logic.
 *
 * Build (from repo root, after server is built):
 *   g++ -std=c++17 client_template.cpp \
 *       -I<build_dir> \                       # generated pb headers
 *       <build_dir>/cardsGame.pb.cc \
 *       <build_dir>/cardsGame.grpc.pb.cc \
 *       $(pkg-config --cflags --libs grpc++ protobuf) \
 *       -o client_template
 *
 * Or add as a CMake target in gRPC/CMakeLists.txt:
 *   add_executable(client_template doc/template_client/client_template.cpp)
 *   target_link_libraries(client_template PRIVATE proto_lib)
 *   target_include_directories(client_template PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/inc)
 */

#include <grpcpp/grpcpp.h>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <chrono>
#include "cardsGame.grpc.pb.h"
#include "cardsGame.pb.h"

// ============================================================
// GameState – minimal client-side view of the running match
// ============================================================
struct GameState {
    int myServerId   = -1;   // assigned by server on Connect()
    int mySessionId  = -1;   // assigned by session on START_MATCH_EVENT (0-3)
    int teammateId   = -1;   // -1 for 2-player games
    cardsGame::GameType gameType = cardsGame::INVALID_GAME_TYPE;
    cardsGame::SingleOrMulti singleOrMulti = cardsGame::INVALID_SINGLE_MULTI;
    cardsGame::CardColor briscolaColor = cardsGame::NO_COLOR;

    std::vector<cardsGame::Card> hand;
    std::vector<cardsGame::Card> legalCards;
    std::vector<cardsGame::Move> movesPlayedInRound;
};

// ============================================================
// CardsGameTemplateClient
// ============================================================
class CardsGameTemplateClient {
public:
    CardsGameTemplateClient(const std::string& lobbyAddress,
                            const std::string& playerName,
                            cardsGame::GameType  gameType,
                            cardsGame::SingleOrMulti singleOrMulti)
        : playerName_(playerName)
    {
        state_.gameType     = gameType;
        state_.singleOrMulti = singleOrMulti;

        lobbyStub_ = cardsGame::CardsGameServer::NewStub(
            grpc::CreateChannel(lobbyAddress, grpc::InsecureChannelCredentials()));
    }

    // ----------------------------------------------------------
    // Entry point
    // ----------------------------------------------------------
    void Run() {
        if (!connect()) return;
        eventLoop();
    }

private:
    // ---- stubs ------------------------------------------------
    std::unique_ptr<cardsGame::CardsGameServer::Stub>  lobbyStub_;
    std::unique_ptr<cardsGame::CardsGameSession::Stub> sessionStub_;

    std::string playerName_;
    GameState   state_;

    // ==========================================================
    // STEP 1 – Connect to lobby
    // ==========================================================
    bool connect() {
        cardsGame::ConnectReq req;
        req.set_name(playerName_);

        auto* fmt = req.add_gameformats();
        fmt->set_gametype(state_.gameType);
        fmt->set_singleormulti(state_.singleOrMulti);

        cardsGame::ConnectRsp rsp;
        grpc::ClientContext ctx;
        grpc::Status status = lobbyStub_->Connect(&ctx, req, &rsp);

        if (!status.ok()) {
            std::cerr << "[connect] RPC failed: " << status.error_message() << "\n";
            return false;
        }

        if (rsp.has_fail_reason()) {
            std::cerr << "[connect] Server rejected: " << rsp.fail_reason() << "\n";
            return false;
        }

        state_.myServerId = rsp.successmsg().playerid();
        const std::string& sessionAddr = rsp.successmsg().address();
        std::cout << "[connect] OK – serverId=" << state_.myServerId
                  << "  sessionAddr=" << sessionAddr << "\n";

        grpc::ChannelArguments args;
        args.SetInt("grpc.wait_for_ready", 1);
        sessionStub_ = cardsGame::CardsGameSession::NewStub(
            grpc::CreateCustomChannel(sessionAddr,
                                      grpc::InsecureChannelCredentials(), args));
        return true;
    }

    // ==========================================================
    // STEP 2 – Subscribe to event stream and process events
    // ==========================================================
    void eventLoop() {
        grpc::ClientContext ctx;
        ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::minutes(60));
        ctx.set_wait_for_ready(true);

        cardsGame::PlayerInfo info;
        info.set_playerid(state_.myServerId);

        auto reader = sessionStub_->SubscribeEvents(&ctx, info);
        if (!reader) {
            std::cerr << "[eventLoop] Failed to open event stream\n";
            return;
        }

        cardsGame::GameEventMsg event;
        while (reader->Read(&event)) {
            dispatch(event);
        }

        grpc::Status status = reader->Finish();
        if (!status.ok()) {
            std::cerr << "[eventLoop] Stream closed with error: "
                      << status.error_message() << "\n";
        } else {
            std::cout << "[eventLoop] Stream closed cleanly – game over.\n";
        }
    }

    // ==========================================================
    // STEP 3 – Event dispatcher
    // ==========================================================
    void dispatch(const cardsGame::GameEventMsg& event) {
        switch (event.eventtype()) {
            case cardsGame::START_MATCH_EVENT:
                on_start_match(event.startmatch(), event.playerinfo());
                break;
            case cardsGame::START_GAME_EVENT:
                on_start_game(event.startgame(), event.playerinfo());
                break;
            case cardsGame::START_ROUND_EVENT:
                on_start_round(event.startround(), event.playerinfo());
                break;
            case cardsGame::PLAYER_DEALT_CARDS_EVENT:
                on_dealt_cards(event.playerdealtcards(), event.playerinfo());
                break;
            case cardsGame::YOUR_TURN_EVENT:
                on_your_turn(event.yourturn(), event.playerinfo());
                break;
            case cardsGame::MOVE_RSP_EVENT:
                on_move_response(event.playmoversp(), event.playerinfo());
                break;
            case cardsGame::PLAYER_PLAYED_MOVE_EVENT:
                on_player_played_move(event.playerplayedmove(), event.playerinfo());
                break;
            case cardsGame::ROUND_OVER_EVENT:
                on_round_over(event.roundover(), event.playerinfo());
                break;
            case cardsGame::GAME_OVER_EVENT:
                on_game_over(event.gameover(), event.playerinfo());
                break;
            case cardsGame::MATCH_OVER_EVENT:
                on_match_over(event.matchover(), event.playerinfo());
                break;
            case cardsGame::ACUSSO_EVENT:
                on_acusso(event.acussomsg(), event.playerinfo());
                break;
            case cardsGame::BRISCOLA_LAST_ROUND_EVENT:
                on_briscola_last_round(event.briscolalastround(), event.playerinfo());
                break;
            default:
                std::cerr << "[dispatch] Unknown event type: " << event.eventtype() << "\n";
        }
    }

    // ==========================================================
    // STEP 4 – Send a move
    // ==========================================================
    void sendMove(const cardsGame::Card& card, cardsGame::Call call = cardsGame::NO_CALL) {
        cardsGame::PlayMoveReq req;
        req.mutable_playerinfo()->set_playerid(state_.myServerId); // server ID, not session ID!
        req.mutable_move()->mutable_card()->CopyFrom(card);
        req.mutable_move()->set_call(call);

        google::protobuf::Empty response;
        grpc::ClientContext ctx;
        grpc::Status s = sessionStub_->PlayMove(&ctx, req, &response);
        if (!s.ok()) {
            std::cerr << "[sendMove] Failed: " << s.error_message() << "\n";
        }
    }

    // ==========================================================
    // EVENT HANDLERS – fill in your game logic here
    // ==========================================================

    /**
     * First event on stream. Sets session-local player ID and teammate.
     * gameFormat tells you which game you're about to play.
     * For 4-player: teammateId = (mySessionId + 2) % 4
     */
    void on_start_match(const cardsGame::StartMatchMsg& msg,
                        const cardsGame::PlayerInfo& addressedTo) {
        state_.mySessionId = addressedTo.playerid();
        state_.teammateId  = msg.has_teammateid() ? msg.teammateid() : -1;
        state_.gameType    = msg.gameformat().gametype();
        state_.singleOrMulti = msg.gameformat().singleormulti();

        std::cout << "[on_start_match] sessionId=" << state_.mySessionId
                  << "  teammate=" << state_.teammateId
                  << "  game=" << msg.gameformat().gametype() << "\n";

        // TODO: initialize your UI / AI state here
    }

    /**
     * A new game begins.
     * For Briscola: msg.has_last_card() == true → that card's color is the briscola suit.
     * firstToPlayId is the session-local ID of the player who plays first.
     */
    void on_start_game(const cardsGame::StartGameMsg& msg,
                       const cardsGame::PlayerInfo& addressedTo) {
        if (msg.has_lastcard()) {
            state_.briscolaColor = msg.lastcard().color();
            std::cout << "[on_start_game] Briscola color: " << state_.briscolaColor << "\n";
        }
        std::cout << "[on_start_game] firstToPlay=" << msg.firsttoplayid().playerid() << "\n";

        // TODO: reset per-game counters, display game info
    }

    /**
     * A new round begins.
     * firstToPlayId is the session-local ID of the player who starts this round.
     */
    void on_start_round(const cardsGame::StartRoundMsg& msg,
                        const cardsGame::PlayerInfo& addressedTo) {
        state_.movesPlayedInRound.clear();
        std::cout << "[on_start_round] firstToPlay=" << msg.firsttoplayid().playerid() << "\n";

        // TODO: reset round state
    }

    /**
     * Cards were dealt to a player.
     * Briscola:   msg.playerinfo().playerid() == mySessionId → your own cards
     * Tressette:  you receive every OTHER player's cards (for full transparency)
     */
    void on_dealt_cards(const cardsGame::PlayerDealtCardsMsg& msg,
                        const cardsGame::PlayerInfo& addressedTo) {
        if (addressedTo.playerid() == state_.mySessionId) {
            state_.hand.assign(msg.card().begin(), msg.card().end());
            std::cout << "[on_dealt_cards] Your hand (" << state_.hand.size() << " cards)\n";
        } else {
            std::cout << "[on_dealt_cards] Player " << msg.playerinfo().playerid()
                      << " was dealt " << msg.card_size() << " cards\n";
        }

        // TODO: update display
    }

    /**
     * It's YOUR turn to play.
     * hand          – your current hand
     * legalCards    – cards you are ALLOWED to play (always play from here!)
     * movesPlayedInRound – moves played so far in this round
     * strongColor   – briscola color (NO_COLOR for tressette)
     *
     * MUST call sendMove() before this function returns (or schedule it async).
     */
    void on_your_turn(const cardsGame::YourTurnMsg& msg,
                      const cardsGame::PlayerInfo& addressedTo) {
        state_.hand.assign(msg.hand().begin(), msg.hand().end());
        state_.legalCards.assign(msg.legalcards().begin(), msg.legalcards().end());
        state_.movesPlayedInRound.assign(msg.movesplayedinround().begin(),
                                         msg.movesplayedinround().end());

        std::cout << "[on_your_turn] hand=" << state_.hand.size()
                  << "  legal=" << state_.legalCards.size() << "\n";

        // ---- Example: always play the first legal card, no call ----
        if (!state_.legalCards.empty()) {
            sendMove(state_.legalCards[0], cardsGame::NO_CALL);
        }

        // TODO: replace above with your decision logic (AI, human input, etc.)
    }

    /**
     * Server's response to your PlayMove call.
     * MOVE_OK                    → move accepted, wait for game events
     * NOT_YOUR_TURN              → you sent a move out of turn
     * CARD_NOT_IN_HAND           → card not in your hand
     * COLOR_CONSTRAINT_NOT_MET   → must follow suit
     * CANT_CALL_IF_NOT_FIRST_OF_HAND → call only allowed on first card of round
     */
    void on_move_response(const cardsGame::PlayMoveRspMsg& msg,
                          const cardsGame::PlayerInfo& addressedTo) {
        if (msg.moversp() != cardsGame::MOVE_OK) {
            std::cerr << "[on_move_response] INVALID MOVE: " << msg.moversp()
                      << " – you must call sendMove() again with a valid card!\n";
            // TODO: re-trigger your move selection
        } else {
            std::cout << "[on_move_response] MOVE_OK\n";
        }
    }

    /**
     * Another player just played a card.
     * msg.playerinfo().playerid() = session-local ID of the mover
     * msg.move() = card + call they played
     * You do NOT receive this for your own moves.
     */
    void on_player_played_move(const cardsGame::PlayerPlayedMoveMsg& msg,
                               const cardsGame::PlayerInfo& addressedTo) {
        std::cout << "[on_player_played_move] player=" << msg.playerinfo().playerid()
                  << "  color=" << msg.move().card().color()
                  << "  number=" << msg.move().card().number()
                  << "  call=" << msg.move().call() << "\n";

        // TODO: update shared game state, opponent hand tracking, etc.
    }

    /**
     * A round just finished.
     * msg.winnerid().playerid() = session-local ID of the winner
     * msg.points().punti()      = trick points
     * msg.points().bella()      = bella bonus (Tressette)
     */
    void on_round_over(const cardsGame::RoundOverMsg& msg,
                       const cardsGame::PlayerInfo& addressedTo) {
        std::cout << "[on_round_over] winner=" << msg.winnerid().playerid()
                  << "  punti=" << msg.points().punti()
                  << "  bella=" << msg.points().bella() << "\n";

        // TODO: update score display
    }

    /**
     * A game is over.
     * msg.team_winner_id()      = winning team (0 or 1)
     * msg.points(i).points()    = per-team accumulated points
     */
    void on_game_over(const cardsGame::GameOverMsg& msg,
                      const cardsGame::PlayerInfo& addressedTo) {
        std::cout << "[on_game_over] winningTeam=" << msg.teamwinnerid() << "\n";
        for (int i = 0; i < msg.points_size(); ++i) {
            std::cout << "  team " << msg.points(i).teamid()
                      << ": punti=" << msg.points(i).points().punti() << "\n";
        }

        // TODO: display game result
    }

    /**
     * The entire match is over (all games played).
     * msg.team_winner_id()       = overall winning team
     * msg.score(i).num_won_games() = games won by team i (Briscola)
     * msg.score(i).points()        = cumulative points (Tressette)
     */
    void on_match_over(const cardsGame::MatchOverMsg& msg,
                       const cardsGame::PlayerInfo& addressedTo) {
        std::cout << "[on_match_over] winningTeam=" << msg.teamwinnerid() << "\n";
        for (int i = 0; i < msg.score_size(); ++i) {
            const auto& s = msg.score(i);
            if (s.has_numwongames()) {
                std::cout << "  team " << s.teamid()
                          << ": wonGames=" << s.numwongames() << "\n";
            } else {
                std::cout << "  team " << s.teamid()
                          << ": punti=" << s.points().punti() << "\n";
            }
        }

        // TODO: show final result, clean up
    }

    /**
     * Tressette only: a player declares their acussos.
     * You receive this for all players EXCEPT yourself.
     * msg.acussoplayer_id() = session-local ID of the player who has acussos
     * msg.acusso(i)         = individual acusso type + points
     */
    void on_acusso(const cardsGame::AcussosMsg& msg,
                   const cardsGame::PlayerInfo& addressedTo) {
        std::cout << "[on_acusso] player=" << msg.acussoplayerid().playerid()
                  << "  count=" << msg.acusso_size() << "\n";

        // TODO: track declared acussos for score calculation
    }

    /**
     * Briscola 4-player only, last round: your teammate reveals their hand.
     * msg.teammate_id() = session-local ID of the teammate
     * msg.cards()       = their remaining cards
     */
    void on_briscola_last_round(const cardsGame::BriscolaLastRoundMsg& msg,
                                const cardsGame::PlayerInfo& addressedTo) {
        std::cout << "[on_briscola_last_round] teammate="
                  << msg.teammateid().playerid()
                  << "  cards=" << msg.cards_size() << "\n";

        // TODO: use teammate's hand to inform your move strategy
    }
};

// ============================================================
// main
// ============================================================
int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0]
                  << " <playerName> <briscola|tressette> <2|4>\n";
        return 1;
    }

    const std::string name    = argv[1];
    const std::string gameStr = argv[2];
    const int         nPlayers = std::stoi(argv[3]);

    cardsGame::GameType gt;
    if (gameStr == "briscola") gt = cardsGame::BRISCOLA;
    else if (gameStr == "tressette") gt = cardsGame::TRESSETTE;
    else { std::cerr << "Unknown game type\n"; return 1; }

    cardsGame::SingleOrMulti sm;
    if (nPlayers == 2) sm = cardsGame::SINGLE;
    else if (nPlayers == 4) sm = cardsGame::MULTI;
    else { std::cerr << "numPlayers must be 2 or 4\n"; return 1; }

    CardsGameTemplateClient client("localhost:50051", name, gt, sm);
    client.Run();

    return 0;
}
