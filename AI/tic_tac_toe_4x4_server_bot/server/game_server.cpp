#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "board.hpp"

class GameServer {
private:
    int serverSocket;
    int playerSockets[2];
    std::string playerNames[2];
    GameBoard board;
    bool playerConnected[2];
    int currentPlayer;
    int moveCounter;

public:
    GameServer(const std::string& ip, int port) : moveCounter(0) {
        playerSockets[0] = -1;
        playerSockets[1] = -1;
        playerConnected[0] = false;
        playerConnected[1] = false;

        // Create socket
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket < 0) {
            throw std::runtime_error("Failed to create socket");
        }

        // Set socket options
        int opt = 1;
        if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            throw std::runtime_error("Failed to set socket options");
        }

        // Bind socket
        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = inet_addr(ip.c_str());
        address.sin_port = htons(port);

        if (bind(serverSocket, (struct sockaddr*)&address, sizeof(address)) < 0) {
            throw std::runtime_error("Socket binding failed");
        }

        // Listen for connections
        if (listen(serverSocket, 3) < 0) {
            throw std::runtime_error("Socket listen failed");
        }

        std::cout << "Server started on " << ip << ":" << port << std::endl;
    }

    ~GameServer() {
        close(serverSocket);
        if (playerSockets[0] >= 0) close(playerSockets[0]);
        if (playerSockets[1] >= 0) close(playerSockets[1]);
    }

    void run() {
        while (true) {
            acceptConnections();
            playGame();
            resetGame();
        }
    }

