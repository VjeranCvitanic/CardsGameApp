#include "CardsGame.h"
#include "Logger.h"
#include <unordered_map>
#include <vector>


CardsGame_NS::CardsGame::CardsGame(const CardsGame_NS::GameState& _gameState, std::unique_ptr<IRuleState> _ruleState, int _handSize, int _numPlayers, const EventEmitter& _eventEmitter) :
    gameState(_gameState), ruleState(std::move(_ruleState)), handSize(_handSize), numPlayers(_numPlayers), eventEmitter(_eventEmitter)
{
    LOG_DEBUG("CardsGame ctor");
    InitGame();
}

CardsGame_NS::GameState::GameState(fullPlayerId _nextToPlayId, const CardsRound_NS::Players& _players) :
    nextToPlayId(_nextToPlayId), players(_players)
{
    LOG_DEBUG("GameState ctor");
    deck = Deck(true);
    roundCnt = 1;
}

void CardsGame_NS::CardsGame::dealCards(int8_t numCards)
{

    dealCardsImpl(numCards, nullptr);
}

void CardsGame_NS::CardsGame::dealCards(int8_t numCards, std::vector<CardSet>& vector)
{
    dealCardsImpl(numCards, &vector);
}

void CardsGame_NS::CardsGame::dealCardsImpl(int8_t numCards, std::vector<CardSet>* out)
{
    LOG_DEBUG("Deal cards: ", +numCards);
    if(gameState.deck.getDeck().size() < numCards)
    {
        LOG_ERROR("Deck too small");
        return;
    }
    CardSet drawnCards = drawCards(numCards);

    PlayerId playerId = gameState.nextToPlayId.second;

    std::vector<CardSet> dealtCards;
    dealtCards.resize(numPlayers);
    if(out)
        out->resize(numCards);

    for(int i = 0; i < numCards; i++)
    {
        Card card = drawnCards[i];

        gameState.players[playerId].deck.AddCard(card);
        dealtCards[playerId].push_back(card);
        playerId = (playerId + 1) % numPlayers;

        if (out)
            (*out)[playerId].push_back(card);
    }

    for(int playerId = 0; playerId < numPlayers; playerId++)
    {
        eventEmitter.emit(PlayerDealtCardsEvent({playerId%2, playerId}, {dealtCards[playerId]}));
    }
}

CardSet CardsGame_NS::CardsGame::drawCards(int8_t numCards)
{
    CardSet drawnCards;
    for(int i = 0; i < numCards; i++)
    {
        drawnCards.push_back(gameState.deck.popCard());
    }

    return drawnCards;
}

bool CardsGame_NS::CardsGame::isLastRound()
{
    if(gameState.roundCnt == DECK_SIZE / handSize)
        return true;
    return false;
}

void CardsGame_NS::CardsGame::postDealtCards(const std::vector<CardSet>& cards)
{}

MoveReturnValue CardsGame_NS::CardsGame::postMove(MoveReturnValue roundRetVal)
{
    if(roundRetVal == Finish)
    {
        updateGameResult();
        LOG_INFO("Game result so far: ", gameResult.points[0], " : ", gameResult.points[1]);
        gameState.roundCnt++;
        gameState.players = std::move(currRound->roundState.players);
        gameState.nextToPlayId = currRound->roundResult.winnerId;

        if(IsFinished())
        {
            EndGame();
            return Finish;
        }

        if(gameState.deck.getDeck().size() > 0)
        {
            std::vector<CardSet> dealtCards;
            dealCards(handSize, dealtCards);
            postDealtCards(dealtCards);
        }

        startNewRound();
    }

    return roundRetVal;
}

MoveReturnValue CardsGame_NS::CardsGame::ApplyMove(const Move& move)
{
    MoveReturnValue roundRetVal = currRound->ApplyMove(move);

    return postMove(roundRetVal);
}

void CardsGame_NS::CardsGame::InitGame()
{
    LOG_INFO("Game init");
    eventEmitter.emit(StartGameEvent(gameState.nextToPlayId));
}

void CardsGame_NS::CardsGame::EndGame()
{
    LOG_INFO("Game End");

    if(gameResult.points.at(0).punta > gameResult.points.at(1).punta)
        gameResult.winnerId = 0;
    else
        gameResult.winnerId = 1;

    eventEmitter.emit(GameOverEvent(std::move(gameResult)));
}
   
