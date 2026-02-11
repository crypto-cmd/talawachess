#pragma once

#include "Board.hpp"
#include "MoveGenerator.hpp"
#include <random>
#include <utility>

namespace talawachess {

class Bot {
  private:
    core::board::Board _board;
    core::board::MoveGenerator _moveGen;

    // Piece values (Centipawns)
    static const int PieceValues[7];

    // Piece-Square Tables
    static std::array<std::array<int, 64>, 7> PieceSquareTables;

    // Infinity constant for search
    static const int INF= 1000000000;
    static const int MATE_VAL= 9000000;

  public:
    Bot();

    void setFen(const std::string& fen);
    void performMove(const std::string& moveStr);
    std::pair<core::Move, int> getBestMove(int timeLimitMs);
    const core::board::Board& getBoard() const {
        return _board;
    }

  private:
    int evaluate() const;
    int quiesce(int alpha, int beta);
    int search(int depth, int alpha, int beta);
    void orderMoves(std::vector<core::Move>& moves) const;
};

} // namespace talawachess