#include "ProtoToDomain.h"
#include "Cards.h"
#include "Types.h"


void toDomainEvent(const cardsGame::GameEventMsg& msg) {
    std::cout << "Received event type: " << msg.eventtype() << std::endl;
}

void PlayMoveReqToDomain(const cardsGame::PlayMoveReq& req, Move& move)
{
    // Convert player ID
    move.playerId.second = atoi(req.playerid().c_str());
    move.playerId.first = move.playerId.second % 2;

    // Convert card
    const cardsGame::Card& protoCard = req.move().card();
    move.card = Cards::makeCard(static_cast<Color>(protoCard.color()), static_cast<Number>(protoCard.number()));

    // Convert call
    move.call = static_cast<Call>(req.move().call());

    std::cout << "Move: player=" << move.playerId.second << ", card=" << Cards::CardToString(move.card) << ", call=" << move.call << std::endl;
}

void MoveRspToProto(const MoveResponseEvent& moveRsp, cardsGame::PlayMoveRsp& rsp)
{
    MoveReturnValue val = moveRsp.moveValidity;
    if(moveRsp.moveValidity == Finish) // proto diff
    {
        val = MoveReturnValue::Ok;
    }

    rsp.set_moversp(static_cast<cardsGame::MoveRsp>(val));
}