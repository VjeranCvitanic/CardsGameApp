# Python gRPC Client for CardsGame

This is a Python implementation of the CardsGame gRPC client, equivalent to the C++ client.

## Setup

## Automatic

## MacOs
./macOs_setup.sh
source venv/bin/activate
python python_client.py <name> <gameType> <numPlayers> <human|ai>

## windows
./windows_setup.ps1
source venv/bin/activate
python python_client.py <name> <gameType> <numPlayers> <human|ai>



## Manual

### 0. Create virtual environment

```bash
python3 -m venv venv
macOs: 
source venv/bin/activate
Windows:
venv\Scripts\Activate.ps1
```

### 1. Install Dependencies

```bash
pip install -r requirements.txt
```

### 2. Generate Python gRPC Code

Generate the Python protobuf and gRPC files from the `.proto` file:

```bash
cd gRPC/python_example
python -m grpc_tools.protoc -I../proto --python_out=. --grpc_python_out=. ../proto/cardsGame.proto
```

This will generate:
- `cardsGame_pb2.py` - Protobuf message classes
- `cardsGame_pb2_grpc.py` - gRPC service stubs

## Usage

Run the Python client with the same arguments as the C++ client:

```bash
python python_client.py <name> <gameType> <numPlayers> <human|ai>
```

**IMPORTANT:** You need to run **multiple clients** (2 for single, 4 for multi) to start a game. The server waits until all players connect before creating the game session.

### Arguments

- `<name>`: Player name (string)
- `<gameType>`: Game type - either `briscola` or `tressette`
- `<numPlayers>`: Number of players - either `2` (single) or `4` (multi)
- `<human|ai>`: Player type - `human` for manual input or `ai` for automatic random moves

### Examples

**Human player in 2-player Briscola:**
```bash
python python_client.py Alice briscola 2 human
```

**AI player in 4-player Tressette:**
```bash
python python_client.py BotBob tressette 4 ai
```

### Testing with Multiple Clients

To test a 2-player game, open **two terminals** and run:

**Terminal 1:**
```bash
cd gRPC/python_example
python python_client.py Alice briscola 2 ai
```

**Terminal 2:**
```bash
cd gRPC/python_example
python python_client.py Bob briscola 2 ai
```

The game will start once both clients connect. You can mix Python and C++ clients - they're compatible!

## Features

The Python client implements all features of the C++ client:

- ✅ Connect to lobby server
- ✅ Subscribe to game events
- ✅ Wait for session to start
- ✅ Process game events
- ✅ Play moves (both human and AI)
- ✅ Parse move input (e.g., `S1`, `D10B`)
- ✅ Handle event streams

## Move Input Format (Human Mode)

When it's your turn, enter moves in the format: `<Color><Number>[Call]`

**Color codes:**
- `S` - Spade
- `D` - Denari
- `B` - Bastoni
- `C` - Coppe

**Number:** 1-10

**Call (optional):**
- `B` - Busso
- `S` - Striscio
- `Q` - Con Questa Basta

**Examples:**
- `S1` - Asso of Spade
- `D10` - Re of Denari
- `B7B` - Sette of Bastoni with Busso call
