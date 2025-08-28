#include <SFML/Network.hpp>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <thread>
#include <mutex>
#include <sstream>

struct Player {
    std::string username;
    sf::Vector2f position;
    sf::Color color;
    sf::TcpSocket* socket;
    bool isLoggedIn;
    
    Player() : position(400, 300), color(sf::Color::Blue), socket(nullptr), isLoggedIn(false) {}
};

class MMORPGServer {
private:
    sf::TcpListener listener;
    std::vector<std::unique_ptr<sf::TcpSocket>> clients;
    std::map<std::string, Player> players;
    std::mutex playersMutex;
    bool running;
    
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
    MMORPGServer() : running(false) {}
    
    bool start(unsigned short port) {
        if (listener.listen(port) != sf::Socket::Done) {
            std::cout << "Error: Could not listen on port " << port << std::endl;
            return false;
        }
        
        running = true;
        std::cout << "Server started on port " << port << std::endl;
        return true;
    }
    
    void run() {
        while (running) {
            auto client = std::make_unique<sf::TcpSocket>();
            
            if (listener.accept(*client) == sf::Socket::Done) {
                std::cout << "New client connected: " << client->getRemoteAddress() << std::endl;
                
                clients.push_back(std::move(client));
                
                // Start thread to handle this client
                std::thread clientThread(&MMORPGServer::handleClient, this, clients.back().get());
                clientThread.detach();
            }
        }
    }
    
    void stop() {
        running = false;
        listener.close();
    }

private:
    void handleClient(sf::TcpSocket* client) {
        sf::Packet packet;
        std::string clientKey = getClientKey(client);
        
        while (client->receive(packet) == sf::Socket::Done) {
            int messageType;
            packet >> messageType;
            
            switch (messageType) {
                case LOGIN:
                    handleLogin(client, packet);
                    break;
                case LOGOUT:
                    handleLogout(client);
                    break;
                case CHAT:
                    handleChat(client, packet);
                    break;
                case PLAYER_MOVE:
                    handlePlayerMove(client, packet);
                    break;
                case CREATE_AVATAR:
                    handleCreateAvatar(client, packet);
                    break;
            }
        }
        
        // Client disconnected
        handleDisconnect(client);
    }
    
    void handleLogin(sf::TcpSocket* client, sf::Packet& packet) {
        std::string username;
        packet >> username;
        
        std::lock_guard<std::mutex> lock(playersMutex);
        
        // Check if username already exists and is logged in
        auto it = players.find(username);
        if (it != players.end() && it->second.isLoggedIn) {
            // Username already logged in
            sf::Packet response;
            response << LOGIN_FAILED << std::string("Username already logged in");
            client->send(response);
            return;
        }
        
        // Create or update player
        Player& player = players[username];
        player.username = username;
        player.socket = client;
        player.isLoggedIn = true;
        
        // Send success response
        sf::Packet response;
        response << LOGIN_SUCCESS << username;
        client->send(response);
        
        std::cout << "Player logged in: " << username << std::endl;
        
        // Send current player list to new player
        sendPlayerList(client);
        
        // Notify other players about new player
        broadcastPlayerUpdate(username);
    }
    
    void handleLogout(sf::TcpSocket* client) {
        std::lock_guard<std::mutex> lock(playersMutex);
        
        // Find and logout player
        for (auto& pair : players) {
            if (pair.second.socket == client && pair.second.isLoggedIn) {
                pair.second.isLoggedIn = false;
                pair.second.socket = nullptr;
                
                std::cout << "Player logged out: " << pair.first << std::endl;
                
                // Notify other players
                broadcastPlayerUpdate(pair.first);
                break;
            }
        }
    }
    
    void handleChat(sf::TcpSocket* client, sf::Packet& packet) {
        std::string message;
        packet >> message;
        
        std::lock_guard<std::mutex> lock(playersMutex);
        
        // Find sender
        std::string sender = "Unknown";
        for (const auto& pair : players) {
            if (pair.second.socket == client && pair.second.isLoggedIn) {
                sender = pair.first;
                break;
            }
        }
        
        std::cout << "Chat [" << sender << "]: " << message << std::endl;
        
        // Broadcast to all logged in players
        sf::Packet broadcast;
        broadcast << CHAT << sender << message;
        
        for (const auto& pair : players) {
            if (pair.second.isLoggedIn && pair.second.socket != client) {
                pair.second.socket->send(broadcast);
            }
        }
    }
    
