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
        for (int i = 0; i < BOARD_SIZE; i++) {
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

    // Kierunki do sprawdzania linii (poziomo, pionowo, ukośnie)
    const int directions[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};

public:
    MinimaxClient(const string& serverIP, int port, int player, const string& name, int depth)
        : playerNumber(player), playerName(name), maxDepth(depth), rng(chrono::steady_clock::now().time_since_epoch().count()) {
        mySymbol = (player == 1) ? 'X' : 'O';
        opponentSymbol = (player == 1) ? 'O' : 'X';

        // Tworzenie socketu
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            cerr << "Błąd tworzenia socketu" << endl;
            exit(1);
        }

        // Konfiguracja adresu serwera
        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr);

        // Połączenie z serwerem
        if (connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            cerr << "Błąd połączenia z serwerem" << endl;
            exit(1);
        }
    }

    ~MinimaxClient() {
        close(sockfd);
    }

    // Odbiera wiadomość od serwera
    string receiveMessage() {
        char buffer[256];
        memset(buffer, 0, sizeof(buffer));
        int n = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) {
            cerr << "Błąd odbierania wiadomości" << endl;
            exit(1);
        }
        // Usuwamy znaki nowej linii z końca
        string msg(buffer);
        while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r')) {
            msg.pop_back();
        }
        return msg;
    }

    // Wysyła wiadomość do serwera
    void sendMessage(const string& msg) {
        send(sockfd, msg.c_str(), msg.length(), 0);
    }

    // Konwertuje pozycję na notację serwera
    string positionToString(int row, int col) {
        return to_string(row + 1) + to_string(col + 1);
    }

    // Konwertuje notację serwera na pozycję
    void stringToPosition(const string& pos, int& row, int& col) {
        tow = pos[0] - '1';
        col = pos[1] - '1';
    }

    /*
     * FUNKCJA OCENY HEURYSTYCZNEJ
     *
     * Funkcja ocenia pozycję na planszy uwzględniając specyfikę gry:
     * - Wygrana: ustawienie 4 symboli w linii
     * - Przegrana: ustawienie dokładnie 3 symboli w linii (chyba że jest też czwórka)
     *
     * Strategia oceny:
     * 1. Sprawdzamy czy pozycja jest wygrywająca/przegrywająca
     * 2. Liczymy potencjał tworzenia czwórek (dobre linie)
     * 3. Unikamy tworzenia niebezpiecznych trójek
     * 4. Oceniamy kontrolę centrum planszy
     *
     * Wartości:
     * - Wygrana: +INF
     * - Przegrana: -INF
     * - Pozycje pośrednie: suma wartości heurystycznych
     */
    int evaluatePosition(char symbol) {
        int score = 0;

        // Sprawdzamy wszystkie możliwe linie na planszy
        for (int i = 0; i < BOARD_SIZE; i++) {
            for (int j = 0; j < BOARD_SIZE; j++) {
                for (int d = 0; d < 4; d++) {
                    int lineScore = evaluateLine(i, j, directions[d][0], directions[d][1], symbol);

                    // Jeśli znaleźliśmy wygraną lub przegraną, zwracamy natychmiast
                    if (lineScore == INF || lineScore == -INF) {
                        return lineScore;
                    }

                    score += lineScore;
                }
            }
        }

        // Bonus za kontrolę centrum planszy
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

    /*
     * Ocenia pojedynczą linię zaczynającą się od (row, col) w kierunku (dr, dc)
     *
     * Wartości zwracane:
     * - INF: znaleziono wygrywającą czwórkę
     * - -INF: znaleziono przegrywającą trójkę (bez czwórki)
     * - Wartość heurystyczna: potencjał linii
     */
    int evaluateLine(int row, int col, int dr, int dc, char symbol) {
        int myCount = 0;
        int opponentCount = 0;
        int emptyCount = 0;

        // Sprawdzamy maksymalnie WIN_LENGTH pól w danym kierunku
        for (int i = 0; i < WIN_LENGTH; i++) {
            int r = row + i * dr;
            int c = col + i * dc;

            if (r < 0 || r >= BOARD_SIZE || c < 0 || c >= BOARD_SIZE) {
                return 0; // Linia wychodzi poza planszę
            }

            if (board.grid[r][c] == symbol) {
                myCount++;
            } else if (board.grid[r][c] == ((symbol == 'X') ? 'O' : 'X')) {
                opponentCount++;
            } else {
                emptyCount++;
            }
        }

        // Jeśli linia jest zablokowana przez przeciwnika, nie ma wartości
        if (opponentCount > 0) {
            return 0;
        }

        // Sprawdzamy czy to wygrana (4 w linii)
        if (myCount == WIN_LENGTH) {
            return INF;
        }

        // Sprawdzamy czy to przegrana (dokładnie 3 w linii)
        // Musimy też sprawdzić czy nie ma przypadkiem czwórki w szerszym kontekście
        if (myCount == LOSE_LENGTH && emptyCount == 1) {
            // Sprawdzamy czy ta trójka jest częścią czwórki
            bool partOfFour = checkIfPartOfFour(row, col, dr, dc, symbol);
            if (!partOfFour) {
                return -INF; // Przegrywająca trójka
            }
        }

        // Ocena heurystyczna potencjału linii
        if (myCount == 3 && emptyCount == 1) {
            return 50; // Prawie wygrana
        } else if (myCount == 2 && emptyCount == 2) {
            return 20; // Dobra pozycja
        } else if (myCount == 1 && emptyCount == 3) {
            return 5; // Początek linii
        }

        return 0;
    }

    /*
     * Sprawdza czy trójka jest częścią czwórki
     * Przegląda sąsiednie pola w tym samym kierunku
     */
    bool checkIfPartOfFour(int row, int col, int dr, int dc, char symbol) {
        // Sprawdzamy pole przed linią
        int prevRow = row - dr;
        int prevCol = col - dc;
        if (prevRow >= 0 && prevRow < BOARD_SIZE && prevCol >= 0 && prevCol < BOARD_SIZE) {
            if (board.grid[prevRow][prevCol] == symbol) {
                return true; // Jest częścią czwórki
            }
        }

        // Sprawdzamy pole za linią
        int nextRow = row + WIN_LENGTH * dr;
        int nextCol = col + WIN_LENGTH * dc;
        if (nextRow >= 0 && nextRow < BOARD_SIZE && nextCol >= 0 && nextCol < BOARD_SIZE) {
            if (board.grid[nextRow][nextCol] == symbol) {
                return true; // Jest częścią czwórki
            }
        }

        return false;
    }

    /*
     * Sprawdza stan gry po ruchu
     * Zwraca:
     * 1 - wygrana gracza symbol
     * -1 - przegrana gracza symbol (utworzył trójkę bez czwórki)
     * 0 - gra trwa
     */
    int checkGameState(int lastRow, int lastCol, char symbol) {
        // Sprawdzamy wszystkie linie przechodzące przez ostatni ruch
        for (int d = 0; d < 4; d++) {
            int dr = directions[d][0];
            int dc = directions[d][1];

            // Sprawdzamy linie zaczynające się do WIN_LENGTH-1 pól przed ostatnim ruchem
            for (int offset = 0; offset < WIN_LENGTH; offset++) {
                int startRow = lastRow - offset * dr;
                int startCol = lastCol - offset * dc;

                if (startRow >= 0 && startRow < BOARD_SIZE && startCol >= 0 && startCol < BOARD_SIZE) {
                    int count = 0;
                    bool valid = true;

                    // Liczymy symbole w linii
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
                        return 1; // Wygrana
                    }
                }
            }
        }

        // Sprawdzamy czy utworzono przegrywającą trójkę
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
                        // Sprawdzamy czy ta trójka nie jest częścią czwórki
                        if (!checkIfPartOfFour(startRow, startCol, dr, dc, symbol)) {
                            hasLosingThree = true;
                        }
                    }
                }
            }
        }

        if (hasLosingThree && !hasWinningFour) {
            return -1; // Przegrana
        }

        return 0; // Gra trwa
    }

    /*
     * Algorytm minimax z obcinaniem alfa-beta
     * Zwraca najlepszy ruch wraz z jego oceną
     */
    Move minimax(int depth, bool isMaximizing, int alpha, int beta) {
        // Sprawdzamy czy osiągnęliśmy maksymalną głębokość
        if (depth == 0) {
            Move move;
            move.score = evaluatePosition(mySymbol) - evaluatePosition(opponentSymbol);
            return move;
        }

        // Zbieramy wszystkie możliwe ruchy
        vector<Move> possibleMoves;
        for (int i = 0; i < BOARD_SIZE; i++) {
            for (int j = 0; j < BOARD_SIZE; j++) {
                if (board.isEmptyCell(i, j)) {
                    possibleMoves.push_back(Move(i, j, 0));
                }
            }
        }

        // Jeśli nie ma ruchów, zwracamy ocenę aktualnej pozycji
        if (possibleMoves.empty()) {
            Move move;
            move.score = evaluatePosition(mySymbol) - evaluatePosition(opponentSymbol);
            return move;
        }

        Move bestMove;
        vector<Move> equalMoves; // Ruchy o tej samej najlepszej wartości

        if (isMaximizing) {
            bestMove.score = -INF - 1;

            for (const auto& move : possibleMoves) {
                // Wykonujemy ruch
                board.makeMove(move.row, move.col, mySymbol);

                // Sprawdzamy stan gry
                int gameState = checkGameState(move.row, move.col, mySymbol);

                Move currentMove = move;
                if (gameState == 1) {
                    currentMove.score = INF;
                } else if (gameState == -1) {
                    currentMove.score = -INF;
                } else {
                    // Rekurencyjne wywołanie minimax
                    Move nextMove = minimax(depth - 1, false, alpha, beta);
                    currentMove.score = nextMove.score;
                }

                // Cofamy ruch
                board.undoMove(move.row, move.col);

                // Aktualizujemy najlepszy ruch
                if (currentMove.score > bestMove.score) {
                    bestMove = currentMove;
                    equalMoves.clear();
                    equalMoves.push_back(currentMove);
                } else if (currentMove.score == bestMove.score) {
                    equalMoves.push_back(currentMove);
                }

                // Obcinanie alfa-beta
                alpha = max(alpha, currentMove.score);
                if (beta <= alpha) {
                    break;
                }
            }
        } else {
            bestMove.score = INF + 1;

            for (const auto& move : possibleMoves) {
                // Wykonujemy ruch przeciwnika
                board.makeMove(move.row, move.col, opponentSymbol);

                // Sprawdzamy stan gry
                int gameState = checkGameState(move.row, move.col, opponentSymbol);

                Move currentMove = move;
                if (gameState == 1) {
                    currentMove.score = -INF;
                } else if (gameState == -1) {
                    currentMove.score = INF;
                } else {
                    // Rekurencyjne wywołanie minimax
                    Move nextMove = minimax(depth - 1, true, alpha, beta);
                    currentMove.score = nextMove.score;
                }

                // Cofamy ruch
                board.undoMove(move.row, move.col);

                // Aktualizujemy najlepszy ruch
                if (currentMove.score < bestMove.score) {
                    bestMove = currentMove;
                    equalMoves.clear();
                    equalMoves.push_back(currentMove);
                } else if (currentMove.score == bestMove.score) {
                    equalMoves.push_back(currentMove);
                }

                // Obcinanie alfa-beta
                beta = min(beta, currentMove.score);
                if (beta <= alpha) {
                    break;
                }
            }
        }

        // Losowy wybór spośród ruchów o tej samej wartości
        if (!equalMoves.empty()) {
            uniform_int_distribution<int> dist(0, equalMoves.size() - 1);
            bestMove = equalMoves[dist(rng)];
        }

        return bestMove;
    }

    // Główna pętla gry
  void play() {
    // Czekamy na komunikat 700
    string msg = receiveMessage();
    cout << "Otrzymano komunikat startowy: '" << msg << "'" << endl;
    if (msg.substr(0, 3) != "700") {
        cerr << "Nieoczekiwany komunikat: " << msg << endl;
        return;
    }

    // Wysyłamy numer gracza i nazwę
    string response = to_string(playerNumber) + " " + playerName;
    sendMessage(response);
    cout << "Wysłano dane gracza: " << response << endl;

    // Gracz 1 otrzymuje komunikat 600
    if (playerNumber == 1) {
        msg = receiveMessage();
        cout << "Otrzymano komunikat: '" << msg << "'" << endl;
        if (msg.substr(0, 3) != "600") {
            cerr << "Nieoczekiwany komunikat: " << msg << endl;
            return;
        }

        // Wykonujemy pierwszy ruch
        Move bestMove = minimax(maxDepth, true, -INF - 1, INF + 1);
        string moveStr = positionToString(bestMove.row, bestMove.col);
        sendMessage(moveStr);
        board.makeMove(bestMove.row, bestMove.col, mySymbol);

        cout << "Mój ruch: " << moveStr << endl;
    }

    // Główna pętla gry
    while (true) {
        // Odbieramy komunikat
        msg = receiveMessage();
        cout << "Otrzymano komunikat: '" << msg << "'" << endl;

        // Parse the message
        int messageCode = stoi(msg);
        int messageType = messageCode / 100;  // First digit(s) for message type
        int opponentMove = messageCode % 100; // Last two digits for move

        // Check for end game messages (100-500)
        if (messageType >= 1 && messageType <= 5) {
            // Game end message
            if (messageType == 1) cout << "Wygrałem!" << endl;
            else if (messageType == 2) cout << "Przegrałem!" << endl;
            else if (messageType == 3) cout << "Remis!" << endl;
            else if (messageType == 4) cout << "Wygrałem. Przeciwnik popełnił błąd." << endl;
            else if (messageType == 5) cout << "Przegrałem. Mój błąd." << endl;
            break;
        }

        // It's a move message - update the board with opponent's move
        if (opponentMove >= 11 && opponentMove <= 55) {
            int row = (opponentMove / 10) - 1;
            int col = (opponentMove % 10) - 1;
            board.makeMove(row, col, opponentSymbol);
            cout << "Ruch przeciwnika: " << opponentMove << endl;
        }

        // Make our move
        Move bestMove = minimax(maxDepth, true, -INF - 1, INF + 1);
        string moveStr = positionToString(bestMove.row, bestMove.col);
        sendMessage(moveStr);
        board.makeMove(bestMove.row, bestMove.col, mySymbol);

        cout << "Mój ruch: " << moveStr << endl;
    }
}
};

int main(int argc, char* argv[]) {



    if (argc != 6) {
        cerr << "Użycie: " << argv[0] << " <adres_ip> <port> <numer_gracza> <nazwa> <głębokość>" << endl;
        return 1;
    }

    string serverIP = argv[1];
    int port = atoi(argv[2]);
    int playerNumber = atoi(argv[3]);
    string playerName = argv[4];
    int depth = atoi(argv[5]);

    // Walidacja parametrów
    if (playerNumber != 1 && playerNumber != 2) {
        cerr << "Numer gracza musi być 1 lub 2" << endl;
        return 1;
    }

    if (playerName.length() > 9) {
        cerr << "Nazwa gracza może mieć maksymalnie 9 znaków" << endl;
        return 1;
    }

    if (depth < 1 || depth > MAX_DEPTH) {
        cerr << "Głębokość musi być w zakresie 1-10" << endl;
        return 1;
    }

    // Tworzenie i uruchomienie klienta
    MinimaxClient client(serverIP, port, playerNumber, playerName, depth);
    client.play();

    return 0;
}