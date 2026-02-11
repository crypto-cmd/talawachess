#pragma once

#include "Coordinate.hpp"
#include "Move.hpp"
#include "Piece.hpp"
#include <iostream>
#include <string>
#include <utility> // for std::pair
#include <vector>

namespace talawachess::core::board {

// Struct to save the state of the game before a move is made
struct GameState {
    std::vector<Piece::Piece> squares;
    std::vector<std::pair<Piece::Piece, Coordinate>> pieces; // Saved piece list
    uint8_t castlingRights;
    int enPassantIndex;
    int halfMoveClock;
    uint64_t zobristHash; // Saved hash

    Coordinate whiteKingPos; // Saved white king position
    Coordinate blackKingPos; // Saved black king position
};

class Board {
  public:
    // Standard starting position FEN
    static constexpr const char* STARTING_POS= "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    // The board representation (Mailbox)
    std::vector<Piece::Piece> squares;

    // Incremental piece list: Tracks where every piece is to avoid O(64) loops
    std::vector<std::pair<Piece::Piece, Coordinate>> pieces;

    // History stack for undoing moves
    std::vector<GameState> game_history;

    // Game State Variables
    Piece::Color activeColor= Piece::WHITE;

    Coordinate whiteKingPos= Coordinate(4, 0);
    Coordinate blackKingPos= Coordinate(4, 7);
    // Castling Bitmasks
    static constexpr uint8_t CASTLE_WK= 1; // White King-side
    static constexpr uint8_t CASTLE_WQ= 2; // White Queen-side
    static constexpr uint8_t CASTLE_BK= 4; // Black King-side
    static constexpr uint8_t CASTLE_BQ= 8; // Black Queen-side

    uint8_t castlingRights= 0;
    int enPassantIndex= -1; // -1 if no en passant available
    int halfMoveClock= 0;   // 50-move rule counter
    int fullMoveNumber= 1;

    // Zobrist Hash for Transposition Table
    uint64_t zobristHash= 0;

    // Constructors and Core Methods
    Board();
    void setFen(const std::string& fen);
    void print() const;

    // Move Execution
    void makeMove(const Move& move);
    void undoMove();

    // Zobrist Helpers
    // Initializes the random keys (called once by constructor)
    static void initZobrist();
    // Calculates the hash from scratch (slow, used for verification/initialization)
    uint64_t calculateHash() const;

    bool isKingInCheck( Piece::Color kingColor) const;
};

} // namespace talawachess::core::board