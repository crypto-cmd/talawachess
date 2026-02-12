#pragma once

#include "Board.hpp"
#include "MoveGenerator.hpp"
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
    core::Move* bestMove;
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

    // Piece values (Centipawns)
    static const int PieceValues[7];

    // Piece-Square Tables
    static std::array<std::array<int, 64>, 7> PieceSquareTables;

    // Infinity constant for search
    static const int INF= 1000000000;
    static const int MATE_VAL= 9000000;

    int positionsEvaluated= 0; // For performance metrics
    int checkMatesFound= 0;    // For performance metrics

  public:
    Bot();

    void setFen(const std::string& fen);
    void performMove(const std::string& moveStr);
    std::pair<core::Move, int> getBestMove(int timeLimitMs);
    const core::board::Board& getBoard() const {
        return _board;
    }

  private:
    int evaluate();
    int quiesce(int alpha, int beta, int ply);
    int search(int depth, int ply, int alpha, int beta);
    void orderMoves(std::vector<core::Move>& moves, const core::Move* ttMove) const;
};

} // namespace talawachess