# CardsGame – Client ↔ Server ↔ GameSession Sequence Diagrams

Rendered with [Mermaid](https://mermaid.js.org/).  
Open in any Mermaid-aware viewer (VS Code extension, GitHub, mermaid.live).

---

## 1. Connection & Session Setup

```mermaid
sequenceDiagram
    autonumber
    participant C1 as Client 1
    participant C2 as Client 2
    participant SRV as Server (CardsGameServer)
    participant GS as GameSession (CardsGameSession)
    participant ENG as CardsMatch (Engine)

    C1->>SRV: Connect(name, [GameFormat{BRISCOLA, SINGLE}])
    SRV-->>C1: ConnectRsp{playerId=0, address="localhost:50052"}

    C2->>SRV: Connect(name, [GameFormat{BRISCOLA, SINGLE}])
    note over SRV: waitingClients queue full → maybeCreateGameSession()
    SRV->>GS: new GameSession(port=50052, players=[0,1])\n+ StartSession() in new thread
    SRV-->>C2: ConnectRsp{playerId=1, address="localhost:50052"}

    note over C1,C2: Both clients now connect to GameSession on port 50052
    C1->>GS: SubscribeEvents(PlayerInfo{playerId=0})
    GS-->>C1: stream: START_MATCH_EVENT {sessionPlayerId=0, teammateId=-1, gameFormat}
    C2->>GS: SubscribeEvents(PlayerInfo{playerId=1})
    GS-->>C2: stream: START_MATCH_EVENT {sessionPlayerId=1, teammateId=-1, gameFormat}

    note over GS: All players connected → startMatch()
    GS->>ENG: createBriscolaMatch(eventEmitter, numPlayers=2)
    ENG-->>GS: (match created, game begins)
```

---

## 2. Game Flow – Event Stream (Briscola 2-player, single round)

> Events emitted by the Engine are received by `GameSession` via `EventSink::onEvent()`,
> translated to proto messages, and forwarded over each player's `SubscribeEvents` stream.

```mermaid
sequenceDiagram
    autonumber
    participant C1 as Client 1 (session ID 0)
    participant C2 as Client 2 (session ID 1)
    participant GS as GameSession
    participant ENG as CardsMatch (Engine)

    ENG->>GS: StartBriscolaGameEvent {firstToPlay=0, lastCard=Briscola}
    GS-->>C1: stream: START_GAME_EVENT {firstToPlayId, gameFormat, lastCard(briscola)}
    GS-->>C2: stream: START_GAME_EVENT {firstToPlayId, gameFormat, lastCard(briscola)}

    ENG->>GS: PlayerDealtCardsEvent {playerId=0, cards=[...10 cards]}
    GS-->>C1: stream: PLAYER_DEALT_CARDS_EVENT {cards}

    ENG->>GS: PlayerDealtCardsEvent {playerId=1, cards=[...10 cards]}
    GS-->>C2: stream: PLAYER_DEALT_CARDS_EVENT {cards}

    loop Each Round
        ENG->>GS: StartRoundEvent {firstToPlayId}
        GS-->>C1: stream: START_ROUND_EVENT {firstToPlayId}
        GS-->>C2: stream: START_ROUND_EVENT {firstToPlayId}

        ENG->>GS: YourTurnEvent {playerId=0, hand, legalCards, movesPlayedInRound}
        GS-->>C1: stream: YOUR_TURN_EVENT {hand, legalCards, movesInRound, strongColor}

        C1->>GS: PlayMove(PlayMoveReq{playerId=0, move={card, call}})
        GS->>ENG: match->ApplyMove(move)

        ENG->>GS: MoveResponseEvent {move, moveValidity=OK}
        GS-->>C1: stream: MOVE_RSP_EVENT {MOVE_OK}

        ENG->>GS: PlayerPlayedMoveEvent {move{card, call, playerId=0}}
        note over GS: Sent to all players EXCEPT the one who played
        GS-->>C2: stream: PLAYER_PLAYED_MOVE_EVENT {playerId=0, card, call}

        ENG->>GS: YourTurnEvent {playerId=1, hand, legalCards, movesPlayedInRound}
        GS-->>C2: stream: YOUR_TURN_EVENT {hand, legalCards, movesInRound, strongColor}

        C2->>GS: PlayMove(PlayMoveReq{playerId=1, move={card, call}})
        GS->>ENG: match->ApplyMove(move)

        ENG->>GS: MoveResponseEvent {move, moveValidity=OK}
        GS-->>C2: stream: MOVE_RSP_EVENT {MOVE_OK}

        ENG->>GS: PlayerPlayedMoveEvent {move{card, call, playerId=1}}
        GS-->>C1: stream: PLAYER_PLAYED_MOVE_EVENT {playerId=1, card, call}

        ENG->>GS: RoundOverEvent {winnerId, points{punti, bella}}
        GS-->>C1: stream: ROUND_OVER_EVENT {winnerId, points}
        GS-->>C2: stream: ROUND_OVER_EVENT {winnerId, points}
    end

    ENG->>GS: GameOverEvent {gameResult{winnerId, points[team0, team1]}}
    GS-->>C1: stream: GAME_OVER_EVENT {teamWinnerId, teamPoints[]}
    GS-->>C2: stream: GAME_OVER_EVENT {teamWinnerId, teamPoints[]}

    ENG->>GS: MatchOverEvent {matchResult{winnerId, score[]}}
    GS-->>C1: stream: MATCH_OVER_EVENT {teamWinnerId, score[]}
    GS-->>C2: stream: MATCH_OVER_EVENT {teamWinnerId, score[]}

    note over GS: IsSessionOver() == true → SubscribeEvents() returns OK
    note over C1,C2: reader->Read() returns false → stream closed
```

---

## 3. Tressette-specific extras (4-player / MULTI)

```mermaid
sequenceDiagram
    autonumber
    participant C0 as Client (session 0)
    participant C2 as Client (session 2, teammate of 0)
    participant C1 as Client (session 1)
    participant C3 as Client (session 3, teammate of 1)
    participant GS as GameSession
    participant ENG as CardsMatch

    note over GS: START_MATCH_EVENT also carries teammateId = (sessionId+2)%4

    ENG->>GS: TressetteDealtCardsEvent {playerId=0, dealtCards}
    note over GS: TressetteDealtCards → broadcast to ALL OTHER players (not the owner)
    GS-->>C1: stream: PLAYER_DEALT_CARDS_EVENT {playerId=0, cards}
    GS-->>C2: stream: PLAYER_DEALT_CARDS_EVENT {playerId=0, cards}
    GS-->>C3: stream: PLAYER_DEALT_CARDS_EVENT {playerId=0, cards}

    note over GS: (repeated for players 1, 2, 3)

    ENG->>GS: AcussoEvent {playerId=0, acussos=[NAPOLITANA_SPADE]}
    note over GS: ACUSSO_EVENT → broadcast to ALL except the owning player
    GS-->>C1: stream: ACUSSO_EVENT {acussoPlayerId=0, acussos}
    GS-->>C2: stream: ACUSSO_EVENT {acussoPlayerId=0, acussos}
    GS-->>C3: stream: ACUSSO_EVENT {acussoPlayerId=0, acussos}
```

---

## 4. Invalid Move Handling

```mermaid
sequenceDiagram
    autonumber
    participant C1 as Client 1
    participant GS as GameSession
    participant ENG as CardsMatch

    GS-->>C1: stream: YOUR_TURN_EVENT {hand, legalCards}

    C1->>GS: PlayMove(PlayMoveReq{card not in legalCards})
    GS->>ENG: match->ApplyMove(move)
    ENG-->>GS: MoveReturnValue = CardNotInHand (or ColorConstraintNotMet)
    ENG->>GS: MoveResponseEvent {moveValidity = CardNotInHand}

    GS-->>C1: stream: MOVE_RSP_EVENT {CARD_NOT_IN_HAND}
    note over C1: Client must retry – GS does NOT advance the game
```

---

## 5. Briscola 4-player – Last Round teammate hand reveal

```mermaid
sequenceDiagram
    autonumber
    participant C0 as Client (session 0)
    participant C2 as Client (session 2, teammate)
    participant GS as GameSession
    participant ENG as CardsMatch

    note over ENG: Last round begins – engine reveals teammate hand
    ENG->>GS: BriscolaLastRoundEvent {receiver=0, sender=2, senderHand=[...]}
    note over GS: Sent ONLY to the receiver player
    GS-->>C0: stream: BRISCOLA_LAST_ROUND_EVENT {teammateId=2, cards=[...]}
```

---

## 6. Routing Summary

| Event | Visibility | Who receives it |
|---|---|---|
| `START_MATCH_EVENT` | Broadcast (on subscribe) | Each player (personalised) |
| `START_GAME_EVENT` | Broadcast | All players |
| `START_ROUND_EVENT` | Broadcast | All players |
| `PLAYER_DEALT_CARDS_EVENT` (Briscola) | Private | Owner only |
| `PLAYER_DEALT_CARDS_EVENT` (Tressette) | Broadcast | All **except** owner |
| `YOUR_TURN_EVENT` | Private | Active player only |
| `MOVE_RSP_EVENT` | Private | Moving player only |
| `PLAYER_PLAYED_MOVE_EVENT` | Broadcast | All **except** mover |
| `ROUND_OVER_EVENT` | Broadcast | All players |
| `GAME_OVER_EVENT` | Broadcast | All players |
| `MATCH_OVER_EVENT` | Broadcast | All players |
| `ACUSSO_EVENT` | Broadcast | All **except** owner |
| `BRISCOLA_LAST_ROUND_EVENT` | Team (Private) | Receiver only |
