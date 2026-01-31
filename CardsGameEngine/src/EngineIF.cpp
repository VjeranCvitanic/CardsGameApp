#include "EngineIF.h"
#include "Logger.h"

std::unique_ptr<BriscolaMatch_NS::BriscolaMatch>
createBriscolaMatch(EventEmitter& emitter,
                    int numPlayers)
{
    Logger::logger_setup("./out/briscola/logs/", nullptr, "./out/event/logs/", DEBUG, true);
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    CardsMatch_NS::Players players;
    if(numPlayers == 2)
    {
        players = {{0, 0}, {1, 1}};
    }
    else if(numPlayers == 4)
    {
        players = {{0, 0}, {0, 1}, {1, 2}, {1, 3}};
    }
    else
    {
        throw std::invalid_argument("Unsupported number of players for Briscola");
    }

    CardsMatch_NS::MatchState matchState({0, 0}, players);
    return std::make_unique<BriscolaMatch_NS::BriscolaMatch>(
        matchState,
        numPlayers,
        emitter
    );
}

std::unique_ptr<TressetteMatch_NS::TressetteMatch>
createTressetteMatch(EventEmitter& emitter,
                    int numPlayers)
{
    Logger::logger_setup("./out/tressette/logs/", nullptr, "./out/event/logs/", DEBUG, true);
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    CardsMatch_NS::Players players;
    if(numPlayers == 2)
    {
        players = {{0, 0}, {1, 1}};
    }
    else if(numPlayers == 4)
    {
        players = {{0, 0}, {0, 1}, {1, 2}, {1, 3}};
    }
    else
    {
        throw std::invalid_argument("Unsupported number of players for Tressette");
    }

    CardsMatch_NS::MatchState matchState({0, 0}, players);
        return std::make_unique<TressetteMatch_NS::TressetteMatch>(
        matchState,
        numPlayers,
        emitter
    );
}