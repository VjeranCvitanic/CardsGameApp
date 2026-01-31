#pragma once

#include "cardsGame.pb.h"
#include "Events.h"

void PlayMoveReqToDomain(const cardsGame::PlayMoveReq& req, Move& move);
cardsGame::PlayMoveRsp MoveRspToProto(const MoveResponseEvent& move, cardsGame::PlayMoveRsp moveValidity);

void toDomainEvent(const cardsGame::GameEventMsg& msg);