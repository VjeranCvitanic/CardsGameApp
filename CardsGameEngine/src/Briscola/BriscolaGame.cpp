#include "BriscolaGame.h"
#include "BriscolaRound.h"
#include "Logger.h"
#include "Cards.h"
#include <memory>

BriscolaGame_NS::BriscolaGame::BriscolaGame(const BriscolaGame_NS::BriscolaGameState& _gameState, int _numPlayers, const EventEmitter& _eventEmitter) :
    CardsGame(_gameState,  std::make_unique<BriscolaRuleState>(), HandSize, _numPlayers, _eventEmitter)
{
    rule().lastCard = gameState.deck.getCard(0);
    rule().strongColor = Cards::getColor(rule().lastCard);
    LOG_INFO("Strong color: ", Cards::ColorToString(rule().strongColor));
    LOG_INFO("Last card: ", Cards::CardToString(rule().lastCard));

    int numCards = 0;
    if(numPlayers == Two)
        numCards = 8;
    else if(numPlayers == Four)
        numCards = 12;

    dealCards(numCards);

    startNewRound();
}

void BriscolaGame_NS::BriscolaGame::updateGameResult()
{
    gameResult.points[currRound->roundResult.winnerId.first] += currRound->roundResult.points; // team points
}


bool BriscolaGame_NS::BriscolaGame::IsFinished()
{
    if(gameState.roundCnt > DECK_SIZE/handSize)
        return true;
    return false;
}

void BriscolaGame_NS::BriscolaGame::startNewRound()
{
    LOG_DEBUG("");
    if(numPlayers == Four && gameState.roundCnt == DECK_SIZE/handSize)
    {
        for(auto& p : gameState.players)
        {
            fullPlayerId teammateId = {p.playerId.first, (p.playerId.second + 2) % numPlayers};
            eventEmitter.emit(BriscolaLastRoundEvent(p.playerId, teammateId, gameState.players[teammateId.second].deck.getDeck(), gameResult.points[p.playerId.first]));
        }
    }

    BriscolaRound_NS::BriscolaRoundState roundState(rule().strongColor, gameState.nextToPlayId, gameState.players);
    currRound = std::make_unique<BriscolaRound_NS::BriscolaRound>(
        roundState,
        handSize,
        numPlayers,
        eventEmitter
    );
}

void BriscolaGame_NS::BriscolaGame::InitGame()
{
    LOG_INFO("Game init");
    eventEmitter.emit(StartBriscolaGameEvent(gameState.nextToPlayId, rule().lastCard));
}
