# CardsGame – Client Implementation Guide

## Architecture Overview

A client interacts with **two separate gRPC services** on **two separate ports**:

| Service | Port | Purpose |
|---|---|---|
| `CardsGameServer` | `50051` (fixed) | Lobby: connect, get session address |
| `CardsGameSession` | `50052+` (dynamic) | Play: subscribe to events, send moves |

**Connection sequence:**
1. Call `CardsGameServer::Connect` → receive `playerId` + `sessionAddress`
2. Connect to `sessionAddress`, call `CardsGameSession::SubscribeEvents` → open a **server-streaming RPC** that delivers all game events
3. When it's your turn (`YOUR_TURN_EVENT`), call `CardsGameSession::PlayMove`
4. Loop until the stream closes (session over)

---

## Proto Services Reference

### `CardsGameServer` (lobby, port 50051)

```protobuf
rpc Connect(ConnectReq) returns (ConnectRsp)
```

**Request:**
```protobuf
ConnectReq {
    string name             // player display name (1–30 chars, unique)
    repeated GameFormat gameFormats   // game types you support
    optional string message           // ignored by server
}
GameFormat { GameType gameType; SingleOrMulti singleOrMulti; }
```

**Response (oneof):**
```protobuf
ConnectRsp {
    oneof rsp {
        SuccessfullConn successMsg   // success path
        ConnFailReason  fail_reason  // EMPTY_NAME | INVALID_NAME_SIZE | NAME_ALREADY_TAKEN
    }
    optional string message
}
SuccessfullConn { int32 playerId; string address; }  // address = "host:port" of GameSession
```

---

### `CardsGameSession` (session, dynamic port)

```protobuf
rpc SubscribeEvents(PlayerInfo) returns (stream GameEventMsg)
rpc PlayMove(PlayMoveReq) returns (google.protobuf.Empty)
```

**SubscribeEvents request:**
```protobuf
PlayerInfo { int32 playerId; }  // the playerId from ConnectRsp
```

The stream delivers `GameEventMsg` until the session ends. The **first message is always `START_MATCH_EVENT`**.

**PlayMove request:**
```protobuf
PlayMoveReq {
    PlayerInfo playerInfo   // your playerId
    Move move               // card + optional call
    string message          // ignored
}
Move { Card card; Call call; }
Card { CardColor color; CardNumber number; }
```

---

## Event Reference

Every `GameEventMsg` has:
- `PlayerInfo playerInfo` – which session-local player this message is addressed to
- `EventType eventType` – discriminator for the `oneof eventMsg`

| EventType | Payload message | When | Who receives |
|---|---|---|---|
| `START_MATCH_EVENT` | `StartMatchMsg` | Session starts, first event | Each player (personalised) |
| `START_GAME_EVENT` | `StartGameMsg` | New game begins | All |
| `START_ROUND_EVENT` | `StartRoundMsg` | New round begins | All |
| `PLAYER_DEALT_CARDS_EVENT` | `PlayerDealtCardsMsg` | Cards dealt | **Briscola:** owner only · **Tressette:** everyone else |
| `YOUR_TURN_EVENT` | `YourTurnMsg` | It's your turn to play | You only |
| `MOVE_RSP_EVENT` | `PlayMoveRspMsg` | After you send `PlayMove` | You only |
| `PLAYER_PLAYED_MOVE_EVENT` | `PlayerPlayedMoveMsg` | A player just played | All except the mover |
| `ROUND_OVER_EVENT` | `RoundOverMsg` | Round finished | All |
| `GAME_OVER_EVENT` | `GameOverMsg` | Game finished | All |
| `MATCH_OVER_EVENT` | `MatchOverMsg` | Match finished | All |
| `ACUSSO_EVENT` | `AcussosMsg` | Tressette acusso declared | All except owner |
| `BRISCOLA_LAST_ROUND_EVENT` | `BriscolaLastRoundMsg` | Last round of 4-player Briscola | Receiver only |

---

## Key Design Notes

### Player ID vs Session Player ID

- `playerId` in `ConnectRsp` is the **server-assigned ID** (global, incrementing)
- After `SubscribeEvents`, the server maps each client to a **session-local ID** (0–3)
- All events inside the session use the **session-local ID** in `playerInfo.playerId`
- When calling `PlayMove`, you must send the **server-assigned ID** (what you got from `Connect`)

### Teams (4-player)

- Session IDs 0 and 2 are teammates (team 0)
- Session IDs 1 and 3 are teammates (team 1)
- `StartMatchMsg.teammateId = (mySessionId + 2) % 4`

### YOUR_TURN_EVENT fields

```protobuf
YourTurnMsg {
    PlayerInfo playerInfo       // your session ID
    repeated Card hand          // your current hand
    repeated Move movesPlayedInRound  // moves already played this round
    CardColor strongColor       // briscola color (NO_COLOR for tressette)
    repeated Card legalCards    // ONLY cards you are allowed to play
}
```
**Always play from `legalCards`**, not from `hand` directly. Playing an illegal card results in `MOVE_RSP_EVENT { CARD_NOT_IN_HAND }` or `COLOR_CONSTRAINT_NOT_MET`, and the game waits for a valid move.

### Move Calls (Tressette only)

| Call enum | Meaning | When allowed |
|---|---|---|
| `NO_CALL` | Plain play | Always |
| `BUSSO` | "I have high cards in this suit" | First card of a round only |
| `STRISCIO` | "I have more of this suit" | First card of a round only |
| `CON_QUESTA_BASTA` | "This is my last card of this suit" | First card of a round only |

---

## Minimal Client Lifecycle (pseudocode)

```
1.  channel_lobby  = connect("localhost:50051")
2.  rsp            = CardsGameServer.Connect(name, formats)
3.  playerId       = rsp.successMsg.playerId
4.  sessionAddr    = rsp.successMsg.address

5.  channel_sess   = connect(sessionAddr)
6.  stream         = CardsGameSession.SubscribeEvents(PlayerInfo{playerId})

7.  event = stream.read()   // always START_MATCH_EVENT
8.  mySessionId = event.playerInfo.playerId
9.  teammateId  = event.startMatch.teammateId

10. while event = stream.read():
        switch event.eventType:
            START_GAME_EVENT        → store gameFormat, briscolaColor
            START_ROUND_EVENT       → reset round state
            PLAYER_DEALT_CARDS_EVENT→ store hand (Briscola) or opponent hand info
            YOUR_TURN_EVENT         → choose card from legalCards, call PlayMove()
            MOVE_RSP_EVENT          → check MOVE_OK; on error retry PlayMove()
            PLAYER_PLAYED_MOVE_EVENT→ update game state
            ROUND_OVER_EVENT        → display result
            GAME_OVER_EVENT         → display game result
            MATCH_OVER_EVENT        → display final score, exit loop
            ACUSSO_EVENT            → note opponent's acussos (Tressette)
            BRISCOLA_LAST_ROUND_EVENT → reveal teammate's hand (Briscola 4p)
```