    void handlePlayerMove(sf::TcpSocket* client, sf::Packet& packet) {
        float x, y;
        packet >> x >> y;
        
        std::lock_guard<std::mutex> lock(playersMutex);
        
        // Update player position
        for (auto& pair : players) {
            if (pair.second.socket == client && pair.second.isLoggedIn) {
                pair.second.position.x = x;
                pair.second.position.y = y;
                
                // Broadcast position update
                sf::Packet broadcast;
                broadcast << PLAYER_MOVE << pair.first << x << y;
                
                for (const auto& otherPair : players) {
                    if (otherPair.second.isLoggedIn && otherPair.second.socket != client) {
                        otherPair.second.socket->send(broadcast);
                    }
                }
                break;
            }
        }
    }
    
    void handleCreateAvatar(sf::TcpSocket* client, sf::Packet& packet) {
        sf::Uint8 r, g, b;
        packet >> r >> g >> b;
        
        std::lock_guard<std::mutex> lock(playersMutex);
        
        // Update player color
        for (auto& pair : players) {
            if (pair.second.socket == client && pair.second.isLoggedIn) {
                pair.second.color = sf::Color(r, g, b);
                
                std::cout << "Player " << pair.first << " updated avatar color" << std::endl;
                
                // Broadcast avatar update
                broadcastPlayerUpdate(pair.first);
                break;
            }
        }
    }
    
    void handleDisconnect(sf::TcpSocket* client) {
        std::lock_guard<std::mutex> lock(playersMutex);
        
        // Find and logout player
        for (auto& pair : players) {
            if (pair.second.socket == client) {
                if (pair.second.isLoggedIn) {
                    pair.second.isLoggedIn = false;
                    pair.second.socket = nullptr;
                    
                    std::cout << "Player disconnected: " << pair.first << std::endl;
                    broadcastPlayerUpdate(pair.first);
                }
                break;
            }
        }
        
        // Remove client from vector
        clients.erase(
            std::remove_if(clients.begin(), clients.end(),
                [client](const std::unique_ptr<sf::TcpSocket>& ptr) {
                    return ptr.get() == client;
                }),
            clients.end()
        );
    }
    
    void sendPlayerList(sf::TcpSocket* client) {
        for (const auto& pair : players) {
            if (pair.second.isLoggedIn) {
                sf::Packet packet;
                packet << PLAYER_UPDATE << pair.first 
                       << pair.second.position.x << pair.second.position.y
                       << pair.second.color.r << pair.second.color.g << pair.second.color.b
                       << true; // isLoggedIn
                client->send(packet);
            }
        }
    }
    
    void broadcastPlayerUpdate(const std::string& username) {
        const Player& player = players[username];
        
        sf::Packet broadcast;
        broadcast << PLAYER_UPDATE << username 
                 << player.position.x << player.position.y
                 << player.color.r << player.color.g << player.color.b
                 << player.isLoggedIn;
        
        for (const auto& pair : players) {
            if (pair.second.isLoggedIn && pair.first != username) {
                pair.second.socket->send(broadcast);
            }
        }
    }
    
    std::string getClientKey(sf::TcpSocket* client) {
        return client->getRemoteAddress().toString() + ":" + std::to_string(client->getRemotePort());
    }
};

int main() {
    MMORPGServer server;
    
    if (!server.start(53000)) {
        return -1;
    }
    
    std::cout << "Press Enter to stop the server..." << std::endl;
    
    // Start server in separate thread
    std::thread serverThread(&MMORPGServer::run, &server);
    
    // Wait for user input to stop
    std::cin.get();
    
    server.stop();
    
    if (serverThread.joinable()) {
        serverThread.join();
    }
    
    return 0;
}