#pragma once

#include "Board.hpp"
#include <vector>
namespace talawachess::core::board {
class MoveGenerator {
  private:
    Board& _board;                  // Reference to the board for move generation context
    std::vector<Move> _pseudomoves; // Storage for generated moves to avoid reallocations
    std::vector<Move> _moves;       // Storage for legal moves to avoid reallocations
  public:
    MoveGenerator(Board& board): _board(board) {
        // Constructor can be used to initialize any necessary data structures or precompute tables if needed
        _pseudomoves.reserve(256); // Reserve space for moves to avoid reallocations during generation
        _moves.reserve(256);       // Reserve space for moves to avoid reallocations during generation
    };
    std::vector<Move>& generateMoves();
    std::vector<Move>& generateLegalMoves();

    static void generatePawnMoves(const Board& board, Piece::Piece piece, Coordinate coord, std::vector<Move>& moves);
    static void generateKnightMoves(const Board& board, Piece::Piece piece, Coordinate coord, std::vector<Move>& moves);
    static void generateSlidingMoves(const Board& board, Piece::Piece piece, Coordinate coord, std::vector<Move>& moves, const std::vector<Coordinate>& directions);
    static void generateKingMoves(const Board& board, Piece::Piece piece, Coordinate coord, std::vector<Move>& moves);
    static bool isSquareAttacked(const Board& board, Coordinate square, Piece::Color attackerColor);

    inline static const std::map<Piece::PieceType, std::vector<Coordinate>> pieceDirections= {
        {Piece::KNIGHT, {{1, 2}, {2, 1}, {2, -1}, {1, -2}, {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}}},
        {Piece::BISHOP, {{1, 1}, {1, -1}, {-1, -1}, {-1, 1}}},
        {Piece::ROOK, {{1, 0}, {0, 1}, {-1, 0}, {0, -1}}},
        {Piece::QUEEN, {{1, 0}, {0, 1}, {-1, 0}, {0, -1}, {1, 1}, {1, -1}, {-1, -1}, {-1, 1}}},
        {Piece::KING, {{1, 0}, {0, 1}, {-1, 0}, {0, -1}, {1, 1}, {1, -1}, {-1, -1}, {-1, 1}}}};
};
} // namespace talawachess::core::board