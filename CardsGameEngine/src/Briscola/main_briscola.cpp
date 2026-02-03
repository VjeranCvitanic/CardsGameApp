#include <iostream>
#include "BriscolaMatch.h"
#include "Cards.h"
#include "Logger.h"

int parse(std::string input, Move& move, fullPlayerId playerId);
void parseInput(Move& move, fullPlayerId playerId);

int main()
{
    Logger::logger_setup("./out/briscola/logs/", nullptr, "./out/event/logs/", DEBUG, true);
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    EventEmitter eventEmitter;
    eventEmitter.subscribe(&Logger::GetInstance());

    CardsMatch_NS::Players players = {{0, 0}, {1, 1}};

    CardsMatch_NS::MatchState matchState({0, 0}, players);
    BriscolaMatch_NS::BriscolaMatch match(matchState, 2, eventEmitter);
    //TressetteMatch_NS::TressetteMatch match(matchState, 2, eventEmitter);

    Move move;
    fullPlayerId playerId = {0, 0};
    do
    {
        parseInput(move, playerId);
        playerId.first = (playerId.first + 1) % 2;
        playerId.second = (playerId.second + 1) % players.size();
    }while(match.ApplyMove(move) != Finish);
}


void parseInput(Move& move, fullPlayerId i)
{
    std::string input;
 
    do
    {
        std::cin >> input;
        getchar();
    }while(parse(input, move, i) != 0);
}

int parse(std::string input, Move& move, fullPlayerId playerId)
{
    Color color;
    int _number = 0;
    Card card;
    Call call = NoCall;

    if (input.size() < 2 || input.size() > 4)
    {
        return -1;
    } 

    switch(std::toupper(input[0]))
    {
        case 'S' :
            color = Spade;
            break;
        case 'D' :
            color = Denari;
            break;
        case 'B' :
            color = Bastoni;
            break;
        case 'C' :
            color = Coppe;
            break;
        default:
            return -2;
    }

    for (size_t i = 1; i < input.size(); ++i) {
        if (std::isdigit(input[i]))
        {
            _number = _number * 10 + (input[i] - '0');
        }
        else
        {
            switch(std::toupper(input[i]))
            {
                case 'B' :
                    call = Busso;
                    break;
                case 'S' :
                    call = Striscio;
                    break;
                case 'Q' :
                    call = ConQuestaBasta;
                    break;
                default:
                    call = NoCall;
                    break;
            }
            i = input.size(); // exit loop
        }
    }

    card = std::make_tuple(color, Cards::intToNumber(_number));

    move = {.card = card, .call = call, .playerId = playerId};

    LOG_DEBUG("move", Cards::CardToString(card));

    return 0;
}
