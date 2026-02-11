#pragma once
#include <array>
#include <cstdint>
#include <map>
namespace talawachess::core {
// Piece representation: [color (1 bit)][type (4 bits)]

namespace Piece {
using Piece= uint8_t;

enum PieceType {
    NONE= 0,
    PAWN= 0b001,
    KNIGHT= 0b010,
    BISHOP= 0b011,
    ROOK= 0b100,
    QUEEN= 0b101,
    KING= 0b110
};

enum Color {
    WHITE= 0b01000,
    BLACK= 0b10000,
};

static constexpr int PieceTypeMask= 0b111;
static constexpr int ColorMask= 0b11000;

static inline bool IsColor(Piece piece, Color color) {

    return (piece & ColorMask) == color;
}

static inline bool IsType(Piece piece, PieceType type) {
    return (piece & PieceTypeMask) == type;
}

static Color GetColor(Piece piece) {
    return IsColor(piece, BLACK) ? BLACK : WHITE;
}

static PieceType GetPieceType(Piece piece) {
    return static_cast<PieceType>(piece & PieceTypeMask);
}
struct Entry {
    Piece p;
    char c;
};

static constexpr char GetSymbol(Piece piece) {
    switch(piece) {
    case NONE:
        return '.';
    case WHITE | PAWN:
        return 'P';
    case WHITE | KNIGHT:
        return 'N';
    case WHITE | BISHOP:
        return 'B';
    case WHITE | ROOK:
        return 'R';
    case WHITE | QUEEN:
        return 'Q';
    case WHITE | KING:
        return 'K';
    case BLACK | PAWN:
        return 'p';
    case BLACK | KNIGHT:
        return 'n';
    case BLACK | BISHOP:
        return 'b';
    case BLACK | ROOK:
        return 'r';
    case BLACK | QUEEN:
        return 'q';
    case BLACK | KING:
        return 'k';
    default:
        return '?';
    }
}
static constexpr Piece FromSymbol(char symbol) {
    switch(symbol) {
    case '.': return NONE;
    case 'P': return WHITE | PAWN;
    case 'N': return WHITE | KNIGHT;
    case 'B': return WHITE | BISHOP;
    case 'R': return WHITE | ROOK;
    case 'Q': return WHITE | QUEEN;
    case 'K': return WHITE | KING;
    case 'p': return BLACK | PAWN;
    case 'n': return BLACK | KNIGHT;
    case 'b': return BLACK | BISHOP;
    case 'r': return BLACK | ROOK;
    case 'q': return BLACK | QUEEN;
    case 'k': return BLACK | KING;
    default: return NONE;
    }
}
}; // namespace Piece

} // namespace talawachess::core
