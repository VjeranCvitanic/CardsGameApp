#include "CardsRound.h"
#include "Cards.h"
#include "Logger.h"

CardsRound_NS::CardsRound::CardsRound(RoundRules& rules, const RoundState& _state, int _handSize, int _numPlayers, const EventEmitter& _eventEmitter) :
    roundRules(rules),
    roundState(_state),
    eventEmitter(_eventEmitter),
    roundResult(),
    handSize(_handSize),
    numPlayers(_numPlayers)
{
    InitRound();
}

CardsRound_NS::RoundRules::RoundRules()
{}

CardsRound_NS::RoundResult::RoundResult() : 
    points(0), winnerId({-1, -1})
{}

CardsRound_NS::RoundState::RoundState(fullPlayerId _nextToPlayId, const Players& _players) : 
    nextToPlayId(_nextToPlayId),
    firstToPlayId(_nextToPlayId),
    players(_players),
    playedMovesInRound({}),
    moveConstraints({})
{
    LOG_DEBUG("RoundState ctor");
}

void CardsRound_NS::CardsRound::InitRound()
{
    logStartRound();
    eventEmitter.emit(StartRoundEvent(roundState.nextToPlayId));
    emitNextYourTurnEvent();
}

bool CardsRound_NS::CardsRound::IsFinished()
{
    if(roundState.playedMovesInRound.size() == handSize)
        return true;
    return false;
}

MoveReturnValue CardsRound_NS::CardsRound::ApplyMove(const Move& move)
{
    MoveReturnValue reason;

    if(roundRules.IsMoveLegal(move, roundState, reason))
    {
        preMoveSetup();
        playMove(move);
        postMoveSetup(move);

        if(IsFinished())
        {
            EndRound();
            return Finish;
        }
        else
        {
            emitNextYourTurnEvent();
            return Ok;
        }
    }
    else
    {
        LOG_DEBUG("Move not valid");
        emitMoveRspEvent(move, reason);
            
        return reason;
    }
}

void CardsRound_NS::CardsRound::playMove(const Move& move)
{
    roundState.playedMovesInRound.push_back(move);
}

void CardsRound_NS::CardsRound::emitNextYourTurnEvent()
{
    eventEmitter.emit(YourTurnEvent(roundState.nextToPlayId,
                roundState.players[roundState.nextToPlayId.second].deck.getDeck(), 
                roundState.playedMovesInRound, roundState.strongColor));
}

void CardsRound_NS::CardsRound::emitMoveRspEvent(const Move& move, MoveReturnValue moveValidity)
{
    eventEmitter.emit(MoveResponseEvent(move, moveValidity));
}

int8_t CardsRound_NS::CardsRound::HandWinner(const CardSet& playedHand, Card& winnerCard, Color strongColor)
{
    if(playedHand.size() != handSize)
    {
        LOG_ERROR("Invalid playedHand size: ", playedHand.size(), ", handsize: ", handSize);
        winnerCard = Cards::makeCard(InvalidColor, InvalidNumber);
        return -1;
    }

    return roundRules.StrongestCard(playedHand, winnerCard, strongColor);
}

void CardsRound_NS::CardsRound::EndRound()
{
    Card roundWinner;
    CardSet cards;

    Cards::getCardsFromMoves(roundState.playedMovesInRound, cards);
    PlayerId nextToPlayId = roundState.nextToPlayId.second;
    PlayerId playerId = (HandWinner(cards, roundWinner, roundState.strongColor) + nextToPlayId) % numPlayers;
    fullPlayerId winnerId = {playerId % 2, playerId};
    LOG_DEBUG("winnerId", winnerId);
    roundResult.winnerId = winnerId;
    roundResult.points = CalculateRoundResult();

    roundState.nextToPlayId = winnerId;

    eventEmitter.emit(RoundOverEvent(std::move(roundResult)));

    LOG_INFO("Round winner card: ", Cards::CardToString(roundWinner), "player: ", roundResult.winnerId, "round points: ", roundResult.points);
}

Points CardsRound_NS::CardsRound::CalculateRoundResult()
{
    Points roundValue;
    CardSet cards;
    Cards::getCardsFromMoves(roundState.playedMovesInRound, cards);

    for(auto& card : cards)
    {
        roundValue += roundRules.getNumberValue(Cards::getNumber(card));
    }

    return roundValue;
}

void CardsRound_NS::CardsRound::preMoveSetup()
{
    LOG_DEBUG("");
}

void CardsRound_NS::CardsRound::postMoveSetup(const Move& move)
{
    PlayerId currPlayedId = roundState.nextToPlayId.second;
    PlayerId nextToPlayId = (currPlayedId + 1) % numPlayers;

    roundState.nextToPlayId = {nextToPlayId % 2, nextToPlayId};
    roundState.players[currPlayedId].deck.eraseCard(move.card);

    emitMoveRspEvent(move, MoveReturnValue::Ok);
    GameEvent event = PlayerPlayedMoveEvent(move);
    eventEmitter.emit(event);
}

void CardsRound_NS::CardsRound::logStartRound()
{
    LOG_INFO("Round starting");
    for(auto& p : roundState.players)
    {
        LOG_DEBUG("Player ", p.playerId, " hand: ");
        p.deck.logDeck();
    }
    LOG_DEBUG("Next to play: ", roundState.nextToPlayId);
}

int8_t CardsRound_NS::RoundRules::StrongestCard(const CardSet& playedHand, Card& winnerCard, Color strongColor) const
{
    if(playedHand.empty())
    {
        LOG_ERROR("Empty playedHand");
        winnerCard = Cards::makeCard(InvalidColor, InvalidNumber);
        return -1;
    }

    Card winner = playedHand[0];
    int winnerPos = 0;

    for(int i = 1; i < playedHand.size(); i++)
    {
        Card w = StrongerCard(winner, playedHand[i], strongColor);
        if(w != winner)
        {
            winner = w;
            winnerPos = i;
        }
    }

    winnerCard = winner;
    return winnerPos;
}