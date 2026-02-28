#!/usr/bin/env python3
"""
Python gRPC client for CardsGame - equivalent to the C++ client.
"""

import grpc
import sys
import random
import threading
from typing import Optional
from datetime import timedelta

# Import generated protobuf/gRPC modules
# These will be generated from cardsGame.proto using:
# python -m grpc_tools.protoc -I./proto --python_out=. --grpc_python_out=. ./proto/cardsGame.proto
try:
    import cardsGame_pb2 as pb2
    import cardsGame_pb2_grpc as pb2_grpc
    from google.protobuf import empty_pb2
except ImportError:
    print("Error: gRPC Python modules not found. Generate them with:")
    print("python -m grpc_tools.protoc -I./proto --python_out=. --grpc_python_out=. ./proto/cardsGame.proto")
    sys.exit(1)


class CardsGameClient:
    """Python gRPC client for CardsGame"""
    
    def __init__(self, lobby_address: str, name: str, is_ai: bool, game_format: pb2.GameFormat):
        """
        Initialize the client.
        
        Args:
            lobby_address: Address of the lobby server (e.g., "localhost:50051")
            name: Player name
            is_ai: Whether this is an AI player
            game_format: Game format (type and single/multi)
        """
        self.lobby_address = lobby_address
        self.name = name
        self.is_ai = is_ai
        self.game_format = game_format
        
        # Create lobby stub
        self.lobby_channel = grpc.insecure_channel(lobby_address)
        self.lobby_stub = pb2_grpc.CardsGameServerStub(self.lobby_channel)
        
        # Session stub will be created after connection
        self.session_stub: Optional[pb2_grpc.CardsGameSessionStub] = None
        self.session_channel: Optional[grpc.Channel] = None
        
        self.player_id: Optional[int] = None
        self.game_session_address: Optional[str] = None
    
    def connect(self, name: str, game_type: pb2.GameType, single_or_multi: pb2.SingleOrMulti) -> bool:
        """
        Connect to the lobby server.
        
        Args:
            name: Player name
            game_type: Type of game (BRISCOLA or TRESSETTE)
            single_or_multi: SINGLE (2 players) or MULTI (4 players)
            
        Returns:
            True if connection successful, False otherwise
        """
        request = pb2.ConnectReq()
        request.name = name
        game_format = request.gameFormats.add()
        game_format.gameType = game_type
        game_format.singleOrMulti = single_or_multi
        
        try:
            response = self.lobby_stub.Connect(request)
            
            if response.HasField('successMsg'):
                print(f"My id: {response.successMsg.playerId}")
                self.player_id = response.successMsg.playerId
                self.game_session_address = response.successMsg.address
                
                # Create session channel with wait_for_ready option
                options = [
                    ('grpc.wait_for_ready', 1),
                ]
                self.session_channel = grpc.insecure_channel(
                    self.game_session_address,
                    options=options
                )
                self.session_stub = pb2_grpc.CardsGameSessionStub(self.session_channel)
                
                return True
            else:
                print(f"RPC failed: {response.fail_reason}")
                return False
                
        except grpc.RpcError as e:
            print(f"RPC failed: {e.details()}")
            return False
    
    def wait_for_session_started(self, event_stream: grpc.CallIterator) -> bool:
        """
        Wait for the session to start by reading from the event stream.
        
        Args:
            event_stream: The event stream to read from
            
        Returns:
            True if START_MATCH_EVENT received, False otherwise
        """
        try:
            # Wait for START_MATCH_EVENT
            for event in event_stream:
                if event.eventType == pb2.START_MATCH_EVENT:
                    print(f"[EVENT] {event}")
                    return True
            
            print("Stream ended without receiving START_MATCH_EVENT")
            return False
            
        except grpc.RpcError as e:
            print(f"Failed to read event stream: {e.details()}")
            return False
    
    def play_move(self, move: pb2.Move) -> None:
        """
        Send a move to the server.
        
        Args:
            move: The move to play
        """
        if not self.session_stub:
            print("Session stub is null")
            return
        
        request = pb2.PlayMoveReq()
        request.playerInfo.playerId = self.player_id
        request.move.CopyFrom(move)
        
        try:
            self.session_stub.PlayMove(request)
        except grpc.RpcError as e:
            print(f"PlayMove failed: {e.details()}")
    
    def start_client(self) -> None:
        """Start the client and begin the event loop."""
        if not self.connect(self.name, self.game_format.gameType, self.game_format.singleOrMulti):
            print("Failed to connect")
            return
        
        # Create event stream
        player_info = pb2.PlayerInfo()
        player_info.playerId = self.player_id
        
        try:
            # Subscribe to events with a deadline and wait_for_ready
            # wait_for_ready allows the call to wait for the server to become available
            event_stream = self.session_stub.SubscribeEvents(
                player_info,
                timeout=1800,  # 30 minutes timeout
                wait_for_ready=True  # Wait for session server to start
            )
            
            # Wait for START_MATCH_EVENT and then continue processing events
            if not self.wait_for_session_started(event_stream):
                return
            
            # Continue reading events from the same stream
            for event in event_stream:
                print(f"[EVENT] {event}")
                self.process_event(event)
                
                if not self.is_ai:
                    input("Press Enter to continue...")
        
        except grpc.RpcError as e:
            print(f"Event stream closed with error: {e.details()}")
        else:
            print("Event stream finished ok.")
    
    def process_event(self, event: pb2.GameEventMsg) -> None:
        """
        Process incoming game events.
        
        Args:
            event: The game event to process
        """
        if event.eventType == pb2.YOUR_TURN_EVENT:
            self.process_my_turn(event)
    
    def process_my_turn(self, event: pb2.GameEventMsg) -> None:
        """
        Process YOUR_TURN event and make a move.
        
        Args:
            event: The YOUR_TURN event
        """
        print("My turn!")
        
        if not self.is_ai:
            # Human player - parse input
            move = pb2.Move()
            self.parse_input(move, event.yourTurn.playerInfo.playerId)
            self.play_move(move)
            return
        
        # AI player - make random move
        hand = event.yourTurn.hand
        if not hand:
            print("Empty hand!")
            return
        
        random_card = random.choice(hand)
        
        move = pb2.Move()
        move.card.color = random_card.color
        move.card.number = random_card.number
        move.call = pb2.NO_CALL
        
        self.play_move(move)
    
    def parse_input(self, move: pb2.Move, player_id: int) -> None:
        """
        Parse user input for a move.
        
        Args:
            move: Move object to populate
            player_id: Player ID making the move
        """
        while True:
            user_input = input("Enter move (e.g., S1, D10, B7B for Busso): ").strip()
            if self.parse(user_input, move, player_id) == 0:
                break
    
    def parse(self, input_str: str, move: pb2.Move, player_id: int) -> int:
        """
        Parse a move string.
        
        Format: <Color><Number>[Call]
        - Color: S (Spade), D (Denari), B (Bastoni), C (Coppe)
        - Number: 1-10
        - Call (optional): B (Busso), S (Striscio), Q (Con Questa Basta)
        
        Args:
            input_str: Input string to parse
            move: Move object to populate
            player_id: Player ID
            
        Returns:
            0 if successful, negative error code otherwise
        """
        if len(input_str) < 2 or len(input_str) > 4:
            return -1
        
        # Parse color
        color_char = input_str[0].upper()
        color_map = {
            'S': pb2.SPADE,
            'D': pb2.DENARI,
            'B': pb2.BASTONI,
            'C': pb2.COPPE,
        }
        
        if color_char not in color_map:
            return -2
        
        color = color_map[color_char]
        
        # Parse number and optional call
        number = pb2.INVALID_NUMBER
        call = pb2.NO_CALL
        
        i = 1
        while i < len(input_str):
            if input_str[i].isdigit():
                if number == pb2.INVALID_NUMBER:
                    number = 0
                number = number * 10 + int(input_str[i])
                if number > 10:
                    number = number - 3
                i += 1
            else:
                # Parse call
                call_char = input_str[i].upper()
                call_map = {
                    'B': pb2.BUSSO,
                    'S': pb2.STRISCIO,
                    'Q': pb2.CON_QUESTA_BASTA,
                }
                call = call_map.get(call_char, pb2.NO_CALL)
                break
        
        # Set the move
        move.card.color = color
        move.card.number = number
        move.call = call
        
        return 0
    
    def close(self) -> None:
        """Close all channels."""
        if self.session_channel:
            self.session_channel.close()
        if self.lobby_channel:
            self.lobby_channel.close()


def main():
    """Main entry point."""
    if len(sys.argv) != 5:
        print("Usage: python_client.py <name> <gameType> <numPlayers> <human|ai>")
        sys.exit(1)
    
    name = sys.argv[1]
    is_ai = sys.argv[4] == "ai"
    game_type_str = sys.argv[2]
    num_players = int(sys.argv[3])
    
    # Parse game type
    if game_type_str == "briscola":
        game_type = pb2.BRISCOLA
    elif game_type_str == "tressette":
        game_type = pb2.TRESSETTE
    else:
        print(f"Unknown game type: {game_type_str}")
        sys.exit(1)
    
    # Parse single or multi
    if num_players == 2:
        single_or_multi = pb2.SINGLE
    elif num_players == 4:
        single_or_multi = pb2.MULTI
    else:
        print("Unknown number of players")
        sys.exit(1)
    
    # Create game format
    game_format = pb2.GameFormat()
    game_format.gameType = game_type
    game_format.singleOrMulti = single_or_multi
    
    # Create and start client
    client = CardsGameClient("localhost:50051", name, is_ai, game_format)
    
    try:
        client.start_client()
    except KeyboardInterrupt:
        print("\nShutting down...")
    finally:
        client.close()


if __name__ == "__main__":
    main()
