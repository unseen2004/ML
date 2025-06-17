#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <random>
#include <arpa/inet.h>
#include "board.hpp"

class RandomBot {
private:
    int socket;
    GameBoard board;
    int playerType;
    std::mt19937 rng;

public:
    RandomBot(const std::string& ip, int port, int playerType, const std::string& name)
        : playerType(playerType), rng(std::random_device()()) {
        connectToServer(ip, port);
        authenticate(playerType, name);
        playGame();
    }

    ~RandomBot() {
        close(socket);
    }

private:
    void connectToServer(const std::string& ip, int port) {
        socket = ::socket(AF_INET, SOCK_STREAM, 0);
        if (socket < 0) {
            throw std::runtime_error("Failed to create socket");
        }

        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());

        if (connect(socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            throw std::runtime_error("Connection failed");
        }
    }

    void authenticate(int playerType, const std::string& name) {
        receiveMessage(); // Ignore the welcome message

        std::string authMsg = std::to_string(playerType) + " " + name;
        sendMessage(authMsg);
    }

    void playGame() {
        board.reset();
        bool gameEnded = false;

        while (!gameEnded) {
            std::string serverMsg = receiveMessage();

            int code = std::stoi(serverMsg);
            int moveCode = code % 100;
            int statusCode = code / 100;

            if (moveCode != 0) {
                board.placeMove(moveCode, 3 - playerType);
            }

            if (statusCode == 0 || statusCode == 6) {
                // Our turn to move
                int move = generateRandomMove();
                board.placeMove(move, playerType);

                sendMessage(std::to_string(move));
            } else {
                // Game ended
                gameEnded = true;
                displayGameResult(statusCode);
            }
        }
    }

    int generateRandomMove() {
        std::vector<int> validMoves;

        for (int row = 0; row < 5; row++) {
            for (int col = 0; col < 5; col++) {
                if (board.getCellValue(row, col) == 0) {
                    validMoves.push_back((row + 1) * 10 + (col + 1));
                }
            }
        }

        if (validMoves.empty()) {
            return 0; // No valid moves
        }

        std::uniform_int_distribution<int> dist(0, validMoves.size() - 1);
        return validMoves[dist(rng)];
    }

    void displayGameResult(int code) {
        switch (code) {
            case 1: std::cout << "Bot won.\n"; break;
            case 2: std::cout << "Bot lost.\n"; break;
            case 3: std::cout << "Draw.\n"; break;
            case 4: std::cout << "Bot won. Opponent error.\n"; break;
            case 5: std::cout << "Bot lost. Bot error.\n"; break;
        }
    }

    void sendMessage(const std::string& message) {
        if (send(socket, message.c_str(), message.length(), 0) < 0) {
            throw std::runtime_error("Failed to send message");
        }
    }

    std::string receiveMessage() {
        char buffer[16] = {0};
        if (recv(socket, buffer, sizeof(buffer), 0) < 0) {
            throw std::runtime_error("Failed to receive message");
        }
        return std::string(buffer);
    }
};

int main(int argc, char *argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <IP> <PORT> <PLAYER_TYPE> <NAME>\n";
        return 1;
    }

    try {
        RandomBot bot(
            argv[1],
            std::stoi(argv[2]),
            std::stoi(argv[3]),
            argv[4]
        );
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}