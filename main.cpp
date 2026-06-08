/*
 * ============================================================
 *                     MINESWEEPER
 * ============================================================
 * Console implementation of the classic puzzle game.
 *
 * Author: Nadeem Sassin
 * Language: C++
 * IDE: Visual Studio Code
 * Notes: Designed to be understandable and fun to read.
 *
 * Features:
 * - First-click safe generation
 * - Recursive blank-cell expansion
 * - Persistent high scores
 * - Multiple difficulties
 * - Flagging system
 * - Win/loss detection
 *
 * This project was created to practice:
 * - Object-Oriented Programming
 * - File I/O
 * - STL Containers
 * - Random Number Generation
 * - Input Validation
 * ============================================================
 *
 * Build:
 *   g++ main.cpp -o minesweeper
 * Compiler:
 *  Visual Studio Code; C++
 *
 * Run:
 *   ./main
 *
 *
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <random>
#include <chrono>
#include <limits>

// ============================================================
//  CELL
//  Represents one square on the board
// ============================================================

struct Cell {
    bool isMine = false;
    bool isRevealed = false;
    bool isFlagged = false;
    int  adjacentMines = 0;

    void reset() {
        isMine = isRevealed = isFlagged = false;
        adjacentMines = 0;
    }
};

// ============================================================
//  BOARD
//  Holds the grid and all core game logic
// ============================================================

// Used for efficient win detection without scanning the entire board every turn.
class Board {
public:
    int rows, cols, totalMines;
    int flagsPlaced = 0;
    // Total number of non-mine cells revealed.
    int revealedCount = 0;
    bool minesPlaced = false;

    std::vector<std::vector<Cell>> grid;

    Board(int r, int c, int m)
        : rows(r), cols(c), totalMines(m),
          grid(r, std::vector<Cell>(c)) {}

    // ── Mine placement ────────────────────────────────────────
    // Called on first reveal so the first click is always safe.
    // Excludes the clicked cell and its 8 neighbors from mine pool.
    void placeMines(int firstRow, int firstCol) {
        std::vector<std::pair<int,int>> candidates;
        for (int r = 0; r < rows; r++)
            for (int c = 0; c < cols; c++)
                if (std::abs(r - firstRow) > 1 || std::abs(c - firstCol) > 1)
                    candidates.push_back({r, c});

        auto seed = std::chrono::steady_clock::now().time_since_epoch().count();
        std::mt19937 rng(static_cast<unsigned>(seed));
        std::shuffle(candidates.begin(), candidates.end(), rng);

        for (int i = 0; i < totalMines && i < (int)candidates.size(); i++)
            grid[candidates[i].first][candidates[i].second].isMine = true;

        minesPlaced = true;
        calcAdjacency();
    }

    // ── Adjacency calculation ─────────────────────────────────
    // For every non-mine cell, count mines in all 8 neighbors
    void calcAdjacency() {
        const int dr[] = {-1,-1,-1, 0, 0, 1, 1, 1};
        const int dc[] = {-1, 0, 1,-1, 1,-1, 0, 1};

        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                if (grid[r][c].isMine) continue;
                int count = 0;
                for (int d = 0; d < 8; d++) {
                    int nr = r + dr[d], nc = c + dc[d];
                    if (inBounds(nr, nc) && grid[nr][nc].isMine)
                        count++;
                }
                grid[r][c].adjacentMines = count;
            }
        }
    }
    // ── Reveal ────────────────────────────────────────────────
    // Returns false if a mine was hit (game over)
    bool reveal(int row, int col) {
        Cell& cell = grid[row][col];
        if (cell.isFlagged || cell.isRevealed) 
            return true;

        // Trigger mine placement on very first reveal
        if (!minesPlaced) placeMines(row, col);

        if (cell.isMine) {
            cell.isRevealed = true;
            return false; // explosion! game over
        }

        // If no adjacent mines, flood-fill to auto-clear the empty region
        if (cell.adjacentMines == 0)
            floodReveal(row, col);
        else {
            cell.isRevealed = true;
            revealedCount++;
        }

        return true;
    }

    // ── Flood fill ────────────────────────────────────────────
    // DFS stack — reveals all connected empty cells and their numbered borders
    void floodReveal(int startRow, int startCol) {
        const int dr[] = {-1,-1,-1, 0, 0, 1, 1, 1};
        const int dc[] = {-1, 0, 1,-1, 1,-1, 0, 1};

        std::vector<std::pair<int,int>> stack = {{startRow, startCol}};

        while (!stack.empty()) {
            auto [r, c] = stack.back();
            stack.pop_back();

            if (!inBounds(r, c)) continue;
            Cell& cell = grid[r][c];
            if (cell.isRevealed || cell.isMine || cell.isFlagged) continue;

            cell.isRevealed = true;
            revealedCount++;

            // Keep spreading only from cells with no adjacent mines
            if (cell.adjacentMines == 0)
                for (int d = 0; d < 8; d++)
                    stack.push_back({r + dr[d], c + dc[d]});
        }
    }

    // ── Flag toggle ───────────────────────────────────────────
    void toggleFlag(int row, int col) {
        Cell& cell = grid[row][col];
        if (cell.isRevealed) return;
        cell.isFlagged ? flagsPlaced-- : flagsPlaced++;
        cell.isFlagged = !cell.isFlagged;
    }

    // ── Win check ─────────────────────────────────────────────
    // Win when every non-mine cell is revealed
    bool checkWin() const {
        return revealedCount == (rows * cols - totalMines);
    }

    // ── Reveal all mines ──────────────────────────────────────
    // Called on game over to show the full board
    void revealAllMines() {
        for (int r = 0; r < rows; r++)
            for (int c = 0; c < cols; c++)
                if (grid[r][c].isMine)
                    grid[r][c].isRevealed = true;
    }

    // ── Reset ─────────────────────────────────────────────────
    void reset() {
        for (int r = 0; r < rows; r++)
            for (int c = 0; c < cols; c++)
                grid[r][c].reset();
        flagsPlaced = revealedCount = 0;
        minesPlaced = false;
    }

    bool inBounds(int r, int c) const {
        return r >= 0 && r < rows && c >= 0 && c < cols;
    }
};

// ============================================================
//  DISPLAY
//  All terminal rendering — kept separate from game logic
// ============================================================

class Display {
public:

    // ── Cell character ────────────────────────────────────────
    static char cellChar(const Cell& cell, bool showAll) {
        if (cell.isFlagged && !showAll)   return 'F';
        if (!cell.isRevealed && !showAll) return '.';  // hidden
        if (cell.isMine)                  return '*';  // mine
        if (cell.adjacentMines == 0)      return ' ';  // blank
        return '0' + cell.adjacentMines;               // 1-8
    }

    // ── Board ─────────────────────────────────────────────────
    static void drawBoard(const Board& board, bool showAll = false) {
        // Column header
        std::cout << "    ";
        for (int c = 0; c < board.cols; c++)
            std::cout << std::setw(2) << c;
        std::cout << "\n    ";
        for (int c = 0; c < board.cols; c++) std::cout << "--";
        std::cout << "\n";

        for (int r = 0; r < board.rows; r++) {
            std::cout << std::setw(3) << r << "|";
            for (int c = 0; c < board.cols; c++)
                std::cout << " " << cellChar(board.grid[r][c], showAll);
            std::cout << "\n";
        }
    }

    // ── Status bar ────────────────────────────────────────────
    static void drawStatus(int minesLeft, int elapsed) {
        int m = elapsed / 60, s = elapsed % 60;
        std::cout << "\n  Mines left: " << std::setw(3) << minesLeft
                  << "   Time: "
                  << std::setfill('0') << std::setw(2) << m << ":"
                  << std::setw(2) << s << std::setfill(' ') << "\n\n";
    }

    // ── Main menu ─────────────────────────────────────────────
    static void drawMenu() {
        std::cout << "\n============================\n";
        std::cout << "       MINESWEEPER\n";
        std::cout << "============================\n";
        std::cout << "  1. Easy    ( 9x9,  10 mines)\n";
        std::cout << "  2. Medium  (16x16, 40 mines)\n";
        std::cout << "  3. Hard    (16x30, 99 mines)\n";
        std::cout << "  4. High Scores\n";
        std::cout << "  5. Quit\n";
        std::cout << "============================\n";
        std::cout << "Select: ";
    }

    // ── Game over / win ───────────────────────────────────────
    static void drawResult(bool won, int elapsed) {
        int m = elapsed / 60, s = elapsed % 60;
        std::cout << "\n============================\n";
        if (won) {
            std::cout << "       YOU WIN!\n";
            std::cout << "  Time: "
                      << std::setfill('0') << std::setw(2) << m << ":"
                      << std::setw(2) << s << std::setfill(' ') << "\n";
        } else {
            std::cout << "    BOOM! Game over.\n";
        }
        std::cout << "============================\n\n";
    }

    // ── High scores ───────────────────────────────────────────
    static void drawScores(int easy, int med, int hard) {
        auto fmt = [](int t) -> std::string {
            if (t == 0) return "--:--";
            char buf[16];
            std::snprintf(buf, sizeof(buf), "%02d:%02d", t / 60, t % 60);
            return std::string(buf);
        };
        std::cout << "\n============================\n";
        std::cout << "       HIGH SCORES\n";
        std::cout << "============================\n";
        std::cout << "  Easy:   " << fmt(easy) << "\n";
        std::cout << "  Medium: " << fmt(med)  << "\n";
        std::cout << "  Hard:   " << fmt(hard) << "\n";
        std::cout << "============================\n\n";
    }
};

// ============================================================
//  GAME
//  Game loop, input handling, timer, score persistence
// ============================================================

enum class Difficulty { EASY, MEDIUM, HARD };

class Game {
public:
    // Minesweeper dimensions
    static const int EASY_R = 9,  EASY_C = 9,  EASY_M = 10;
    static const int MED_R  = 16, MED_C  = 16, MED_M  = 40;
    static const int HARD_R = 16, HARD_C = 30, HARD_M = 99;

    int bestTimes[3] = {0, 0, 0}; // indexed by Difficulty enum

    Game() { loadScores("scores.txt"); }

    // ── Main menu loop ────────────────────────────────────────
    void run() {
        while (true) {
            Display::drawMenu();
            int choice;
            std::cin >> choice;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            // Choices
            if      (choice == 1) playGame(Difficulty::EASY);
            else if (choice == 2) playGame(Difficulty::MEDIUM);
            else if (choice == 3) playGame(Difficulty::HARD);
            else if (choice == 4) Display::drawScores(bestTimes[0], bestTimes[1], bestTimes[2]);
            else if (choice == 5) { std::cout << "Game Quit.\n"; return; }
            else                  std::cout << "Invalid choice.\n";
        }
    }

private:
    std::chrono::steady_clock::time_point startTime;
    bool timerStarted = false;

    // ── Single game session ───────────────────────────────────
    void playGame(Difficulty diff) {
        // Pick dimensions
        int rows, cols, mines;
        switch (diff) {
            case Difficulty::EASY:   rows = EASY_R; cols = EASY_C; mines = EASY_M; break;
            case Difficulty::MEDIUM: rows = MED_R;  cols = MED_C;  mines = MED_M;  break;
            default:                 rows = HARD_R; cols = HARD_C; mines = HARD_M; break;
        }
        Board board(rows, cols, mines);
        timerStarted = false;
        bool playing = true;

        std::cout << "\nControls: \"row col\" to reveal  |  \"f row col\" to flag\n\n";

        while (playing) {
            Display::drawBoard(board);
            Display::drawStatus(
                board.totalMines - board.flagsPlaced,
                timerStarted ? elapsed() : 0
            );

            std::cout << "Input: ";
            std::string line;
            std::getline(std::cin, line);

            bool isFlag;
            int  row, col;
            if (!parseInput(line, isFlag, row, col)) {
                std::cout << "Bad input. Examples: \"4 7\" or \"f 4 7\"\n";
                continue;
            }
            if (!board.inBounds(row, col)) {
                std::cout << "Out of bounds (row 0-" << rows-1
                          << ", col 0-" << cols-1 << ")\n";
                continue;
            }

            if (isFlag) {
                board.toggleFlag(row, col);
            } else {
                // Start timer on first reveal
                if (!timerStarted) {
                    startTime    = std::chrono::steady_clock::now();
                    timerStarted = true;
                }

                if (!board.reveal(row, col)) {
                    // Hit a mine
                    board.revealAllMines();
                    Display::drawBoard(board, true);
                    Display::drawResult(false, elapsed());
                    playing = false;
                } else if (board.checkWin()) {
                    int t = elapsed();
                    Display::drawBoard(board);
                    Display::drawResult(true, t);
                    updateBest(diff, t);
                    saveScores("scores.txt");
                    playing = false;
                }
            }
        }

        // Offer replay on same difficulty
        std::cout << "Play again? (y/n): ";
        char again;
        std::cin >> again;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        if (again == 'y' || again == 'Y') playGame(diff);
    }

    // ── Input parser ──────────────────────────────────────────
    // Accepts "r c" (reveal) or "f r c" (flag)
    // Supports:
    // 4 7
    // Reveals row 4 column 7
    // Could also do:
    // f 4 7
    // Toggles a flag at row 4 column 7
    bool parseInput(const std::string& input, bool& isFlag, int& row, int& col) {
        std::istringstream ss(input);
        std::string token;
        ss >> token;

        if (token == "f" || token == "F") {
            isFlag = true;
            return (bool)(ss >> row >> col);
        }

        isFlag = false;
        try { row = std::stoi(token); }
        catch (...) { return false; }
        return (bool)(ss >> col);
    }

    // ── Timer ─────────────────────────────────────────────────
    int elapsed() const {
        return static_cast<int>(
            std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - startTime
            ).count()
        );
    }

    // ── Score tracking ────────────────────────────────────────
    void updateBest(Difficulty diff, int seconds) {
        int i = static_cast<int>(diff);
        if (bestTimes[i] == 0 || seconds < bestTimes[i])
            bestTimes[i] = seconds;
    }

    void saveScores(const std::string& file) {
        std::ofstream f(file);
        if (f) f << bestTimes[0] << " " << bestTimes[1] << " " << bestTimes[2];
    }

    void loadScores(const std::string& file) {
        std::ifstream f(file);
        if (f) f >> bestTimes[0] >> bestTimes[1] >> bestTimes[2];
    }
};

// ============================================================
//  MAIN
// ============================================================

int main() {
    // Create the game and hand control over to the main loop.
    Game game;
    game.run();
    return 0;
}