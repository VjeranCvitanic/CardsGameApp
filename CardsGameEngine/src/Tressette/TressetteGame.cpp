#include "TressetteGame.h"
#include "TressetteRound.h"
#include "Acussos.h"
#include "Logger.h"


TressetteGame_NS::TressetteGame::TressetteGame(const TressetteGame_NS::TressetteGameState& _gameState, int _numPlayers, const EventEmitter& _eventEmitter) :
    CardsGame(_gameState, std::make_unique<TressetteRuleState>(), _numPlayers, _numPlayers, _eventEmitter)
{
    dealCards(10 * numPlayers);

    handleBeforeFirstMove();
    startNewRound();
}

void TressetteGame_NS::TressetteGame::updateGameResult()
{
    if(isLastRound())
        gameResult.points[currRound->roundResult.winnerId.first].punta++;
    auto& round = static_cast<TressetteRound_NS::TressetteRound&>(*currRound);

    rule().bastaCalled = round.bastaCalled;

    fullPlayerId winnerId = currRound->roundResult.winnerId;
    if(rule().bastaCalled.first != -1)
    {
        LOG_INFO("Con questa basta called by team ", rule().bastaCalled.first, ", player ", rule().bastaCalled.second);
        TeamId loserId = (winnerId.first + 1) % 2;

        if(rule().bastaCalled == winnerId)
        {    
            Points newPoints = gameResult.points[winnerId.first] + currRound->roundResult.points;
            LOG_INFO("Caller won round. New points would be: ", newPoints.punta);
            if(newPoints.punta >= 41)
            {
                LOG_INFO("Caller reached 41+, awarding points");
                gameResult.points[winnerId.first] = newPoints;
            }
            else
            {
                LOG_INFO("Caller failed to reach 41, giving other team +11");
                gameResult.points[loserId] += 11;
            }
        }
        else {
            LOG_INFO("Caller lost round, giving other team +11");
            gameResult.points[winnerId.first] += 11;
        }
    }
    else
        gameResult.points[winnerId.first] += currRound->roundResult.points;
}

bool TressetteGame_NS::TressetteGame::IsFinished()
{
    if(rule().bastaCalled.first != -1)
        return true;
    if(gameState.roundCnt > DECK_SIZE/handSize)
        return true;
    return false;
}

void TressetteGame_NS::TressetteGame::startNewRound()
{
    TressetteRound_NS::TressetteRoundState roundState(gameState.nextToPlayId, gameState.players);
    currRound = std::make_unique<TressetteRound_NS::TressetteRound>(
        roundState,
        numPlayers,
        eventEmitter
    );
}

void TressetteGame_NS::TressetteGame::handleBeforeFirstMove()
{
    LOG_DEBUG("handleBeforeFirstMove");
    for(auto& player : gameState.players)
    {
        Acussos acussoList = {};
        int pts = 0;

        Acussos_NS::CalculateAcussoPoints(player.deck.getDeck(), pts, acussoList);
        gameResult.points[player.playerId.first] += pts;

        if(acussoList.size() > 0)
        {
            eventEmitter.emit(AcussoEvent{
                player.playerId,
                acussoList
            });
        }
    }
}

void TressetteGame_NS::TressetteGame::postDealtCards(const std::vector<CardSet>& cards)
{
    for(int i = 0; i < numPlayers; i++)
    {
        eventEmitter.emit(TressetteDealtCardsEvent({i%2, i}, cards[i]));
    }
}