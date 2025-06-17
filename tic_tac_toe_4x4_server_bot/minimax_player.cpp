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

struct Move {
    int row;
    int col;
    int score;

    Move(int r = -1, int c = -1, int s = 0) : row(r), col(c), score(s) {}
};

class MinimaxClient {
private:
    int sockfd;
    char mySymbol;
    char opponentSymbol;
    int playerNumber;
    string playerName;
    int maxDepth;
    Board board;
    mt19937 rng;
    bool useAI;

    const int directions[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};

public:
    MinimaxClient(const string& serverIP, int port, int player, const string& name, int depth, bool ai)
        : playerNumber(player), playerName(name), maxDepth(depth), useAI(ai),
          rng(chrono::steady_clock::now().time_since_epoch().count()) {
        mySymbol = (player == 1) ? 'X' : 'O';
        opponentSymbol = (player == 1) ? 'O' : 'X';

        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            cerr << "Socket creation error" << endl;
            exit(1);
        }

        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr);

        if (connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            cerr << "Connection error" << endl;
            exit(1);
        }
    }

    ~MinimaxClient() {
        close(sockfd);
    }

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

    void sendMessage(const string& msg) {
        send(sockfd, msg.c_str(), msg.length(), 0);
    }

    string positionToString(int row, int col) {
        return to_string(row + 1) + to_string(col + 1);
    }

    void stringToPosition(const string& pos, int& row, int& col) {
        row = pos[0] - '1';
        col = pos[1] - '1';
    }

    int evaluatePosition(char symbol) {
        int score = 0;

        for (int i = 0; i < BOARD_SIZE; i++) {
            for (int j = 0; j < BOARD_SIZE; j++) {
                for (int d = 0; d < 4; d++) {
                    int lineScore = evaluateLine(i, j, directions[d][0], directions[d][1], symbol);

                    if (lineScore == INF || lineScore == -INF) {
                        return lineScore;
                    }

                    score += lineScore;
                }
            }
        }

        int centerBonus = 0;
        int center = BOARD_SIZE / 2;
        for (int i = center - 1; i <= center + 1; i++) {
            for (int j = center - 1; j <= center + 1; j++) {
                if (i >= 0 && i < BOARD_SIZE && j >= 0 && j < BOARD_SIZE) {
                    if (board.grid[i][j] == symbol) {
                        centerBonus += 10;
                    }
                }
            }
        }

        score += centerBonus;
        return score;
    }

    int evaluateLine(int row, int col, int dr, int dc, char symbol) {
        int myCount = 0;
        int opponentCount = 0;
        int emptyCount = 0;

        for (int i = 0; i < WIN_LENGTH; i++) {
            int r = row + i * dr;
            int c = col + i * dc;

            if (r < 0 || r >= BOARD_SIZE || c < 0 || c >= BOARD_SIZE) {
                return 0;
            }

            if (board.grid[r][c] == symbol) {
                myCount++;
            } else if (board.grid[r][c] == ((symbol == 'X') ? 'O' : 'X')) {
                opponentCount++;
            } else {
                emptyCount++;
            }
        }

        if (opponentCount > 0) {
            return 0;
        }

        if (myCount == WIN_LENGTH) {
            return INF;
        }

        if (myCount == LOSE_LENGTH && emptyCount == 1) {
            bool partOfFour = checkIfPartOfFour(row, col, dr, dc, symbol);
            if (!partOfFour) {
                return -INF;
            }
        }

        if (myCount == 3 && emptyCount == 1) {
            return 50;
        } else if (myCount == 2 && emptyCount == 2) {
            return 20;
        } else if (myCount == 1 && emptyCount == 3) {
            return 5;
        }

        return 0;
    }

    bool checkIfPartOfFour(int row, int col, int dr, int dc, char symbol) {
        int prevRow = row - dr;
        int prevCol = col - dc;
        if (prevRow >= 0 && prevRow < BOARD_SIZE && prevCol >= 0 && prevCol < BOARD_SIZE) {
            if (board.grid[prevRow][prevCol] == symbol) {
                return true;
            }
        }

        int nextRow = row + WIN_LENGTH * dr;
        int nextCol = col + WIN_LENGTH * dc;
        if (nextRow >= 0 && nextRow < BOARD_SIZE && nextCol >= 0 && nextCol < BOARD_SIZE) {
            if (board.grid[nextRow][nextCol] == symbol) {
                return true;
            }
        }

        return false;
    }

    int checkGameState(int lastRow, int lastCol, char symbol) {
        for (int d = 0; d < 4; d++) {
            int dr = directions[d][0];
            int dc = directions[d][1];

            for (int offset = 0; offset < WIN_LENGTH; offset++) {
                int startRow = lastRow - offset * dr;
                int startCol = lastCol - offset * dc;

                if (startRow >= 0 && startRow < BOARD_SIZE && startCol >= 0 && startCol < BOARD_SIZE) {
                    int count = 0;
                    bool valid = true;

                    for (int i = 0; i < WIN_LENGTH; i++) {
                        int r = startRow + i * dr;
                        int c = startCol + i * dc;

                        if (r < 0 || r >= BOARD_SIZE || c < 0 || c >= BOARD_SIZE) {
                            valid = false;
                            break;
                        }

                        if (board.grid[r][c] == symbol) {
                            count++;
                        }
                    }

                    if (valid && count == WIN_LENGTH) {
                        return 1;
                    }
                }
            }
        }

        bool hasLosingThree = false;
        bool hasWinningFour = false;

        for (int d = 0; d < 4; d++) {
            int dr = directions[d][0];
            int dc = directions[d][1];

            for (int offset = 0; offset < LOSE_LENGTH; offset++) {
                int startRow = lastRow - offset * dr;
                int startCol = lastCol - offset * dc;

                if (startRow >= 0 && startRow < BOARD_SIZE && startCol >= 0 && startCol < BOARD_SIZE) {
                    int count = 0;
                    bool valid = true;

                    for (int i = 0; i < LOSE_LENGTH; i++) {
                        int r = startRow + i * dr;
                        int c = startCol + i * dc;

                        if (r < 0 || r >= BOARD_SIZE || c < 0 || c >= BOARD_SIZE) {
                            valid = false;
                            break;
                        }

                        if (board.grid[r][c] == symbol) {
                            count++;
                        }
                    }

                    if (valid && count == LOSE_LENGTH) {
                        if (!checkIfPartOfFour(startRow, startCol, dr, dc, symbol)) {
                            hasLosingThree = true;
                        }
                    }
                }
            }
        }

        if (hasLosingThree && !hasWinningFour) {
            return -1;
        }

        return 0;
    }

    Move minimax(int depth, bool isMaximizing, int alpha, int beta) {
        if (depth == 0) {
            Move move;
            move.score = evaluatePosition(mySymbol) - evaluatePosition(opponentSymbol);
            return move;
        }

        vector<Move> possibleMoves;
        for (int i = 0; i < BOARD_SIZE; i++) {
            for (int j = 0; j < BOARD_SIZE; j++) {
                if (board.isEmptyCell(i, j)) {
                    possibleMoves.push_back(Move(i, j, 0));
                }
            }
        }

        if (possibleMoves.empty()) {
            Move move;
            move.score = evaluatePosition(mySymbol) - evaluatePosition(opponentSymbol);
            return move;
        }

        Move bestMove;
        vector<Move> equalMoves;

        if (isMaximizing) {
            bestMove.score = -INF - 1;

            for (const auto& move : possibleMoves) {
                board.makeMove(move.row, move.col, mySymbol);

                int gameState = checkGameState(move.row, move.col, mySymbol);

                Move currentMove = move;
                if (gameState == 1) {
                    currentMove.score = INF;
                } else if (gameState == -1) {
                    currentMove.score = -INF;
                } else {
                    Move nextMove = minimax(depth - 1, false, alpha, beta);
                    currentMove.score = nextMove.score;
                }

                board.undoMove(move.row, move.col);

                if (currentMove.score > bestMove.score) {
                    bestMove = currentMove;
                    equalMoves.clear();
                    equalMoves.push_back(currentMove);
                } else if (currentMove.score == bestMove.score) {
                    equalMoves.push_back(currentMove);
                }

                alpha = max(alpha, currentMove.score);
                if (beta <= alpha) {
                    break;
                }
            }
        } else {
            bestMove.score = INF + 1;

            for (const auto& move : possibleMoves) {
                board.makeMove(move.row, move.col, opponentSymbol);

                int gameState = checkGameState(move.row, move.col, opponentSymbol);

                Move currentMove = move;
                if (gameState == 1) {
                    currentMove.score = -INF;
                } else if (gameState == -1) {
                    currentMove.score = INF;
                } else {
                    Move nextMove = minimax(depth - 1, true, alpha, beta);
                    currentMove.score = nextMove.score;
                }

                board.undoMove(move.row, move.col);

                if (currentMove.score < bestMove.score) {
                    bestMove = currentMove;
                    equalMoves.clear();
                    equalMoves.push_back(currentMove);
                } else if (currentMove.score == bestMove.score) {
                    equalMoves.push_back(currentMove);
                }

                beta = min(beta, currentMove.score);
                if (beta <= alpha) {
                    break;
                }
            }
        }

        if (!equalMoves.empty()) {
            uniform_int_distribution<int> dist(0, equalMoves.size() - 1);
            bestMove = equalMoves[dist(rng)];
        }

        return bestMove;
    }

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

    void makeAIMove() {
        cout << "AI is thinking..." << endl;
        Move bestMove = minimax(maxDepth, true, -INF - 1, INF + 1);
        int row = bestMove.row;
        int col = bestMove.col;
        board.makeMove(row, col, mySymbol);
        string mv = positionToString(row, col);
        sendMessage(mv);
        cout << "AI moved: " << row+1 << "," << col+1 << " (score: " << bestMove.score << ")" << endl;
        board.print();
    }

    void play() {
        string msg = receiveMessage();
        cout << "Server: " << msg << endl;
        if (msg.substr(0, 3) != "700") {
            cerr << "Unexpected message: " << msg << endl;
            return;
        }

        string response = to_string(playerNumber) + " " + playerName;
        sendMessage(response);
        cout << "Sent: " << response << endl;

        if (playerNumber == 1) {
            msg = receiveMessage();
            cout << "Server: " << msg << endl;
            if (msg.substr(0, 3) != "600") {
                cerr << "Unexpected message: " << msg << endl;
                return;
            }
            board.print();
            if (useAI) {
                makeAIMove();
            } else {
                makeHumanMove();
            }
        }

        while (true) {
            msg = receiveMessage();
            cout << "Server: " << msg << endl;
            int code = stoi(msg);
            int type = code / 100;
            int mv = code % 100;

            if (type >= 1 && type <= 5) {
                if (type == 1) cout << "You win!" << endl;
                else if (type == 2) cout << "You lose!" << endl;
                else if (type == 3) cout << "Draw!" << endl;
                else if (type == 4) cout << "You win; opponent error." << endl;
                else if (type == 5) cout << "You lose; your error." << endl;
                break;
            }

            if (mv >= 11 && mv <= 55) {
                int row = (mv / 10) - 1;
                int col = (mv % 10) - 1;
                board.makeMove(row, col, opponentSymbol);
                cout << "Opponent moved: " << row+1 << "," << col+1 << endl;
                board.print();
            }

            if (useAI) {
                makeAIMove();
            } else {
                makeHumanMove();
            }
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc < 5 || argc > 7) {
        cerr << "Usage: " << argv[0] << " <server_ip> <port> <player_number> <name> [depth] [ai]" << endl;
        cerr << "  depth: AI depth (1-10), default=5" << endl;
        cerr << "  ai: 0=human, 1=AI (default=0)" << endl;
        return 1;
    }

    string serverIP = argv[1];
    int port = atoi(argv[2]);
    int playerNumber = atoi(argv[3]);
    string playerName = argv[4];
    int depth = (argc > 5) ? atoi(argv[5]) : 5;
    bool useAI = (argc > 6) ? (atoi(argv[6]) > 0) : false;

    if (playerNumber != 1 && playerNumber != 2) {
        cerr << "Player number must be 1 or 2" << endl;
        return 1;
    }

    if (playerName.length() > 9) {
        cerr << "Name can be max 9 chars" << endl;
        return 1;
    }

    if (depth < 1 || depth > MAX_DEPTH) {
        cerr << "Depth must be between 1-10" << endl;
        return 1;
    }

    cout << (useAI ? "AI" : "Human") << " player mode with depth=" << depth << endl;

    MinimaxClient client(serverIP, port, playerNumber, playerName, depth, useAI);
    client.play();
    return 0;
}