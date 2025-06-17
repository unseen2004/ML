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

struct State {
    vector<vector<int>> board;
    int empty_row, empty_col;
    int g_cost;
    int h_cost;
    int f_cost;
    State* parent;
    int moved_tile;

    State() : g_cost(0), h_cost(0), f_cost(0), parent(nullptr), moved_tile(0) {}

    bool operator<(const State& other) const {
        return f_cost > other.f_cost;
    }
};

class FifteenPuzzle {
private:
    int size;
    int total_tiles;
    vector<vector<int>> goal_state;
    const int dx[4] = {-1, 1, 0, 0};
    const int dy[4] = {0, 0, -1, 1};

public:
    FifteenPuzzle(int n) : size(n), total_tiles(n * n) {
        goal_state.resize(size, vector<int>(size));
        int num = 1;
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                goal_state[i][j] = num % total_tiles;
                num++;
            }
        }
    }

    bool isSolvable(const vector<vector<int>>& board) {
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

        if (size % 2 == 1) {
            return inversions % 2 == 0;
        } else {
            int empty_row = 0;
            for (int i = 0; i < size; i++) {
                for (int j = 0; j < size; j++) {
                    if (board[i][j] == 0) {
                        empty_row = size - i;
                        break;
                    }
                }
            }
            return (inversions + empty_row) % 2 == 1;
        }
    }

    State generateRandomState() {
        int k = 50;
        State state;
        state.board = goal_state;
        state.empty_row = size - 1;
        state.empty_col = size - 1;

        int last_dir = -1;
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> dist(0, 3);

        for (int step = 0; step < k; ++step) {
            int dir;
            int new_row, new_col;
            do {
                dir = dist(gen);
                new_row = state.empty_row + dx[dir];
                new_col = state.empty_col + dy[dir];
            } while (new_row < 0 || new_row >= size
                  || new_col < 0 || new_col >= size
                  || (last_dir != -1 && (dir ^ 1) == last_dir));

            swap(state.board[state.empty_row][state.empty_col],
                 state.board[new_row][new_col]);
            state.empty_row = new_row;
            state.empty_col = new_col;
            last_dir = dir;
        }

        return state;
    }

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

    int manhattanDistance(const vector<vector<int>>& board) {
        int distance = 0;
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                if (board[i][j] != 0) {
                    int value = board[i][j];
                    int goal_row = (value - 1) / size;
                    int goal_col = (value - 1) % size;
                    distance += abs(i - goal_row) + abs(j - goal_col);
                }
            }
        }
        return distance;
    }

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

    string boardToString(const vector<vector<int>>& board) {
        string result = "";
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                result += to_string(board[i][j]) + ",";
            }
        }
        return result;
    }

    vector<State> generateSuccessors(const State& current) {
        vector<State> successors;

        for (int i = 0; i < 4; i++) {
            int new_row = current.empty_row + dx[i];
            int new_col = current.empty_col + dy[i];

            if (new_row >= 0 && new_row < size && new_col >= 0 && new_col < size) {
                State next;
                next.board = current.board;
                next.empty_row = new_row;
                next.empty_col = new_col;
                next.g_cost = current.g_cost + 1;
                next.parent = const_cast<State*>(&current);
                next.moved_tile = current.board[new_row][new_col];

                swap(next.board[current.empty_row][current.empty_col],
                     next.board[new_row][new_col]);

                successors.push_back(next);
            }
        }

        return successors;
    }

    pair<vector<int>, int> solve(const State& initial, int heuristic_type) {
        priority_queue<State> open_set;
        map<string, State*> all_states;
        set<string> closed_set;

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
            State current = open_set.top();
            open_set.pop();

            string current_key = boardToString(current.board);

            if (closed_set.find(current_key) != closed_set.end()) {
                continue;
            }

            visited_states++;

            if (isGoalState(current.board)) {
                vector<int> solution;
                State* path = all_states[current_key];
                while (path->parent != nullptr) {
                    solution.push_back(path->moved_tile);
                    path = path->parent;
                }
                reverse(solution.begin(), solution.end());

                for (auto& pair : all_states) {
                    delete pair.second;
                }

                return make_pair(solution, visited_states);
            }

            closed_set.insert(current_key);

            vector<State> successors = generateSuccessors(current);

            for (State& next : successors) {
                string next_key = boardToString(next.board);

                if (closed_set.find(next_key) != closed_set.end()) {
                    continue;
                }

                if (heuristic_type == 1) {
                    next.h_cost = hammingDistance(next.board);
                } else {
                    next.h_cost = manhattanDistance(next.board);
                }
                next.f_cost = next.g_cost + next.h_cost;

                if (all_states.find(next_key) != all_states.end()) {
                    if (next.g_cost < all_states[next_key]->g_cost) {
                        *all_states[next_key] = next;
                        all_states[next_key]->parent = all_states[current_key];
                        open_set.push(*all_states[next_key]);
                    }
                } else {
                    State* new_state = new State(next);
                    new_state->parent = all_states[current_key];
                    all_states[next_key] = new_state;
                    open_set.push(*new_state);
                }
            }
        }

        for (auto& pair : all_states) {
            delete pair.second;
        }

        return make_pair(vector<int>(), visited_states);
    }

    void printBoard(const vector<vector<int>>& board) {
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                if (board[i][j] == 0) {
                    cout << "   ";
                } else {
                    cout << " " << board[i][j];
                    if (board[i][j] < 10) cout << " ";
                }
            }
            cout << "\n";
        }
        cout << "\n";
    }

    void demonstrateSolution() {
        State initial = generateRandomState();

        cout << "Initial state:\n";
        printBoard(initial.board);

        if (!isSolvable(initial.board)) {
            cout << "This puzzle is not solvable!\n";
            return;
        }

        cout << "Select heuristic:\n";
        cout << "1. Hamming distance\n";
        cout << "2. Manhattan distance\n";
        cout << "Choice: ";
        int heuristic_choice;
        cin >> heuristic_choice;

        auto start_time = chrono::high_resolution_clock::now();
        auto result = solve(initial, heuristic_choice);
        auto end_time = chrono::high_resolution_clock::now();

        auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time).count();

        cout << "\nSolution found!\n";
        cout << "Number of moves: " << result.first.size() << "\n";
        cout << "States visited: " << result.second << "\n";
        cout << "Time taken: " << duration << " ms\n";

        if (!result.first.empty()) {
            cout << "Moves (tiles to slide): ";
            for (int tile : result.first) {
                cout << tile << " ";
            }
            cout << "\n";
        } else {
            cout << "No solution found!\n";
        }
    }

    void runTests(int num_tests) {
        int hamming_total_moves = 0;
        int manhattan_total_moves = 0;
        int hamming_total_states = 0;
        int manhattan_total_states = 0;
        long long hamming_total_time = 0;
        long long manhattan_total_time = 0;

        for (int test = 1; test <= num_tests; test++) {
            cout << "Running test " << test << "/" << num_tests << "...\n";

            State initial = generateRandomState();
            while (!isSolvable(initial.board)) {
                initial = generateRandomState();
            }

            // Hamming heuristic
            auto start_time = chrono::high_resolution_clock::now();
            auto hamming_result = solve(initial, 1);
            auto end_time = chrono::high_resolution_clock::now();
            auto hamming_duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time).count();

            // Manhattan heuristic
            start_time = chrono::high_resolution_clock::now();
            auto manhattan_result = solve(initial, 2);
            end_time = chrono::high_resolution_clock::now();
            auto manhattan_duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time).count();

            hamming_total_moves += hamming_result.first.size();
            manhattan_total_moves += manhattan_result.first.size();
            hamming_total_states += hamming_result.second;
            manhattan_total_states += manhattan_result.second;
            hamming_total_time += hamming_duration;
            manhattan_total_time += manhattan_duration;
        }

        cout << "\n=== TEST RESULTS ===\n";
        cout << "Hamming Distance Heuristic:\n";
        cout << "  Average moves: " << (double)hamming_total_moves / num_tests << "\n";
        cout << "  Average states visited: " << (double)hamming_total_states / num_tests << "\n";
        cout << "  Average time: " << (double)hamming_total_time / num_tests << " ms\n";

        cout << "\nManhattan Distance Heuristic:\n";
        cout << "  Average moves: " << (double)manhattan_total_moves / num_tests << "\n";
        cout << "  Average states visited: " << (double)manhattan_total_states / num_tests << "\n";
        cout << "  Average time: " << (double)manhattan_total_time / num_tests << " ms\n";
    }
};

int main() {
    int choice, size;

    cout << "=== FIFTEEN PUZZLE SOLVER (A*) ===\n";
    cout << "Select board size:\n";
    cout << "1. 3x3 (8-puzzle)\n";
    cout << "2. 4x4 (15-puzzle)\n";
    cout << "Choice: ";
    cin >> choice;

    size = (choice == 1) ? 3 : 4;
    FifteenPuzzle puzzle(size);

    cout << "\nSelect mode:\n";
    cout << "1. Demonstrate single solution\n";
    cout << "2. Run heuristic comparison tests\n";
    cout << "Choice: ";
    cin >> choice;

    if (choice == 1) {
        puzzle.demonstrateSolution();
    } else {
        int num_tests;
        cout << "Number of tests: ";
        cin >> num_tests;
        puzzle.runTests(num_tests);
    }

    return 0;
}