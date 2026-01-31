#include "TressetteGame.h"
#include "TressetteRound.h"
#include "Acussos.h"
#include "Logger.h"


TressetteGame_NS::TressetteGame::TressetteGame(const TressetteGame_NS::TressetteGameState& _gameState, int _numPlayers, const EventEmitter& _eventEmitter) :
    CardsGame(_gameState, std::make_unique<TressetteRuleState>(), _numPlayers, _numPlayers, _eventEmitter)
{
    dealCards(20);

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
        TeamId loserId = (winnerId.first + 1) % 2;

        if(rule().bastaCalled == winnerId)
        {    
            Points newPoints = gameResult.points[winnerId.first] + currRound->roundResult.points;
            if(newPoints.punta >= 41)
                gameResult.points[winnerId.first] = newPoints;
            else
                gameResult.points[loserId] += 11;
        }
        else {
            gameResult.points[loserId] += 11;
        }
    }
    else
        gameResult.points[winnerId.first] += currRound->roundResult.points;
}

bool TressetteGame_NS::TressetteGame::IsFinished()
{
    if(currRound->roundState.playedMovesInRound[0].call == ConQuestaBasta)
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

void TressetteGame_NS::TressetteGame::onEvent(const GameEvent& e)
{
    if (const auto* ev = std::get_if<BeforeFirstMoveEvent>(&e))
    {
        handleBeforeFirstMove(*ev);
    }
}

void TressetteGame_NS::TressetteGame::handleBeforeFirstMove(const BeforeFirstMoveEvent& e)
{
    if (acussoHandled.contains(e.playerId.second))
        return;

    Acussos acussoList;

    int pts = 0;
    Acussos_NS::CalculateAcussoPoints(gameState.players[e.playerId.second].deck.getDeck(), pts, acussoList);

    gameResult.points[e.playerId.first] += pts;

    eventEmitter.emit(AcussoEvent{
        e.playerId,
        acussoList
    });

    acussoHandled.insert(e.playerId.second);
}

void TressetteGame_NS::TressetteGame::postDealtCards(const std::vector<CardSet>& cards)
{
    for(int i = 0; i < numPlayers; i++)
    {
        eventEmitter.emit(TressetteDealtCardsEvent({i%2, i}, cards[i]));
    }
}