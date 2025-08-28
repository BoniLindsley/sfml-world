# Barebone MMORPG Template with SFML

A simple client-server MMORPG template built with C++ and SFML that includes login/logout, chat system, avatar creation, and movement in a shared game world.

## Features

### ✅ Implemented Features
1. **Login/Logout System** - Players can connect with unique usernames
2. **Real-time Chat** - Global chat system for all connected players
3. **Avatar Creation** - Customizable player colors (RGB values)
4. **Movement System** - Real-time movement with WASD/Arrow keys
5. **Multi-player Support** - See other players moving around in real-time

### 🏗️ Server Features
- Handles multiple concurrent clients
- Player authentication and session management
- Real-time position synchronization
- Chat message broadcasting
- Avatar customization system

### 🎮 Client Features
- Graphical interface with SFML
- Real-time movement with smooth controls
- Interactive chat system
- Avatar customization interface
- Visual representation of other players
- Connection status and user feedback

## Prerequisites

### System Requirements
- C++ compiler with C++17 support (GCC 7+ or Clang 5+)
- SFML 2.5+ library
- pthread support (usually built-in on Linux/macOS)

### Installing SFML

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install libsfml-dev
```

**macOS (with Homebrew):**
```bash
brew install sfml
```

**Windows:**
- Download SFML from https://www.sfml-dev.org/download.php
- Extract and configure your IDE/compiler paths

## Building the Project

### Using the Makefile

1. **Build everything:**
```bash
make all
```

2. **Build only server:**
```bash
make server
```

3. **Build only client:**
```bash
make client
```

### Manual Compilation

**Server:**
```bash
g++ -std=c++17 -Wall -Wextra -O2 -o mmorpg_server server.cpp -lsfml-network -lpthread
```

**Client:**
```bash
g++ -std=c++17 -Wall -Wextra -O2 -o mmorpg_client client.cpp -lsfml-graphics -lsfml-window -lsfml-system -lsfml-network -lpthread
```

## Running the Game

### 1. Start the Server
```bash
./mmorpg_server
```
- Server listens on port 53000
- Press Enter to stop the server
- Server logs all player activities

### 2. Start Client(s)
```bash
./mmorpg_client
```

**Note:** You need a font file named `arial.ttf` in the same directory as the client executable. If you don't have it, the client will still work but text may not display properly.

### 3. Download a Font (Optional but Recommended)
```bash
# On Ubuntu/Debian
sudo apt-get install fonts-liberation
cp /usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf arial.ttf

# Or download any TTF font and rename it to arial.ttf
```

## How to Play

### Initial Connection
1. Launch the client
2. Press **C** to connect to the server (connects to localhost:53000)
3. Type your username and press **Enter** to login

### Controls
- **WASD** or **Arrow Keys** - Move your character
- **T** - Open chat input
- **A** - Open avatar customization
- **Enter** - Send chat message or confirm input
- **Escape** - Cancel current input/close menus

### Avatar Customization
1. Press **A** to open avatar editor
2. Press **R**, **G**, **B** to cycle through color values
3. Press **Space** to confirm changes
4. Press **Escape** to cancel

### Chat System
1. Press **T** to activate chat input
2. Type your message
3. Press **Enter** to send
4. Chat history shows last 8 messages

## Network Protocol

The client-server communication uses the following message types:

```cpp
enum MessageType {
    LOGIN = 1,          // Client -> Server: Login request
    LOGOUT = 2,         // Client -> Server: Logout request  
    CHAT = 3,           // Bidirectional: Chat messages
    PLAYER_MOVE = 4,    // Bidirectional: Position updates
    PLAYER_UPDATE = 5,  // Server -> Client: Player state
    CREATE_AVATAR = 6,  // Client -> Server: Avatar changes
    LOGIN_SUCCESS = 7,  // Server -> Client: Login confirmed
    LOGIN_FAILED = 8    // Server -> Client: Login rejected
};
```

## Extending the Template

### Adding New Features
1. **Game World Objects** - Add static/dynamic objects to the world
2. **Player Stats** - Health, level, experience system
3. **Inventory System** - Items, equipment, trading
4. **Combat System** - Player vs Player or vs Environment
5. **Database Integration** - Persistent player data
6. **Map System** - Multiple areas, teleportation
7. **Guilds/Groups** - Team formation and management

### Code Structure
```
server.cpp          # Server-side logic and networking
├── Player struct   # Player data management
├── MMORPGServer   # Main server class
└── Message handlers # Login, chat, movement, etc.

client.cpp          # Client-side logic and rendering
├── RemotePlayer   # Other players representation
├── MMORPGClient   # Main client class
└── Event handlers  # Input, network, UI updates
```

## Troubleshooting

### Common Issues

**"Could not connect to server"**
- Ensure server is running first
- Check if port 53000 is available
- Try connecting to "127.0.0.1" explicitly

**"Could not load arial.ttf font"**
- Download any TTF font file and rename it to `arial.ttf`
- Place it in the same directory as the client executable

**Compilation errors**
- Ensure SFML is properly installed
- Check C++17 compiler support
- Verify all library paths are correct

**Players not visible**
- Check network connectivity
- Ensure both client and server are using the same message protocol
- Look at server console for connection logs

### Performance Tips
- The server can handle multiple clients, but performance depends on hardware
- Client rendering is capped by SFML's default framerate
- Network updates are sent for every movement - consider adding position delta thresholds for optimization

## License

This is a template/educational project. Feel free to use and modify as needed for your own projects.

## Next Steps

This barebone template provides the foundation for a more complex MMORPG. Consider implementing:

1. **Database persistence** (SQLite, PostgreSQL)
2. **Advanced networking** (UDP for movement, TCP for important data)
3. **Game world design** (maps, zones, NPCs)
4. **Security measures** (input validation, anti-cheat)
5. **Scalability improvements** (multiple server instances, load balancing)