# Makefile for MMORPG Server and Client

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
LIBS = -lsfml-graphics -lsfml-window -lsfml-system -lsfml-network -lpthread

# Source files
SERVER_SRC = server.cpp
CLIENT_SRC = client.cpp

# Output executables
SERVER_OUT = mmorpg_server
CLIENT_OUT = mmorpg_client

# Default target
all: server client

# Build server
server: $(SERVER_OUT)

$(SERVER_OUT): $(SERVER_SRC)
	$(CXX) $(CXXFLAGS) -o $(SERVER_OUT) $(SERVER_SRC) $(LIBS)

# Build client
client: $(CLIENT_OUT)

$(CLIENT_OUT): $(CLIENT_SRC)
	$(CXX) $(CXXFLAGS) -o $(CLIENT_OUT) $(CLIENT_SRC) $(LIBS)

# Clean build files
clean:
	rm -f $(SERVER_OUT) $(CLIENT_OUT)

# Install SFML (Ubuntu/Debian)
install-deps:
	sudo apt-get update
	sudo apt-get install libsfml-dev

# Install SFML (macOS with Homebrew)
install-deps-mac:
	brew install sfml

# Run server
run-server: server
	./$(SERVER_OUT)

# Run client
run-client: client
	./$(CLIENT_OUT)

.PHONY: all server client clean install-deps install-deps-mac run-server run-client
