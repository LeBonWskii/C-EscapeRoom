# C-EscapeRoom
## Description
A TCP-based client-server application implementing reliable communication, centralized game management, and a collaborative mini-game.

## Features
- **Reliable Communication**: Uses TCP sockets for string exchanges.
- **I/O Multiplexing**: Efficient handling of input/output for server and clients.
- **Data Persistence**: User login data stored in text files for durability.
- **Special Functionality**: A collaborative mini-game between player and helper clients.

## Project Structure
- **Server**: Manages sessions, handshake, and communication with clients.
- **Player Client** (`client.c`): Sends game commands to the server.
- **Helper Client** (`other.c`): Assists in the mini-game.

## Additional Features
### "Guess" Mini-Game
A cooperative game where the player and helper write words aiming to synchronize. If successful, the player gains extra time.

## Requirements
- C Compiler (e.g., GCC)
- TCP-compatible OS
- Standard C Libraries

## Instructions
1. **Clone the repository**:
    ```bash
    git clone https://github.com/your-username/network-systems-project.git
    ```
2. **Compile the project**:
    ```bash
    gcc -o server server.c
    gcc -o client client.c
    gcc -o other other.c
    ```
3. **Run the server**:
    ```bash
    ./server
    ```
4. **Run the clients**:
    ```bash
    ./client
    ./other
    ```
5. Follow terminal instructions to start a game session.

## Implementation Details
- **TCP Sockets**: Ensures reliable communication.
- **Persistent Data**: User data stored in text files for fault tolerance.
- **Dynamic Structures**: Supports future enhancements for multiple users and games.

## Author
Tommaso Falaschi [MAT.616097]

