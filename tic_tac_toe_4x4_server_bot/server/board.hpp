#pragma once
#include <vector>
#include <iostream>
#include <array>

class GameBoard {
private:
    std::array<std::array<int, 5>, 5> grid;

    static const std::array<std::array<std::array<int, 2>, 4>, 28> winPatterns;
    static const std::array<std::array<std::array<int, 2>, 3>, 48> losePatterns;

public:
    GameBoard() { reset(); }

    void reset() {
        for (int row = 0; row < 5; row++) {
            for (int col = 0; col < 5; col++) {
                grid[row][col] = 0;
            }
        }
    }

    void display() {
        std::cout << "  1 2 3 4 5\n";
        for (int row = 0; row < 5; row++) {
            std::cout << row + 1;
            for (int col = 0; col < 5; col++) {
                char symbol = '-';
                if (grid[row][col] == 1) symbol = 'X';
                else if (grid[row][col] == 2) symbol = 'O';
                std::cout << " " << symbol;
            }
            std::cout << "\n";
        }
        std::cout << "\n";
    }

    bool placeMove(int move, int player) {
        int row = (move / 10) - 1;
        int col = (move % 10) - 1;

        if (row < 0 || row > 4 || col < 0 || col > 4) return false;
        if (grid[row][col] != 0) return false;

        grid[row][col] = player;
        return true;
    }

    bool checkWin(int player) {
        for (const auto& pattern : winPatterns) {
            if (grid[pattern[0][0]][pattern[0][1]] == player &&
                grid[pattern[1][0]][pattern[1][1]] == player &&
                grid[pattern[2][0]][pattern[2][1]] == player &&
                grid[pattern[3][0]][pattern[3][1]] == player) {
                return true;
            }
        }
        return false;
    }

    bool checkLose(int player) {
        for (const auto& pattern : losePatterns) {
            if (grid[pattern[0][0]][pattern[0][1]] == player &&
                grid[pattern[1][0]][pattern[1][1]] == player &&
                grid[pattern[2][0]][pattern[2][1]] == player) {
                return true;
            }
        }
        return false;
    }

    int getCellValue(int row, int col) const {
        if (row >= 0 && row < 5 && col >= 0 && col < 5) {
            return grid[row][col];
        }
        return -1;
    }
};

const std::array<std::array<std::array<int, 2>, 4>, 28> GameBoard::winPatterns = {{
    {{{{0, 0}}, {{0, 1}}, {{0, 2}}, {{0, 3}}}},
    {{{{1, 0}}, {{1, 1}}, {{1, 2}}, {{1, 3}}}},
    /* ... remaining win patterns ... */
}};

const std::array<std::array<std::array<int, 2>, 3>, 48> GameBoard::losePatterns = {{
    {{{{0, 0}}, {{0, 1}}, {{0, 2}}}},
    {{{{0, 1}}, {{0, 2}}, {{0, 3}}}},
    /* ... remaining lose patterns ... */
}};