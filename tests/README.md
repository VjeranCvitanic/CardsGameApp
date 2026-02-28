# CardsGame – Test Suite

## Quick Reference

```bash
# Run all unit tests (fastest - no dependencies)
./tests/build/unit_tests

# Run specific test suites
./tests/build/unit_tests --gtest_filter="BriscolaRules_*"
./tests/build/unit_tests --gtest_filter="TressetteRules_*"
./tests/build/unit_tests --gtest_filter="Deck_*"

# Run with verbose output
./tests/build/unit_tests --gtest_output=xml:results.xml

# List all available tests
./tests/build/unit_tests --gtest_list_tests

# Rebuild + run
cmake --build tests/build -j4 && ./tests/build/unit_tests
```

---

## Test Layers

| Layer | Location | Framework | What it tests | Speed |
|---|---|---|---|---|
| **Unit** | `unit/` | GoogleTest | Pure engine rule logic – no network, no events | < 1s |
| **Block** | `block/` | GoogleTest + GMock | GameSession event→proto translation & routing | < 5s |
| **End-to-End** | `e2e/` | pytest | Full game flow over real gRPC with AI clients | ~30s |

---

## Build & Run (fully automated via CMake + CTest)

### One-time setup

```bash
# Install Python deps for e2e tests
pip install grpcio grpcio-tools pytest

# Generate Python proto stubs (re-run after any .proto change)
python -m grpc_tools.protoc \
    -I gRPC/proto \
    --python_out=tests/e2e \
    --grpc_python_out=tests/e2e \
    gRPC/proto/cardsGame.proto
```

### Configure (once)

```bash
# From repo root – point CMAKE_PREFIX_PATH at your gRPC+Protobuf install
cmake -S tests -B tests/build \
      -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_PREFIX_PATH=/usr/local   # or wherever vcpkg/brew installed gRPC
```

### Build + run ALL tests with one command

```bash
# Auto-detect CPU count (works on all platforms)
cmake --build tests/build -j && \
ctest --test-dir tests/build --output-on-failure

# Or specify manually
cmake --build tests/build -j4 && \
ctest --test-dir tests/build --output-on-failure
```

`gtest_discover_tests()` in `tests/CMakeLists.txt` registers every `TEST()` macro as a
separate CTest test case automatically — no manual registration needed.

### Run specific subsets

```bash
# All unit tests (fastest, no network)
ctest --test-dir tests/build -R unit_ --output-on-failure

# All block tests
ctest --test-dir tests/build -R block_ --output-on-failure

# Single test suite by name
ctest --test-dir tests/build -R BriscolaRules --output-on-failure
ctest --test-dir tests/build -R TressetteRules --output-on-failure
ctest --test-dir tests/build -R Deck --output-on-failure

# Run binaries directly for richer output / filtering
./tests/build/unit_tests --gtest_filter="BriscolaRules_Strength.*"
./tests/build/unit_tests --gtest_filter="TressetteRules_IsMoveLegal.*"
./tests/build/unit_tests --gtest_filter="Deck_FullDeck.*"
./tests/build/unit_tests --gtest_output=xml:results.xml
```

### Python end-to-end tests (require running server)

**One-time Python setup (macOS with externally-managed Python):**
```bash
# Create virtual environment
python3 -m venv tests/venv
source tests/venv/bin/activate

# Install dependencies
pip install grpcio grpcio-tools pytest

# Generate Python proto stubs
python -m grpc_tools.protoc \
    -I gRPC/proto \
    --python_out=tests/e2e \
    --grpc_python_out=tests/e2e \
    gRPC/proto/cardsGame.proto
```

**Build server:**
```bash
cmake -S gRPC -B gRPC/build -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_PREFIX_PATH=/usr/local
cmake --build gRPC/build -j
```

**Run e2e tests:**
```bash
# Activate venv first
source tests/venv/bin/activate

# Run all e2e tests (server auto-started/stopped by pytest fixture)
SERVER_BIN=gRPC/build/server pytest tests/e2e/ -v

# Subsets
SERVER_BIN=gRPC/build/server pytest tests/e2e/ -v -k TestConnectValidation
SERVER_BIN=gRPC/build/server pytest tests/e2e/ -v -k TestBriscola2Player
```

### CI / automated pipeline

CTest supports parallel execution and JUnit XML output for CI systems:

```bash
cmake --build tests/build -j
ctest --test-dir tests/build \
      --output-on-failure \
      --parallel 4 \
      --output-junit tests/build/results.xml
```

---

## Test Descriptions

