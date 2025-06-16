#include <iostream>
#include <cstring>
#include <cstdlib>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <algorithm>
#include <limits>
#include <random>
#include <chrono>

using namespace std;

const int BOARD_SIZE = 5;
const int WIN_LENGTH = 4;
const int LOSE_LENGTH = 3;
const int MAX_DEPTH = 10;
const int INF = 1000000;

struct Board {
    char grid[BOARD_SIZE][BOARD_SIZE];

    Board() {
        for (int i = 0; i < BOARD_SIZE; i++) {
            for (int j = 0; j < BOARD_SIZE; j++) {
                grid[i][j] = '.';
            }
        }
    }

    bool isEmptyCell(int row, int col) const {
        return grid[row][col] == '.';
    }

    void makeMove(int row, int col, char symbol) {
        grid[row][col] = symbol;
    }

    void undoMove(int row, int col) {
        grid[row][col] = '.';
    }

    void print() const {
        cout << "  1 2 3 4 5\n";
        for (int i = 0; i < BOARD_SIZE; i++) {
            cout << i+1 << " ";
            for (int j = 0; j < BOARD_SIZE; j++) {
                cout << grid[i][j] << " ";
            }
            cout << endl;
        }
    }
};

class MinimaxClient {
private:
    int sockfd;
    char mySymbol;
    char opponentSymbol;
    int playerNumber;
    string playerName;
    Board board;

    // Directions for checking lines
    const int directions[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};

public:
    MinimaxClient(const string& serverIP, int port, int player, const string& name)
        : playerNumber(player), playerName(name) {
        mySymbol = (player == 1) ? 'X' : 'O';
        opponentSymbol = (player == 1) ? 'O' : 'X';

        // Create socket
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            cerr << "Error creating socket" << endl;
            exit(1);
        }

        // Configure server address
        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr);

        // Connect to server
        if (connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            cerr << "Error connecting to server" << endl;
            exit(1);
        }
    }

    ~MinimaxClient() {
        close(sockfd);
    }

    // Receive message from server
    string receiveMessage() {
        char buffer[256];
        memset(buffer, 0, sizeof(buffer));
        int n = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) {
            cerr << "Error receiving message" << endl;
            exit(1);
        }
        string msg(buffer);
        while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r')) {
            msg.pop_back();
        }
        return msg;
    }

    // Send message to server
    void sendMessage(const string& msg) {
        send(sockfd, msg.c_str(), msg.length(), 0);
    }

    // Convert position to server notation
    string positionToString(int row, int col) {
        return to_string(row + 1) + to_string(col + 1);
    }

    void stringToPosition(const string& pos, int& row, int& col) {
        row = pos[0] - '1';
        col = pos[1] - '1';
    }
    // Main game loop
    void play() {
        // Wait for start message 700
        string msg = receiveMessage();
        cout << "Server: " << msg << endl;
        if (msg.substr(0, 3) != "700") {
            cerr << "Unexpected message: " << msg << endl;
            return;
        }

        // Send player number and name
        string response = to_string(playerNumber) + " " + playerName;
        sendMessage(response);
        cout << "Sent: " << response << endl;

        // If player 1, server sends 600 and we make the first move
        if (playerNumber == 1) {
            msg = receiveMessage();
            cout << "Server: " << msg << endl;
            if (msg.substr(0, 3) != "600") {
                cerr << "Unexpected message: " << msg << endl;
                return;
            }
            board.print();
            makeHumanMove();
        }

        // Game loop
        while (true) {
            msg = receiveMessage();
            cout << "Server: " << msg << endl;
            int code = stoi(msg);
            int type = code / 100;
            int mv = code % 100;

            // End-game messages 1xx-5xx
            if (type >= 1 && type <= 5) {
                if (type == 1) cout << "You win!" << endl;
                else if (type == 2) cout << "You lose!" << endl;
                else if (type == 3) cout << "Draw!" << endl;
                else if (type == 4) cout << "You win; opponent error." << endl;
                else if (type == 5) cout << "You lose; your error." << endl;
                break;
            }

            // Opponent move
            if (mv >= 11 && mv <= 55) {
                int row = (mv / 10) - 1;
                int col = (mv % 10) - 1;
                board.makeMove(row, col, opponentSymbol);
                cout << "Opponent moved: " << row+1 << "," << col+1 << endl;
                board.print();
            }

            // Human move
            makeHumanMove();
        }
    }

    // Prompt human for move, send to server
    void makeHumanMove() {
        int row, col;
        while (true) {
            cout << "Your turn. Enter row and column (1-5): ";
            if (!(cin >> row >> col)) {
                cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Invalid input. Try again." << endl;
                continue;
            }
            if (row < 1 || row > BOARD_SIZE || col < 1 || col > BOARD_SIZE) {
                cout << "Out of range. Try again." << endl;
                continue;
            }
            if (!board.isEmptyCell(row-1, col-1)) {
                cout << "Cell occupied. Try again." << endl;
                continue;
            }
            break;
        }
        board.makeMove(row-1, col-1, mySymbol);
        string mv = positionToString(row-1, col-1);
        sendMessage(mv);
        cout << "You moved: " << row << "," << col << endl;
        board.print();
    }
};

int main(int argc, char* argv[]) {
    if (argc != 5) {
        cerr << "Usage: " << argv[0] << " <server_ip> <port> <player_number> <name>" << endl;
        return 1;
    }
    string serverIP = argv[1];
    int port = atoi(argv[2]);
    int playerNumber = atoi(argv[3]);
    string playerName = argv[4];

    if (playerNumber != 1 && playerNumber != 2) {
        cerr << "Player number must be 1 or 2" << endl;
        return 1;
    }
    if (playerName.length() > 9) {
        cerr << "Name can be max 9 chars" << endl;
        return 1;
    }

    MinimaxClient client(serverIP, port, playerNumber, playerName);
    client.play();
    return 0;
}