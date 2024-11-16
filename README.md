# Escape Room Network Game 🎲

## Description 🌐
A TCP-based client-server escape room game where players solve puzzles to progress and collaborate with a helper client.

## Features 🛠
- **Reliable Communication**: TCP sockets ensure robust data transmission.
- **I/O Multiplexing**: Efficient handling of multiple streams for server and clients.
- **Collaborative Gameplay**: Players can interact with a helper client for solving puzzles.
- **Interactive Commands**: A variety of commands for managing gameplay and interacting with the environment.
## Setup ⚙
This game has been tested on Debian 8
### Clone the repository
``` git clone https://github.com/your-username/escape-room-game.git```
### Run the Script
```bash
./exec2024.sh
```
## GamePlay Overview 🎮
1. Register and Login:
   - Use the `register` command to create an account.
   - Use the `login` command to log into the game.
   
2. Start a Game:
   - Use the `start` command to begin an escape room session.
   
3. Explore and Solve:
   - Use commands like `look`, `take`, and `use` to interact with the environment and solve puzzles.

4. Special Functionality:
   - Activate the `guess` command to collaborate with the helper client.

5. End the Game:
   - Use the `end` command to terminate the session.

## Commands 🕹
### Server Commands
- `start`: Starts the game server.
- `stop`: Stops the game server.
### Client Commands
- `register <username> <password>`: Register a new user.
- `login <username> <password>`: Log in to the game.
- `start <room_name>`: Start the escape room with the specified room name.
- `look <room | object>`: Display the description of a room or an object.
- `take <object>`: Pick up an object or initiate a puzzle to unlock it.
- `use <object1> [object2]`: Use an object, optionally with another object.
- `objs`: View a list of collected objects.
- `end`: End the current game session.
- `guess`: Activate the special collaborative mini-game with the helper client. 
       In this feature, the player collaborates with the helper client to guess the same word 
       by exchanging words in turns without seeing each other's input. Success earns bonus time.


