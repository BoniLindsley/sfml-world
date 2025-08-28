#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <iostream>
#include <string>
#include <map>
#include <thread>
#include <mutex>
#include <deque>
#include <sstream>

struct RemotePlayer {
    sf::Vector2f position;
    sf::Color color;
    bool isOnline;
    sf::CircleShape shape;
    sf::Text nameText;
    
    RemotePlayer() : position(400, 300), color(sf::Color::Red), isOnline(false) {
        shape.setRadius(15);
        shape.setOrigin(15, 15);
    }
};

class MMORPGClient {
private:
    sf::RenderWindow window;
    sf::TcpSocket socket;
    sf::Font font;
    
    // Game state
    bool connected;
    bool loggedIn;
    std::string username;
    sf::Vector2f playerPosition;
    sf::Color playerColor;
    sf::CircleShape playerShape;
    
    // Remote players
    std::map<std::string, RemotePlayer> remotePlayers;
    std::mutex remotePlayersMutex;
    
    // Chat system
    std::deque<std::string> chatMessages;
    std::string currentChatInput;
    bool chatInputActive;
    sf::Text chatText;
    sf::RectangleShape chatInputBox;
    
    // UI elements
    sf::Text statusText;
    std::string statusMessage;
    
    // Avatar creation
    bool showingAvatarCreation;
    sf::Uint8 avatarR, avatarG, avatarB;
    
    enum MessageType {
        LOGIN = 1,
        LOGOUT = 2,
        CHAT = 3,
        PLAYER_MOVE = 4,
        PLAYER_UPDATE = 5,
        CREATE_AVATAR = 6,
        LOGIN_SUCCESS = 7,
        LOGIN_FAILED = 8
    };

public:
    MMORPGClient() : window(sf::VideoMode(1024, 768), "MMORPG Client"),
                     connected(false), loggedIn(false),
                     playerPosition(400, 300), playerColor(sf::Color::Blue),
                     chatInputActive(false), showingAvatarCreation(false),
                     avatarR(0), avatarG(0), avatarB(255) {
        
        // Load font (you'll need to have a font file)
        if (!font.loadFromFile("arial.ttf")) {
            // If arial.ttf doesn't exist, try to use the default font
            std::cout << "Warning: Could not load arial.ttf font" << std::endl;
        }
        
        setupUI();
        setupPlayer();
    }
    
    bool connectToServer(const std::string& serverIP, unsigned short port) {
        if (socket.connect(serverIP, port) != sf::Socket::Done) {
            statusMessage = "Failed to connect to server";
            return false;
        }
        
        connected = true;
        statusMessage = "Connected to server. Type username and press Enter to login.";
        
        // Start network thread
        std::thread networkThread(&MMORPGClient::handleNetwork, this);
        networkThread.detach();
        
        return true;
    }
    
    void run() {
        sf::Clock clock;
        
        while (window.isOpen()) {
            handleEvents();
            update(clock.restart());
            render();
        }
    }

private:
    void setupUI() {
        statusText.setFont(font);
        statusText.setCharacterSize(16);
        statusText.setFillColor(sf::Color::White);
        statusText.setPosition(10, 10);
        
        chatText.setFont(font);
        chatText.setCharacterSize(14);
        chatText.setFillColor(sf::Color::White);
        chatText.setPosition(10, window.getSize().y - 150);
        
        chatInputBox.setSize(sf::Vector2f(400, 25));
        chatInputBox.setPosition(10, window.getSize().y - 30);
        chatInputBox.setFillColor(sf::Color(50, 50, 50));
        chatInputBox.setOutlineColor(sf::Color::White);
        chatInputBox.setOutlineThickness(1);
        
        statusMessage = "Press C to connect to server";
    }
    
    void setupPlayer() {
        playerShape.setRadius(15);
        playerShape.setOrigin(15, 15);
        playerShape.setFillColor(playerColor);
        playerShape.setPosition(playerPosition);
    }
    
