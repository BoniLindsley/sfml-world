#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <deque>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

struct RemotePlayer {
  sf::Vector2f position;
  sf::Color color;
  bool isOnline;
  sf::CircleShape shape;
  sf::Text nameText;

  RemotePlayer(const sf::Font& font)
      : position(400, 300), color(sf::Color::Red), isOnline(false),
        nameText(font) {
    shape.setRadius(15);
    shape.setOrigin({15, 15});
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
  sf::Text chatText{font};
  sf::RectangleShape chatInputBox;

  // UI elements
  sf::Text statusText{font};
  std::string statusMessage;

  // Avatar creation
  bool showingAvatarCreation;
  sf::Color avatarColor{};

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
  MMORPGClient()
      : window(sf::VideoMode({1024, 768}), "MMORPG Client"),
        connected(false), loggedIn(false), playerPosition(400, 300),
        playerColor(sf::Color::Blue), chatInputActive(false),
        showingAvatarCreation(false) {

    // Load font (you'll need to have a font file)
    if (!font.openFromFile("arial.ttf")) {
      // If arial.ttf doesn't exist, try to use the default font
      std::cout << "Warning: Could not load arial.ttf font" << std::endl;
    }

    setupUI();
    setupPlayer();
  }

  bool
  connectToServer(const std::string& serverIP, unsigned short port) {
    auto resolvedIP = sf::IpAddress::resolve(serverIP);
    if (not resolvedIP or
        socket.connect(*resolvedIP, port) != sf::Socket::Status::Done) {
      statusMessage = "Failed to connect to server";
      return false;
    }

    connected = true;
    statusMessage =
        "Connected to server. Type username and press Enter to login.";

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
    statusText.setPosition({10, 10});

    chatText.setFont(font);
    chatText.setCharacterSize(14);
    chatText.setFillColor(sf::Color::White);
    chatText.setPosition({10, window.getSize().y - 150});

    chatInputBox.setSize(sf::Vector2f(400, 25));
    chatInputBox.setPosition({10, window.getSize().y - 30});
    chatInputBox.setFillColor(sf::Color(50, 50, 50));
    chatInputBox.setOutlineColor(sf::Color::White);
    chatInputBox.setOutlineThickness(1);

    statusMessage = "Press C to connect to server";
  }

  void setupPlayer() {
    playerShape.setRadius(15);
    playerShape.setOrigin({15, 15});
    playerShape.setFillColor(playerColor);
    playerShape.setPosition(playerPosition);
  }

  void handleEvents() {
    while (const auto event = window.pollEvent()) {
      if (event->is<sf::Event::Closed>()) {
        if (loggedIn) {
          logout();
        }
        window.close();
      }

      if (event->is<sf::Event::KeyPressed>()) {
        auto code = event->getIf<sf::Event::KeyPressed>()->code;
        if (code == sf::Keyboard::Key::C && !connected) {
          connectToServer("127.0.0.1", 53000);
        } else if (code == sf::Keyboard::Key::Enter) {
          if (chatInputActive) {
            if (!currentChatInput.empty()) {
              if (loggedIn) {
                sendChatMessage(currentChatInput);
              }
              currentChatInput.clear();
            }
            chatInputActive = false;
          } else if (
              connected && !loggedIn && !currentChatInput.empty()) {
            login(currentChatInput);
            currentChatInput.clear();
          }
        } else if (code == sf::Keyboard::Key::T && loggedIn) {
          chatInputActive = true;
        } else if (code == sf::Keyboard::Key::A && loggedIn) {
          showingAvatarCreation = !showingAvatarCreation;
        } else if (code == sf::Keyboard::Key::Escape) {
          chatInputActive = false;
          showingAvatarCreation = false;
          currentChatInput.clear();
        } else if (showingAvatarCreation) {
          handleAvatarInput(code);
        }
      }

      if (event->is<sf::Event::TextEntered>() &&
          (chatInputActive || (!loggedIn && connected))) {
        auto unicode = event->getIf<sf::Event::TextEntered>()->unicode;
        if (unicode >= 32 && unicode < 127) { // Printable ASCII
          currentChatInput += static_cast<char>(unicode);
        } else if (
            unicode == 8 && !currentChatInput.empty()) { // Backspace
          currentChatInput.pop_back();
        }
      }
    }
  }

  void handleAvatarInput(sf::Keyboard::Key key) {
    bool changed = false;

    switch (key) {
      case sf::Keyboard::Key::R:
      avatarColor.r += 50;
      changed = true;
      break;
      case sf::Keyboard::Key::G:
      avatarColor.g += 50;
      changed = true;
      break;
    case sf::Keyboard::Key::B:
      avatarColor.b += 50;
      changed = true;
      break;
    case sf::Keyboard::Key::Space:
      if (changed || playerColor != avatarColor) {
        createAvatar();
      }
      showingAvatarCreation = false;
      break;
    }

    if (changed) {
      playerColor = avatarColor;
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

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
      movement.x -= speed * deltaTime.asSeconds();
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
      movement.x += speed * deltaTime.asSeconds();
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) {
      movement.y -= speed * deltaTime.asSeconds();
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) {
      movement.y += speed * deltaTime.asSeconds();
    }

    if (movement.x != 0 || movement.y != 0) {
      playerPosition += movement;

      // Keep player in bounds
      if (playerPosition.x < 15)
        playerPosition.x = 15;
      if (playerPosition.x > window.getSize().x - 15)
        playerPosition.x = window.getSize().x - 15;
      if (playerPosition.y < 15)
        playerPosition.y = 15;
      if (playerPosition.y > window.getSize().y - 15)
        playerPosition.y = window.getSize().y - 15;

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
    } else if (chatInputActive) {
      displayStatus += "\nChat: " + currentChatInput;
    } else if (showingAvatarCreation) {
      std::stringstream ss;
      ss << "\nAvatar Creation (RGB: " << static_cast<int>(avatarColor.r)
         << "," << static_cast<int>(avatarColor.g) << ","
         << static_cast<int>(avatarColor.b) << ")";
      displayStatus += ss.str();
    } else if (loggedIn) {
      displayStatus +=
          "\nControls: WASD/Arrows=Move, T=Chat, A=Avatar, Esc=Cancel";
    }

    statusText.setString(displayStatus);

    // Update chat display
    std::string chatDisplay;
    int linesToShow = std::min(8, (int)chatMessages.size());
    for (int i = chatMessages.size() - linesToShow;
         i < chatMessages.size(); i++) {
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

      sf::Text inputText{font};
      inputText.setCharacterSize(14);
      inputText.setFillColor(sf::Color::White);
      inputText.setPosition({15, window.getSize().y - 27});
      inputText.setString(currentChatInput + "_");
      window.draw(inputText);
    }

    window.display();
  }

  void handleNetwork() {
    sf::Packet packet;
    while (connected) {
      if (socket.receive(packet) == sf::Socket::Status::Done) {
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
    packet << CREATE_AVATAR << avatarColor.r << avatarColor.g
           << avatarColor.b;
    socket.send(packet);
  }

  void handleLoginSuccess(sf::Packet& packet) {
    std::string user;
    packet >> user;

    loggedIn = true;
    statusMessage = "Logged in as: " + user;

    // Set avatar colors to current player color
    avatarColor = playerColor;
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
    if (auto player = remotePlayers.find(playerName); player != remotePlayers.end()) {
      player->second.position = {x, y};
      player->second.shape.setPosition({x, y});
    }
  }

  void handlePlayerUpdate(sf::Packet& packet) {
    std::string playerName;
    float x, y;
    auto receivedColor = sf::Color{};
    bool isOnline;
    packet >> playerName >> x >> y >> receivedColor.r >> receivedColor.g >> receivedColor.b >> isOnline;

    std::lock_guard<std::mutex> lock(remotePlayersMutex);
    auto playerIter = remotePlayers.try_emplace(playerName, font);
    RemotePlayer& player = playerIter.first->second;
    player.position = sf::Vector2f(x, y);
    player.color = receivedColor;
    player.isOnline = isOnline;

    player.shape.setPosition({x, y});
    player.shape.setFillColor(player.color);

    player.nameText.setFont(font);
    player.nameText.setString(playerName);
    player.nameText.setCharacterSize(12);
    player.nameText.setFillColor(sf::Color::White);
    player.nameText.setPosition({x - 20, y - 30});

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