private:
    void acceptConnections() {
        std::cout << "Waiting for players to connect..." << std::endl;

        while (!playerConnected[0] || !playerConnected[1]) {
            struct sockaddr_in clientAddr;
            socklen_t addrLen = sizeof(clientAddr);
            int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &addrLen);

            if (clientSocket < 0) {
                std::cerr << "Error accepting connection" << std::endl;
                continue;
            }

            std::cout << "New connection accepted" << std::endl;

            // Send welcome message
            std::string welcomeMsg = "700";
            send(clientSocket, welcomeMsg.c_str(), welcomeMsg.length(), 0);

            // Receive player info
            char buffer[256] = {0};
            if (recv(clientSocket, buffer, sizeof(buffer) - 1, 0) <= 0) {
                std::cerr << "Error receiving player info" << std::endl;
                close(clientSocket);
                continue;
            }

            std::string playerInfo(buffer);
            std::cout << "Received: " << playerInfo << std::endl;

            // Parse player type and name
            int playerType;
            std::string playerName;
            try {
                int spacePos = playerInfo.find(' ');
                playerType = std::stoi(playerInfo.substr(0, spacePos));
                playerName = playerInfo.substr(spacePos + 1);
            } catch (const std::exception& e) {
                std::cerr << "Error parsing player info" << std::endl;
                close(clientSocket);
                continue;
            }

            // Check player type
            if (playerType != 1 && playerType != 2) {
                std::cerr << "Invalid player type: " << playerType << std::endl;
                close(clientSocket);
                continue;
            }

            // Check if player slot is available
            int playerIndex = playerType - 1;
            if (playerConnected[playerIndex]) {
                std::cerr << "Player slot " << playerType << " already taken" << std::endl;
                close(clientSocket);
                continue;
            }

            // Register player
            playerSockets[playerIndex] = clientSocket;
            playerNames[playerIndex] = playerName;
            playerConnected[playerIndex] = true;

            std::cout << "Player " << playerType << " (" << playerName << ") connected" << std::endl;
        }

        std::cout << "Both players connected. Starting game..." << std::endl;
    }

    void playGame() {
        board.reset();
        moveCounter = 0;
        currentPlayer = 0; // Player 1 starts

        // Notify first player to make a move
        std::string firstMoveMsg = "600";
        send(playerSockets[currentPlayer], firstMoveMsg.c_str(), firstMoveMsg.length(), 0);

        while (moveCounter < 25) {
            // Receive move from current player
            char buffer[16] = {0};

            int bytesReceived = recv(playerSockets[currentPlayer], buffer, sizeof(buffer) - 1, 0);
            if (bytesReceived <= 0) {
                std::cerr << "Player " << currentPlayer + 1 << " disconnected" << std::endl;
                endGame(1 - currentPlayer, 4); // Other player wins due to disconnect
                return;
            }

            std::string moveStr(buffer);
            std::cout << "Player " << currentPlayer + 1 << " move: " << moveStr << std::endl;

            int move;
            try {
                move = std::stoi(moveStr);
            } catch (const std::exception& e) {
                std::cerr << "Invalid move format from player " << currentPlayer + 1 << std::endl;
                endGame(1 - currentPlayer, 4); // Player loses due to error
                return;
            }

            // Process move
            if (!board.placeMove(move, currentPlayer + 1)) {
                std::cerr << "Invalid move from player " << currentPlayer + 1 << std::endl;
                endGame(1 - currentPlayer, 4); // Player loses due to error
                return;
            }

            moveCounter++;
            board.display();

            // Check for win/lose conditions
            if (board.checkWin(currentPlayer + 1)) {
                std::cout << "Player " << currentPlayer + 1 << " wins by making 4 in a row" << std::endl;
                endGame(currentPlayer, 1); // Current player wins
                return;
            }

            if (board.checkLose(currentPlayer + 1)) {
                std::cout << "Player " << currentPlayer + 1 << " loses by making forbidden 3 in a row" << std::endl;
                endGame(1 - currentPlayer, 1); // Current player loses
                return;
            }

            // Check for draw
            if (moveCounter == 25) {
                std::cout << "Draw - board is full" << std::endl;
                endGame(-1, 3); // Draw
                return;
            }

            // Switch to other player
            int nextPlayer = 1 - currentPlayer;

            // Send move notification to next player
            std::string notification = "0" + std::to_string(move);
            send(playerSockets[nextPlayer], notification.c_str(), notification.length(), 0);

            currentPlayer = nextPlayer;
        }
    }

    void endGame(int winningPlayer, int statusCode) {
        if (statusCode == 3) {
            // Draw
            std::string drawMsg = "300";
            send(playerSockets[0], drawMsg.c_str(), drawMsg.length(), 0);
            send(playerSockets[1], drawMsg.c_str(), drawMsg.length(), 0);
        }
        else if (statusCode == 4) {
            // Player error - disconnect or invalid move
            int losingPlayer = 1 - winningPlayer;

            std::string winMsg = "400";
            std::string loseMsg = "500";
            send(playerSockets[winningPlayer], winMsg.c_str(), winMsg.length(), 0);
            if (playerSockets[losingPlayer] >= 0) {
                send(playerSockets[losingPlayer], loseMsg.c_str(), loseMsg.length(), 0);
            }
        }
        else {
            // Regular win/loss (status code 1)
            int losingPlayer = 1 - winningPlayer;

            std::string winMsg = "100";
            std::string loseMsg = "200";

            send(playerSockets[winningPlayer], winMsg.c_str(), winMsg.length(), 0);
            send(playerSockets[losingPlayer], loseMsg.c_str(), loseMsg.length(), 0);
        }
    }

    void resetGame() {
        std::cout << "Game ended. Waiting 5 seconds before resetting..." << std::endl;
        sleep(5);

        close(playerSockets[0]);
        close(playerSockets[1]);
        playerSockets[0] = -1;
        playerSockets[1] = -1;
        playerConnected[0] = false;
        playerConnected[1] = false;
    }
};

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <IP> <PORT>" << std::endl;
        return 1;
    }

    std::string ip = argv[1];
    int port;
    try {
        port = std::stoi(argv[2]);
    } catch (const std::exception& e) {
        std::cerr << "Invalid port number" << std::endl;
        return 1;
    }

    try {
        GameServer server(ip, port);
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}