    void handleEvents() {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                if (loggedIn) {
                    logout();
                }
                window.close();
            }
            
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::C && !connected) {
                    connectToServer("127.0.0.1", 53000);
                }
                else if (event.key.code == sf::Keyboard::Enter) {
                    if (chatInputActive) {
                        if (!currentChatInput.empty()) {
                            if (loggedIn) {
                                sendChatMessage(currentChatInput);
                            }
                            currentChatInput.clear();
                        }
                        chatInputActive = false;
                    }
                    else if (connected && !loggedIn && !currentChatInput.empty()) {
                        login(currentChatInput);
                        currentChatInput.clear();
                    }
                }
                else if (event.key.code == sf::Keyboard::T && loggedIn) {
                    chatInputActive = true;
                }
                else if (event.key.code == sf::Keyboard::A && loggedIn) {
                    showingAvatarCreation = !showingAvatarCreation;
                }
                else if (event.key.code == sf::Keyboard::Escape) {
                    chatInputActive = false;
                    showingAvatarCreation = false;
                    currentChatInput.clear();
                }
                else if (showingAvatarCreation) {
                    handleAvatarInput(event.key.code);
                }
            }
            
            if (event.type == sf::Event::TextEntered && (chatInputActive || (!loggedIn && connected))) {
                if (event.text.unicode >= 32 && event.text.unicode < 127) { // Printable ASCII
                    currentChatInput += static_cast<char>(event.text.unicode);
                }
                else if (event.text.unicode == 8 && !currentChatInput.empty()) { // Backspace
                    currentChatInput.pop_back();
                }
            }
        }
    }
    
    void handleAvatarInput(sf::Keyboard::Key key) {
        bool changed = false;
        
        switch (key) {
            case sf::Keyboard::R:
                avatarR = (avatarR + 50) % 256;
                changed = true;
                break;
            case sf::Keyboard::G:
                avatarG = (avatarG + 50) % 256;
                changed = true;
                break;
            case sf::Keyboard::B:
                avatarB = (avatarB + 50) % 256;
                changed = true;
                break;
            case sf::Keyboard::Space:
                if (changed || playerColor != sf::Color(avatarR, avatarG, avatarB)) {
                    createAvatar();
                }
                showingAvatarCreation = false;
                break;
        }
        
        if (changed) {
            playerColor = sf::Color(avatarR, avatarG, avatarB);
            playerShape.setFillColor(playerColor);
        }
    }
    
    void update(sf::Time deltaTime) {
        if (loggedIn) {
            handleMovement(deltaTime);
        }
        
        updateUI();
    }
    
    void handleMovement(sf::Time deltaTime) {
        sf::Vector2f movement(0, 0);
        float speed = 200.0f; // pixels per second
        
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) || sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
            movement.x -= speed * deltaTime.asSeconds();
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) || sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
            movement.x += speed * deltaTime.asSeconds();
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up) || sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
            movement.y -= speed * deltaTime.asSeconds();
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down) || sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
            movement.y += speed * deltaTime.asSeconds();
        }
        
        if (movement.x != 0 || movement.y != 0) {
            playerPosition += movement;
            
            // Keep player in bounds
            if (playerPosition.x < 15) playerPosition.x = 15;
            if (playerPosition.x > window.getSize().x - 15) playerPosition.x = window.getSize().x - 15;
            if (playerPosition.y < 15) playerPosition.y = 15;
            if (playerPosition.y > window.getSize().y - 15) playerPosition.y = window.getSize().y - 15;
            
            playerShape.setPosition(playerPosition);
            
            // Send movement to server
            sendMovement();
        }
    }
    
    void updateUI() {
        // Update status text
        std::string displayStatus = statusMessage;
        if (!loggedIn && connected) {
            displayStatus += "\nUsername: " + currentChatInput;
        }
        else if (chatInputActive) {
            displayStatus += "\nChat: " + currentChatInput;
        }
        else if (showingAvatarCreation) {
            std::stringstream ss;
            ss << "\nAvatar Creation (RGB: " << (int)avatarR << ", " << (int)avatarG << ", " << (int)avatarB << ")";
            ss << "\nPress R/G/B to change colors, Space to confirm, Esc to cancel";
            displayStatus += ss.str();
        }
        else if (loggedIn) {
            displayStatus += "\nControls: WASD/Arrows=Move, T=Chat, A=Avatar, Esc=Cancel";
        }
        
        statusText.setString(displayStatus);
        
        // Update chat display
        std::string chatDisplay;
        int linesToShow = std::min(8, (int)chatMessages.size());
        for (int i = chatMessages.size() - linesToShow; i < chatMessages.size(); i++) {
            chatDisplay += chatMessages[i] + "\n";
        }
        chatText.setString(chatDisplay);
    }
    
    void render() {
        window.clear(sf::Color::Black);
        
        if (loggedIn) {
            // Draw remote players
            std::lock_guard<std::mutex> lock(remotePlayersMutex);
            for (const auto& pair : remotePlayers) {
                if (pair.second.isOnline) {
                    window.draw(pair.second.shape);
                    window.draw(pair.second.nameText);
                }
            }
            
            // Draw local player
            window.draw(playerShape);
        }
        
        // Draw UI
        window.draw(statusText);
        window.draw(chatText);
        
        if (chatInputActive || (!loggedIn && connected)) {
            window.draw(chatInputBox);
            
            sf::Text inputText;
            inputText.setFont(font);
            inputText.setCharacterSize(14);
            inputText.setFillColor(sf::Color::White);
            inputText.setPosition(15, window.getSize().y - 27);
            inputText.setString(currentChatInput + "_");
            window.draw(inputText);
        }
        
        window.display();
    }
    
    void handleNetwork() {
        sf::Packet packet;
        while (connected) {
            if (socket.receive(packet) == sf::Socket::Done) {
                int messageType;
                packet >> messageType;
                
                switch (messageType) {
                    case LOGIN_SUCCESS:
                        handleLoginSuccess(packet);
                        break;
                    case LOGIN_FAILED:
                        handleLoginFailed(packet);
                        break;
                    case CHAT:
                        handleChatMessage(packet);
                        break;
                    case PLAYER_MOVE:
                        handlePlayerMove(packet);
                        break;
                    case PLAYER_UPDATE:
                        handlePlayerUpdate(packet);
                        break;
                }
            }
        }
    }
    
    void login(const std::string& user) {
        username = user;
        sf::Packet packet;
        packet << LOGIN << username;
        socket.send(packet);
    }
    
    void logout() {
        if (loggedIn && connected) {
            sf::Packet packet;
            packet << LOGOUT;
            socket.send(packet);
            loggedIn = false;
        }
    }
    
    void sendChatMessage(const std::string& message) {
        sf::Packet packet;
        packet << CHAT << message;
        socket.send(packet);
        
        // Add to local chat
        chatMessages.push_back("[" + username + "]: " + message);
        if (chatMessages.size() > 20) {
            chatMessages.pop_front();
        }
    }
    
    void sendMovement() {
        sf::Packet packet;
        packet << PLAYER_MOVE << playerPosition.x << playerPosition.y;
        socket.send(packet);
    }
    
    void createAvatar() {
        sf::Packet packet;
        packet << CREATE_AVATAR << avatarR << avatarG << avatarB;
        socket.send(packet);
    }
    
    void handleLoginSuccess(sf::Packet& packet) {
        std::string user;
        packet >> user;
        
        loggedIn = true;
        statusMessage = "Logged in as: " + user;
        
        // Set avatar colors to current player color
        avatarR = playerColor.r;
        avatarG = playerColor.g;
        avatarB = playerColor.b;
    }
    
    void handleLoginFailed(sf::Packet& packet) {
        std::string reason;
        packet >> reason;
        statusMessage = "Login failed: " + reason;
    }
    
    void handleChatMessage(sf::Packet& packet) {
        std::string sender, message;
        packet >> sender >> message;
        
        chatMessages.push_back("[" + sender + "]: " + message);
        if (chatMessages.size() > 20) {
            chatMessages.pop_front();
        }
    }
    
    void handlePlayerMove(sf::Packet& packet) {
        std::string playerName;
        float x, y;
        packet >> playerName >> x >> y;
        
        std::lock_guard<std::mutex> lock(remotePlayersMutex);
        if (remotePlayers.find(playerName) != remotePlayers.end()) {
            remotePlayers[playerName].position = sf::Vector2f(x, y);
            remotePlayers[playerName].shape.setPosition(x, y);
        }
    }
    
    void handlePlayerUpdate(sf::Packet& packet) {
        std::string playerName;
        float x, y;
        sf::Uint8 r, g, b;
        bool isOnline;
        packet >> playerName >> x >> y >> r >> g >> b >> isOnline;
        
        std::lock_guard<std::mutex> lock(remotePlayersMutex);
        RemotePlayer& player = remotePlayers[playerName];
        player.position = sf::Vector2f(x, y);
        player.color = sf::Color(r, g, b);
        player.isOnline = isOnline;
        
        player.shape.setPosition(x, y);
        player.shape.setFillColor(player.color);
        
        player.nameText.setFont(font);
        player.nameText.setString(playerName);
        player.nameText.setCharacterSize(12);
        player.nameText.setFillColor(sf::Color::White);
        player.nameText.setPosition(x - 20, y - 30);
        
        if (!isOnline) {
            // Player logged out, but keep in map for potential reconnection
        }
    }
};

int main() {
    MMORPGClient client;
    client.run();
    return 0;
}