### Unit: `unit/test_briscola_rules.cpp`

Tests `BriscolaRules` in complete isolation — no game state, no match, no network.

Strength order: **Due(1) < Quattro(2) < … < Tre(9) < Asso(10)**  
*(Due=2 is the weakest card in Briscola — opposite of most games)*

| Test | What it checks |
|---|---|
| `AceIsStrongest` | Asso (strength 10) beats everything |
| `ThreeIsSecondStrongest` | Tre (9) beats Re, Cavallo, Due; loses to Asso |
| `DueIsWeakest` | Due (1) loses to all other numbers |
| `QuattroIsSecondWeakest` | Quattro (2) only beats Due |
| `AceWorth11` … `SevenWorth0` | Point values (A=11, 3=10, K=4, Q=3, J=2, rest=0) |
| `AceBeatsSeven_SameSuit` | StrongerCard same-suit: Asso beats Sette |
| `KingBeatsDue_SameSuit` | Re (8) beats Due (1) same suit |
| `AceBeatsKing_SameSuit` | Asso beats Re same suit |
| `BriscolaBeatsSameSuitAce` | Any briscola (even Quattro) beats off-suit Asso |
| `BriscolaAceBeatsAllBriscola` | Briscola Asso beats all other briscola cards |
| `AnyCardLegal_FirstMove` | No constraint → full hand is playable |
| `CardNotInHand_Rejected` | Card not in hand → `CardNotInHand` |
| `FullHandIsLegal_NoConstraint` | `calculateLegalMoves` returns full hand with no constraint |

### Unit: `unit/test_tressette_rules.cpp`

Tests `TressetteRules` in complete isolation.

Strength order: **Quattro(1) < Cinque(2) < Sei(3) < Sette(4) < Fante(5) < Cavallo(6) < Re(7) < Asso(8) < Due(9) < Tre(10)**  
No trump suit. Must follow lead suit if able.

| Test | What it checks |
|---|---|
| `TreIsStrongest` | Tre (10) beats all |
| `DueIsSecondStrongest` | Due (9) beats Asso, Re; loses to Tre |
| `AssoIsThirdStrongest` | Asso (8) beats Re; loses to Due, Tre |
| `QuattroIsWeakest` | Quattro (1) loses to all |
| `FullOrderIsMonotonic` | Complete strength chain is strict ascending |
| `AssoWorth1Punto` | Asso = 1 punta |
| `TreWorth1Bella` … `FanteWorth1Bella` | Tre/Due/Re/Cavallo/Fante = 1 bella each |
| `NumeralsWorthNothing` | 4,5,6,7 = 0 points |
| `TreBeatsAsso_SameSuit` | StrongerCard: Tre beats Asso same suit |
| `FirstCardWins_DifferentSuits` | Off-suit never wins; first card always returned |
| `DueBeatsRe_SameSuit` | Due (9) beats Re (7) same suit |
| `NoConstraint_AnyCardIsLegal` | No lead color → any card legal |
| `MustFollowSuit_WhenAble` | Has lead suit → must play it |
| `CanPlayAnySuit_WhenVoidInLeadSuit` | No lead suit in hand → any card legal |
| `CardNotInHand_Rejected` | Card not in hand → `CardNotInHand` |
| `Call_OnlyLegalOnFirstCardOfTrick` | Busso/Striscio/ConQuestaBasta illegal after first card |
| `NoConstraint_FullHand` | `calculateLegalMoves` returns full hand |
| `Constraint_OnlyMatchingSuit` | Returns only cards matching lead color |
| `VoidInLeadSuit_FullHandLegal` | No matching cards → full hand returned |

### Unit: `unit/test_deck_and_cards.cpp`

Tests `Deck` construction and manipulation, and `Cards` utility functions.

| Test | What it checks |
|---|---|
| `ColorAndNumberRoundTrip` | `makeCard`/`getColor`/`getNumber` round-trip for all 40 cards |
| `IsCardInCardSet_Found/NotFound/EmptySet` | Membership check edge cases |
| `GetCardsFromMoves_*` | Extracts cards from `Moves` list correctly |
| `DefaultIsEmpty / BoolFalseIsEmpty` | Empty deck constructors |
| `HasExactly40Cards` | Full deck has exactly 40 cards |
| `AllCardsUnique` | No duplicates in full deck |
| `ContainsAllColorsAndNumbers` | All 4×10 combinations present |
| `HasFourSuits / TenCardsPerSuit` | Suit distribution is correct |
| `AddCard_*` | Size increments, card is findable after add |
| `EraseCard_*` | Card removed; erasing non-existent is no-op |
| `EraseDeck` | Deck empty after clear |
| `PopCard_*` | Returns back element; empty deck returns invalid card |
| `Copy_IsIndependent` | Deck copy doesn't share state with original |
| `PopOrderIsLIFO` | `popCard()` returns last-added card |
| `ShuffledVariants_AreStillComplete` | Two `Deck(true)` instances contain identical card sets |

