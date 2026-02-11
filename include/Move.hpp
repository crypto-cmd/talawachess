#pragma once

#include "Coordinate.hpp"
#include "Piece.hpp"
#include <string>
namespace talawachess::core {
struct Move {
    board::Coordinate from= board::Coordinate(0, 0);
    board::Coordinate to= board::Coordinate(0, 0);
    Piece::Piece promotion= Piece::NONE;  // NONE if not a promotion
    Piece::Piece captured= Piece::NONE;   // NONE if no capture
    Piece::Piece movedPiece= Piece::NONE; // The piece that is moving (useful for undoing moves)

    inline std::string ToString() const {
        std::string s= from.toAlgebraic() + to.toAlgebraic();
        if(promotion != Piece::NONE) {
            char promoChar;
            switch(Piece::GetPieceType(promotion)) {
            case Piece::QUEEN: promoChar= 'q'; break;
            case Piece::ROOK: promoChar= 'r'; break;
            case Piece::BISHOP: promoChar= 'b'; break;
            case Piece::KNIGHT: promoChar= 'n'; break;
            default: promoChar= 'q'; // Default to queen if something goes wrong
            }
            s+= promoChar;
        }
        return s;
    }
};
} // namespace talawachess::core
