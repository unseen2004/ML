#include <iostream>
#include <vector>
#include <queue>
#include <map>
#include <set>
#include <algorithm>
#include <random>
#include <chrono>
#include <cmath>

using namespace std;

// Struktura reprezentująca stan planszy
struct State {
    vector<vector<int>> board;
    int empty_row, empty_col;
    int g_cost;  // Koszt dotarcia do tego stanu
    int h_cost;  // Heurystyczny koszt do celu
    int f_cost;  // f = g + h
    State* parent;  // Wskaźnik na poprzedni stan
    int moved_tile;  // Kafelek, który został przesunięty

    State() : g_cost(0), h_cost(0), f_cost(0), parent(nullptr), moved_tile(0) {}

    // Operator porównania dla kolejki priorytetowej
    bool operator<(const State& other) const {
        return f_cost > other.f_cost;  // Mniejszy koszt = wyższy priorytet
    }
};

class FifteenPuzzle {
private:
    int size;
    int total_tiles;
    vector<vector<int>> goal_state;

    // Możliwe ruchy: góra, dół, lewo, prawo
    const int dx[4] = {-1, 1, 0, 0};
    const int dy[4] = {0, 0, -1, 1};

public:
    FifteenPuzzle(int n) : size(n), total_tiles(n * n) {
        // Inicjalizacja stanu docelowego
        goal_state.resize(size, vector<int>(size));
        int num = 1;
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                goal_state[i][j] = num % total_tiles;
                num++;
            }
        }
    }

    // Sprawdzenie czy dana permutacja ma rozwiązanie
    bool isSolvable(const vector<vector<int>>& board) {
        // Zliczamy inwersje (pary elementów w złej kolejności)
        vector<int> flat;
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                if (board[i][j] != 0) {
                    flat.push_back(board[i][j]);
                }
            }
        }

        int inversions = 0;
        for (int i = 0; i < flat.size(); i++) {
            for (int j = i + 1; j < flat.size(); j++) {
                if (flat[i] > flat[j]) {
                    inversions++;
                }
            }
        }

        // Dla planszy o nieparzystym rozmiarze: rozwiązywalna gdy parzysta liczba inwersji
        if (size % 2 == 1) {
            return inversions % 2 == 0;
        }
        // Dla planszy o parzystym rozmiarze
        else {
            int empty_row = 0;
            for (int i = 0; i < size; i++) {
                for (int j = 0; j < size; j++) {
                    if (board[i][j] == 0) {
                        empty_row = size - i;  // Liczymy od dołu
                        break;
                    }
                }
            }
            return (inversions + empty_row) % 2 == 1;
        }
    }

    // Generowanie stanu przez k losowych ruchów od stanu docelowego
    State generateRandomState() {
        int k = 50;
        // Zacznij od stanu docelowego:
        State state;
        state.board = goal_state;
        state.empty_row = size - 1;
        state.empty_col = size - 1;
        state.g_cost = state.h_cost = state.f_cost = 0;
        state.parent = nullptr;
        state.moved_tile = 0;

        // Aby uniknąć natychmiastowego cofnięcia, zapamiętujemy ostatni kierunek (–1 = brak)
        int last_dir = -1;
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> dist(0, 3);

        for (int step = 0; step < k; ++step) {
            int dir;
            int new_row, new_col;
            // Szukamy ruchu, który jest legalny i nie jest odwrotnością poprzedniego
            do {
                dir = dist(gen);
                new_row = state.empty_row + dx[dir];
                new_col = state.empty_col + dy[dir];
            } while (new_row < 0 || new_row >= size
                  || new_col < 0 || new_col >= size
                  || (last_dir != -1 && (dir ^ 1) == last_dir));
            // Wykonaj ruch:
            swap(state.board[state.empty_row][state.empty_col],
                 state.board[new_row][new_col]);
            // Zaktualizuj puste pole:
            state.empty_row = new_row;
            state.empty_col = new_col;
            // Zapamiętaj ostatni ruch (do przeciwieństwa używamy dir^1)
            last_dir = dir;
        }

        return state;
    }





    /*

    // Generowanie losowej permutacji początkowej
    State generateRandomState() {
        State state;
        state.board.resize(size, vector<int>(size));

        do {
            // Tworzymy listę liczb od 0 do total_tiles-1
            vector<int> numbers;
            for (int i = 0; i < total_tiles; i++) {
                numbers.push_back(i);
            }

            // Losowo tasujemy liczby
            random_device rd;
            mt19937 gen(rd());
            shuffle(numbers.begin(), numbers.end(), gen);

            // Wypełniamy planszę
            int idx = 0;
            for (int i = 0; i < size; i++) {
                for (int j = 0; j < size; j++) {
                    state.board[i][j] = numbers[idx];
                    if (numbers[idx] == 0) {
                        state.empty_row = i;
                        state.empty_col = j;
                    }
                    idx++;
                }
            }

            // Przesuwamy puste pole do prawego dolnego rogu
            while (state.empty_row != size - 1 || state.empty_col != size - 1) {
                if (state.empty_row < size - 1) {
                    swap(state.board[state.empty_row][state.empty_col],
                         state.board[state.empty_row + 1][state.empty_col]);
                    state.empty_row++;
                } else if (state.empty_col < size - 1) {
                    swap(state.board[state.empty_row][state.empty_col],
                         state.board[state.empty_row][state.empty_col + 1]);
                    state.empty_col++;
                }
            }

        } while (!isSolvable(state.board));  // Powtarzamy aż znajdziemy rozwiązywalną permutację

        return state;
    }
*/
    // Heurystyka 1: Liczba kafelków nie na swoim miejscu (Hamming distance)
    int hammingDistance(const vector<vector<int>>& board) {
        int distance = 0;
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                if (board[i][j] != 0 && board[i][j] != goal_state[i][j]) {
                    distance++;
                }
            }
        }
        return distance;
    }

    // Heurystyka 2: Suma odległości Manhattan każdego kafelka od docelowej pozycji
    int manhattanDistance(const vector<vector<int>>& board) {
        int distance = 0;
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                if (board[i][j] != 0) {
                    // Znajdujemy docelową pozycję tego kafelka
                    int value = board[i][j];
                    int goal_row = (value - 1) / size;
                    int goal_col = (value - 1) % size;
                    // Dodajemy odległość Manhattan
                    distance += abs(i - goal_row) + abs(j - goal_col);
                }
            }
        }
        return distance;
    }

    // Sprawdzenie czy osiągnięto stan docelowy
    bool isGoalState(const vector<vector<int>>& board) {
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                if (board[i][j] != goal_state[i][j]) {
                    return false;
                }
            }
        }
        return true;
    }

    // Konwersja planszy na string
    string boardToString(const vector<vector<int>>& board) {
        string result = "";
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                result += to_string(board[i][j]) + ",";
            }
        }
        return result;
    }

    // Generowanie możliwych następników danego stanu
    vector<State> generateSuccessors(const State& current) {
        vector<State> successors;

        // Sprawdzamy wszystkie 4 możliwe ruchy
        for (int i = 0; i < 4; i++) {
            int new_row = current.empty_row + dx[i];
            int new_col = current.empty_col + dy[i];

            // Sprawdzamy czy ruch jest w granicach planszy
            if (new_row >= 0 && new_row < size && new_col >= 0 && new_col < size) {
                State next;
                next.board = current.board;
                next.empty_row = new_row;
                next.empty_col = new_col;
                next.g_cost = current.g_cost + 1;
                next.parent = const_cast<State*>(&current);
                next.moved_tile = current.board[new_row][new_col];

                // Wykonujemy ruch (zamieniamy puste pole z kafelkiem)
                swap(next.board[current.empty_row][current.empty_col],
                     next.board[new_row][new_col]);

                successors.push_back(next);
            }
        }

        return successors;
    }

    // Algorytm A*
    pair<vector<int>, int> solve(const State& initial, int heuristic_type) {
        priority_queue<State> open_set;
        map<string, State*> all_states;
        set<string> closed_set;

        // Inicjalizacja stanu początkowego
        State* start = new State(initial);
        if (heuristic_type == 1) {
            start->h_cost = hammingDistance(start->board);
        } else {
            start->h_cost = manhattanDistance(start->board);
        }
        start->f_cost = start->g_cost + start->h_cost;

        open_set.push(*start);
        all_states[boardToString(start->board)] = start;

        int visited_states = 0;

        while (!open_set.empty()) {
            // Pobieramy stan z najniższym kosztem f
            State current = open_set.top();
            open_set.pop();

            string current_key = boardToString(current.board);

            // Jeśli już zbadaliśmy ten stan, pomijamy
            if (closed_set.find(current_key) != closed_set.end()) {
                continue;
            }

            visited_states++;

            // Sprawdzamy czy osiągnęliśmy cel
            if (isGoalState(current.board)) {
                // Odtwarzamy ścieżkę
                vector<int> solution;
                State* path = all_states[current_key];
                while (path->parent != nullptr) {
                    solution.push_back(path->moved_tile);
                    path = path->parent;
                }
                reverse(solution.begin(), solution.end());

                // Czyszczenie pamięci
                for (auto& pair : all_states) {
                    delete pair.second;
                }

                return make_pair(solution, visited_states);
            }

            // Dodajemy do zbadanych
            closed_set.insert(current_key);

            // Generujemy następniki
            vector<State> successors = generateSuccessors(current);

            for (State& next : successors) {
                string next_key = boardToString(next.board);

                // Jeśli już zbadaliśmy ten stan, pomijamy
                if (closed_set.find(next_key) != closed_set.end()) {
                    continue;
                }

                // Obliczamy heurystykę
                if (heuristic_type == 1) {
                    next.h_cost = hammingDistance(next.board);
                } else {
                    next.h_cost = manhattanDistance(next.board);
                }
                next.f_cost = next.g_cost + next.h_cost;

                // Jeśli stan już istnieje i znaleźliśmy lepszą ścieżkę
                if (all_states.find(next_key) != all_states.end()) {
                    if (next.g_cost < all_states[next_key]->g_cost) {
                        *all_states[next_key] = next;
                        all_states[next_key]->parent = all_states[current_key];
                        open_set.push(*all_states[next_key]);
                    }
                } else {
                    // Nowy stan
                    State* new_state = new State(next);
                    new_state->parent = all_states[current_key];
                    all_states[next_key] = new_state;
                    open_set.push(*new_state);
                }
            }
        }

        // Czyszczenie pamięci
        for (auto& pair : all_states) {
            delete pair.second;
        }

        return make_pair(vector<int>(), visited_states);  // Brak rozwiązania
    }

    // Wyświetlanie planszy
    void printBoard(const vector<vector<int>>& board) {
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                if (board[i][j] == 0) {
                    cout << "   ";
                } else {
                    cout.width(3);
                    cout << board[i][j];
                }
            }
            cout << endl;
        }
        cout << endl;
    }

    // Testowanie obu heurystyk
    void runTests(int num_tests) {
        double avg_visited_h1 = 0, avg_visited_h2 = 0;
        double avg_steps_h1 = 0, avg_steps_h2 = 0;

        cout << "Przeprowadzanie " << num_tests << " testów dla planszy "
             << size << "x" << size << "...\n\n";

        for (int test = 0; test < num_tests; test++) {
            State initial = generateRandomState();

            // Test z heurystyką Hamminga
            auto start_h1 = chrono::high_resolution_clock::now();
            pair<vector<int>, int> result_h1 = solve(initial, 1);
            auto end_h1 = chrono::high_resolution_clock::now();
            auto duration_h1 = chrono::duration_cast<chrono::milliseconds>(end_h1 - start_h1);

            // Test z heurystyką Manhattan
            auto start_h2 = chrono::high_resolution_clock::now();
            pair<vector<int>, int> result_h2 = solve(initial, 2);
            auto end_h2 = chrono::high_resolution_clock::now();
            auto duration_h2 = chrono::duration_cast<chrono::milliseconds>(end_h2 - start_h2);

            avg_visited_h1 += result_h1.second;
            avg_visited_h2 += result_h2.second;
            avg_steps_h1 += result_h1.first.size();
            avg_steps_h2 += result_h2.first.size();

            cout << "Test " << test + 1 << ":\n";
            cout << "  Heurystyka Hamminga: " << result_h1.second << " stanów, "
                 << result_h1.first.size() << " kroków, czas: " << duration_h1.count() << "ms\n";
            cout << "  Heurystyka Manhattan: " << result_h2.second << " stanów, "
                 << result_h2.first.size() << " kroków, czas: " << duration_h2.count() << "ms\n";
        }

        avg_visited_h1 /= num_tests;
        avg_visited_h2 /= num_tests;
        avg_steps_h1 /= num_tests;
        avg_steps_h2 /= num_tests;

        cout << "\n=== PODSUMOWANIE ===\n";
        cout << "Średnia liczba odwiedzonych stanów:\n";
        cout << "  Heurystyka Hamminga: " << avg_visited_h1 << "\n";
        cout << "  Heurystyka Manhattan: " << avg_visited_h2 << "\n";
        cout << "Średnia liczba kroków do rozwiązania:\n";
        cout << "  Heurystyka Hamminga: " << avg_steps_h1 << "\n";
        cout << "  Heurystyka Manhattan: " << avg_steps_h2 << "\n";
    }

    // Demonstracja pojedynczego rozwiązania
    void demonstrateSolution() {
        State initial = generateRandomState();

        cout << "=== STAN POCZĄTKOWY ===\n";
        printBoard(initial.board);

        cout << "Rozwiązywanie z heurystyką Manhattan...\n";
        auto start = chrono::high_resolution_clock::now();
        pair<vector<int>, int> result = solve(initial, 2);
        auto end = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);

        if (!result.first.empty()) {
            cout << "\n=== ROZWIĄZANIE ZNALEZIONE ===\n";
            cout << "Liczba odwiedzonych stanów: " << result.second << "\n";
            cout << "Liczba kroków: " << result.first.size() << "\n";
            cout << "Czas wykonania: " << duration.count() << "ms\n";
            cout << "Kolejne kafelki do przesunięcia: ";
            for (int tile : result.first) {
                cout << tile << " ";
            }
            cout << "\n";
        } else {
            cout << "Nie znaleziono rozwiązania!\n";
        }
    }
};

int main() {
    int choice, size;

    cout << "=== SOLVER UKŁADANKI PIĘTNASTKA (A*) ===\n";
    cout << "Wybierz rozmiar planszy:\n";
    cout << "1. 3x3 (8-puzzle)\n";
    cout << "2. 4x4 (15-puzzle)\n";
    cout << "Wybór: ";
    cin >> choice;

    size = (choice == 1) ? 3 : 4;
    FifteenPuzzle puzzle(size);

    cout << "\nWybierz tryb:\n";
    cout << "1. Demonstracja pojedynczego rozwiązania\n";
    cout << "2. Testy porównawcze heurystyk\n";
    cout << "Wybór: ";
    cin >> choice;

    if (choice == 1) {
        puzzle.demonstrateSolution();
    } else {
        int num_tests;
        cout << "Liczba testów: ";
        cin >> num_tests;
        puzzle.runTests(num_tests);
    }

    return 0;
}