#pragma once

#include "Board.hpp"
namespace talawachess::core::board {
class MoveGenerator {
  public:
    static std::vector<Move> generateMoves(const Board& board);
    static std::vector<Move> generateLegalMoves(Board& board);

    static void generatePawnMoves(const Board& board, Piece::Piece piece, Coordinate coord, std::vector<Move>& moves);
    static void generateKnightMoves(const Board& board, Piece::Piece piece, Coordinate coord, std::vector<Move>& moves);
    static void generateSlidingMoves(const Board& board, Piece::Piece piece, Coordinate coord, std::vector<Move>& moves, const std::vector<Coordinate>& directions);
    static void generateKingMoves(const Board& board, Piece::Piece piece, Coordinate coord, std::vector<Move>& moves);

    inline static const std::map<Piece::PieceType, std::vector<Coordinate>> pieceDirections= {
        {Piece::KNIGHT, {{1, 2}, {2, 1}, {2, -1}, {1, -2}, {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}}},
        {Piece::BISHOP, {{1, 1}, {1, -1}, {-1, -1}, {-1, 1}}},
        {Piece::ROOK, {{1, 0}, {0, 1}, {-1, 0}, {0, -1}}},
        {Piece::QUEEN, {{1, 0}, {0, 1}, {-1, 0}, {0, -1}, {1, 1}, {1, -1}, {-1, -1}, {-1, 1}}},
        {Piece::KING, {{1, 0}, {0, 1}, {-1, 0}, {0, -1}, {1, 1}, {1, -1}, {-1, -1}, {-1, 1}}}};
};
} // namespace talawachess::core::board