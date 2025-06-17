#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include "board.hpp"

class GameClient {
private:
    int socket;
    GameBoard board;
    int playerType;

public:
    GameClient(const std::string& ip, int port, int playerType, const std::string& name)
        : playerType(playerType) {
        connectToServer(ip, port);
        authenticate(playerType, name);
        playGame();
    }

    ~GameClient() {
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

        std::cout << "Connected to server\n";
    }

    void authenticate(int playerType, const std::string& name) {
        std::string serverMsg = receiveMessage();
        std::cout << "Server: " << serverMsg << "\n";

        std::string authMsg = std::to_string(playerType) + " " + name;
        sendMessage(authMsg);
    }

    void playGame() {
        board.reset();
        bool gameEnded = false;

        while (!gameEnded) {
            std::string serverMsg = receiveMessage();
            std::cout << "Server: " << serverMsg << "\n";

            int code = std::stoi(serverMsg);
            int moveCode = code % 100;
            int statusCode = code / 100;

            if (moveCode != 0) {
                board.placeMove(moveCode, 3 - playerType);
                board.display();
            }

            if (statusCode == 0 || statusCode == 6) {
                // Our turn to move
                int move = getPlayerMove();
                board.placeMove(move, playerType);
                board.display();

                sendMessage(std::to_string(move));
            } else {
                // Game ended
                gameEnded = true;
                displayGameResult(statusCode);
            }
        }
    }

    int getPlayerMove() {
        int move;
        std::cout << "Your move: ";
        std::cin >> move;
        return move;
    }

    void displayGameResult(int code) {
        switch (code) {
            case 1: std::cout << "You won!\n"; break;
            case 2: std::cout << "You lost.\n"; break;
            case 3: std::cout << "Draw.\n"; break;
            case 4: std::cout << "You won. Opponent error.\n"; break;
            case 5: std::cout << "You lost. Your error.\n"; break;
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
        GameClient client(
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
