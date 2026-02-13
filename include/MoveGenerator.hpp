#pragma once

#include "Board.hpp"
#include <vector>
namespace talawachess::core::board {
struct MoveList {
    core::Move moves[256];
    int count= 0;

    void push_back(const core::Move& m) { moves[count++]= m; }
    void clear() { count= 0; }
    bool empty() const { return count == 0; }
    int size() const { return count; }

    // Iterators so range-based for loops still work
    core::Move* begin() { return &moves[0]; }
    core::Move* end() { return &moves[count]; }
    core::Move& operator[](int i) { return moves[i]; }
};
class MoveGenerator {
  private:
    Board& _board; // Reference to the board for move generation context
  public:
    MoveGenerator(Board& board): _board(board) {};
    void generateMoves(MoveList& moveList);

    static void generatePawnMoves(const Board& board, Piece::Piece piece, Coordinate coord, MoveList& moves);
    static void generateKnightMoves(const Board& board, Piece::Piece piece, Coordinate coord, MoveList& moves);
    static void generateSlidingMoves(const Board& board, Piece::Piece piece, Coordinate coord, MoveList& moves, const std::vector<Coordinate>& directions);
    static void generateKingMoves(const Board& board, Piece::Piece piece, Coordinate coord, MoveList& moves);
    static bool isSquareAttacked(const Board& board, Coordinate square, Piece::Color attackerColor);
    inline static const bool IsLegalPosition(const Board& board) {
        auto kingPos= board.activeColor == Piece::WHITE ? board.blackKingPos : board.whiteKingPos;
        return !isSquareAttacked(board, kingPos, board.activeColor);
    }

    inline static const std::vector<Coordinate> KNIGHT_DIRS= {{1, 2}, {2, 1}, {2, -1}, {1, -2}, {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}};
    inline static const std::vector<Coordinate> BISHOP_DIRS= {{1, 1}, {1, -1}, {-1, -1}, {-1, 1}};
    inline static const std::vector<Coordinate> ROOK_DIRS= {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
    inline static const std::vector<Coordinate> QUEEN_DIRS= {{1, 0}, {0, 1}, {-1, 0}, {0, -1}, {1, 1}, {1, -1}, {-1, -1}, {-1, 1}};
    inline static const std::vector<Coordinate> KING_DIRS= {{1, 0}, {0, 1}, {-1, 0}, {0, -1}, {1, 1}, {1, -1}, {-1, -1}, {-1, 1}};
};
} // namespace talawachess::core::board