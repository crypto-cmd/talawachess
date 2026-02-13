#pragma once

#include "Board.hpp"
#include "MoveGenerator.hpp"
#include <chrono>
#include <functional>
#include <random>
#include <utility>

namespace talawachess {

// Transposition Table Flags
enum TTFlag : uint8_t {
    TT_EXACT, // We know the exact score
    TT_ALPHA, // Upper Bound (We know the score is at most this)
    TT_BETA   // Lower Bound (We know the score is at least this)
};

struct TTEntry {
    uint64_t zobristHash;
    core::Move bestMove;
    int score;
    int depth;
    TTFlag flag;
};

class Bot {
  private:
    core::board::Board _board;
    core::board::MoveGenerator _moveGen;

    std::vector<TTEntry> _tt;
    void resizeTT(size_t sizeInMB);
    void clearTT();

    // Killer Moves: 2 killers per ply, max 64 plies
    static const int MAX_PLY= 64;
    core::Move _killers[MAX_PLY][2];
    void clearKillers();
    void updateKillers(const core::Move& move, int ply);

    // Infinity constant for search
    static const int INF= 1000000000;
    static const int MATE_VAL= 9000000;

    int positionsEvaluated= 0; // For performance metrics
    int checkMatesFound= 0;    // For performance metrics

    // Time Management
    std::chrono::time_point<std::chrono::steady_clock> _searchStartTime;
    int _timeLimitMs= 0;
    bool _stopSearch= false;

    // Optional callback to check for stop command from UCI
    std::function<bool()> _inputChecker= nullptr;

    void checkTime() {
        if(_stopSearch) return; // Already signaled to stop
        // Check for external stop command via input polling
        if(_inputChecker && _inputChecker()) {
            _stopSearch= true;
            return;
        }
        if(_timeLimitMs <= 0) return; // No time limit (infinite or depth-only search)
        int elapsedMs= getElapsedTimeMs();
        if(elapsedMs >= _timeLimitMs) {
            _stopSearch= true;
        }
    }

    int getElapsedTimeMs() const {
        auto now= std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - _searchStartTime).count();
    }

    void startTimer(int timeLimitMs= 0) {
        _searchStartTime= std::chrono::steady_clock::now();
        _timeLimitMs= timeLimitMs;
        _stopSearch= false;
    }

  public:
    Bot();

    void setFen(const std::string& fen);
    void performMove(const std::string& moveStr);
    std::pair<core::Move, int> getBestMove(int timeLimitMs, int maxDepth= 0);
    void stopSearch() { _stopSearch= true; }
    void setInputChecker(std::function<bool()> checker) { _inputChecker= checker; }
    const core::board::Board& getBoard() const {
        return _board;
    }

  private:
    int quiesce(int alpha, int beta, int ply);
    int search(int depth, int ply, int alpha, int beta);
    void orderMoves(core::board::MoveList& moves, const core::Move* ttMove, int ply) const;
    std::string extractPV(const core::Move& bestMove, int depth);
};

} // namespace talawachess