### Block: `block/test_game_session_events.cpp`

Tests the GameSession event-translation and routing layer. Each test
documents the exact assertion once a test seam is added to `GameSession`.

**Required code change to enable block tests (one line in `GameSession.h`):**
```cpp
// Add inside GameSession class:
friend class TestableGameSession;
// AND expose the connections map / connectionsMutex as protected:
```

| Test | What it checks |
|---|---|
| `StartRoundEvent_BroadcastToAll` | Both players receive START_ROUND_EVENT |
| `YourTurnEvent_PrivateToOwnerOnly` | Only the addressed player receives YOUR_TURN_EVENT |
| `PlayerDealtCards_Briscola_PrivateToOwner` | Briscola deal goes only to owner |
| `PlayerPlayedMove_NotSentToMover` | PLAYER_PLAYED_MOVE_EVENT skips the mover |
| `MoveRspEvent_OnlySentToMover` | MOVE_RSP_EVENT goes only to the mover |
| `RoundOverEvent_BroadcastToAll` | Both players receive ROUND_OVER_EVENT |
| `DomainMoveConversion_CorrectCard` | PlayMoveReqToDomain maps proto→domain correctly |

### E2E: `e2e/test_full_game.py`

Runs real server + 2 AI clients. Tests:

| Class | Test | What it checks |
|---|---|---|
| `TestBriscola2Player` | `test_both_receive_start_match` | Both clients get START_MATCH_EVENT |
| | `test_session_ids_are_different` | Players get distinct session IDs (0 or 1) |
| | `test_game_type_is_briscola` | GameFormat in START_MATCH has BRISCOLA |
| | `test_start_game_has_briscola_card` | START_GAME_EVENT contains lastCard (briscola suit) |
| | `test_both_receive_dealt_cards` | Each client receives dealt cards |
| | `test_your_turn_contains_legal_cards` | YOUR_TURN_EVENT always has ≥1 legal card |
| | `test_move_responses_are_ok` | All move responses are MOVE_OK (AI plays legal cards) |
| | `test_game_ends_with_match_over` | Both clients receive MATCH_OVER_EVENT |
| | `test_match_over_declares_winner` | MATCH_OVER_EVENT includes teamWinnerId |
| | `test_event_sequence_order` | Correct ordering: MATCH→GAME→DEAL→ROUND→TURN→ROUND_OVER→MATCH_OVER |
| | `test_player_played_move_not_sent_to_mover` | No client receives its own PLAYER_PLAYED_MOVE |
| | `test_no_unexpected_events` | No unrecognised event types in stream |
| `TestConnectValidation` | `test_empty_name_rejected` | gRPC INVALID_ARGUMENT on empty name |
| | `test_name_too_long_rejected` | gRPC INVALID_ARGUMENT on name > 30 chars |
| | `test_duplicate_name_rejected` | gRPC ALREADY_EXISTS on duplicate name |
| | `test_successful_connect_returns_player_id_and_address` | playerId ≥ 0, address has host:port |

---

## How an expert approaches testing this system

### Principle: test at the lowest possible layer

```
Unit ──────────────── Fast, deterministic, no deps
   BriscolaRules      → card values, StrongerCard, IsMoveLegal, legalMoves

Block ─────────────── Isolated, no real network
   GameSession        → event→proto translation, routing logic
   Server.Connect     → client validation, session creation logic

E2E ───────────────── Slow, real system
   2p Briscola game   → full happy path
   Connect errors     → server-side rejection
```

### What to add next

1. **Unit**: `TressetteRules` — suit-following constraint, acusso detection
2. **Unit**: `CardsGame.ApplyMove` — test `Finish` return value after last round
3. **Block**: Tressette deal broadcast (opposite of Briscola — everyone but owner)
4. **Block**: `AcussoEvent` routing (everyone except owner)
5. **Block**: `BriscolaLastRoundEvent` routing (only to designated receiver)
6. **E2E**: 4-player Briscola match
7. **E2E**: 2-player Tressette match
8. **E2E**: Concurrent sessions (2 pairs joining simultaneously)
9. **E2E**: Client disconnects mid-game (stream cancellation)
