#pragma once

#include "Move.hpp"
#include "Piece.hpp"
#include <array>
#include <iostream>
#include <string>
#include <vector>

namespace talawachess::core::board {

class GameState {
  public:
    std::vector<Piece::Piece> squares;
    uint8_t castlingRights;
    int enPassantIndex;
    int halfMoveClock;
};
class Board {

  public:
    static constexpr const char* STARTING_POS= "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    std::vector<Piece::Piece> squares;
    std::vector<std::pair<Piece::Piece, Coordinate>> pieces; // List of pieces and their coordinates for quick access
    std::vector<GameState> game_history;
    Piece::Color activeColor= Piece::WHITE;

    static constexpr uint8_t CASTLE_WK = 1; // White King Side
    static constexpr uint8_t CASTLE_WQ = 2; // White Queen Side
    static constexpr uint8_t CASTLE_BK = 4; // Black King Side
    static constexpr uint8_t CASTLE_BQ = 8; // Black Queen Side

    uint8_t castlingRights = 0; 
    int enPassantIndex = -1; // -1 if no en passant square
    int halfMoveClock = 0;
    int fullMoveNumber = 1;

    Board();
    void setFen(const std::string& fen);
    void print() const;

    void makeMove(const Move& move);
    void undoMove();
};
} // namespace talawachess::core::board
