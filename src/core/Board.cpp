#include "Board.hpp"
#include "Coordinate.hpp"
#include <algorithm>
#include <iostream>
#include <random>
#include <sstream>

namespace talawachess::core::board {

// -----------------------------------------------------------------------------
// GLOBAL STATIC ZOBRIST KEYS
// -----------------------------------------------------------------------------
static uint64_t zPiece[12][64];
static uint64_t zEnPassant[65];
static uint64_t zCastling[16];
static uint64_t zSide;

static int getPieceIndex(Piece::Piece p) {
    if(p == Piece::NONE) return -1;
    int type= Piece::GetPieceType(p);
    int idx= 0;
    switch(type) {
    case Piece::PAWN: idx= 0; break;
    case Piece::KNIGHT: idx= 1; break;
    case Piece::BISHOP: idx= 2; break;
    case Piece::ROOK: idx= 3; break;
    case Piece::QUEEN: idx= 4; break;
    case Piece::KING: idx= 5; break;
    default: return -1;
    }
    if(Piece::IsColor(p, Piece::BLACK)) idx+= 6;
    return idx;
}

// -----------------------------------------------------------------------------
// BOARD IMPLEMENTATION
// -----------------------------------------------------------------------------

Board::Board() {
    static bool initialized= false;
    if(!initialized) {
        initZobrist();
        initialized= true;
    }
    game_history.reserve(512);
    setFen(STARTING_POS);
}

void Board::initZobrist() {
    std::mt19937_64 gen(123456789);
    std::uniform_int_distribution<uint64_t> dis;

    for(int p= 0; p < 12; ++p) {
        for(int s= 0; s < 64; ++s) {
            zPiece[p][s]= dis(gen);
        }
    }
    for(int i= 0; i < 65; ++i) zEnPassant[i]= dis(gen);
    for(int i= 0; i < 16; ++i) zCastling[i]= dis(gen);
    zSide= dis(gen);
}

uint64_t Board::calculateHash() const {
    uint64_t hash= 0;
    for(int i= 0; i < 64; ++i) {
        if(squares[i] != Piece::NONE) {
            hash^= zPiece[getPieceIndex(squares[i])][i];
        }
    }
    hash^= zCastling[castlingRights];
    if(enPassantIndex != -1) hash^= zEnPassant[enPassantIndex];
    else hash^= zEnPassant[64];

    if(activeColor == Piece::BLACK) hash^= zSide;
    return hash;
}

void Board::setFen(const std::string& fen) {
    std::fill(std::begin(squares), std::end(squares), Piece::NONE);
    game_history.clear();

    std::stringstream ss(fen);
    std::string boardPart, activeColorStr, castlingPart, enPassantPart;
    std::string halfMoveStr, fullMoveStr;

    ss >> boardPart >> activeColorStr >> castlingPart >> enPassantPart >> halfMoveStr >> fullMoveStr;

    Coordinate coord= Coordinate::FromAlgebraic("a8");
    for(char c: boardPart) {
        if(c == '/') {
            coord= Coordinate(0, coord.rank - 1);
            continue;
        }
        if(std::isdigit(c)) {
            coord= Coordinate(coord.file + (c - '0'), coord.rank);
        } else {
            int squareIndex= coord.ToIndex();
            Piece::Piece piece= Piece::FromSymbol(c);
            squares[squareIndex]= piece;
            coord= Coordinate(coord.file + 1, coord.rank);
        }
    }

    this->activeColor= (activeColorStr == "w") ? Piece::WHITE : Piece::BLACK;

    this->castlingRights= 0;
    if(castlingPart != "-") {
        for(char c: castlingPart) {
            if(c == 'K') this->castlingRights|= CASTLE_WK;
            else if(c == 'Q') this->castlingRights|= CASTLE_WQ;
            else if(c == 'k') this->castlingRights|= CASTLE_BK;
            else if(c == 'q') this->castlingRights|= CASTLE_BQ;
        }
    }

    if(enPassantPart != "-") {
        this->enPassantIndex= Coordinate::FromAlgebraic(enPassantPart.c_str()).ToIndex();
    } else {
        this->enPassantIndex= -1;
    }

    try {
        this->halfMoveClock= std::stoi(halfMoveStr);
        this->fullMoveNumber= std::stoi(fullMoveStr);
    } catch(...) {
        this->halfMoveClock= 0;
        this->fullMoveNumber= 1;
    }

    // Find Kings' positions
    for(int i= 0; i < 64; ++i) {
        Piece::Piece piece= squares[i];
        if(Piece::IsType(piece, Piece::KING)) {
            Coordinate pos= Coordinate(i);
            if(Piece::IsColor(piece, Piece::WHITE)) {
                whiteKingPos= pos;
            } else {
                blackKingPos= pos;
            }
        }
    }

    this->zobristHash= calculateHash();
}

void Board::makeMove(const Move& move) {
    // 1. Save History
    GameState state;
    state.move= move;
    state.capturedPiece= move.captured;
    state.castlingRights= castlingRights;
    state.enPassantIndex= enPassantIndex;
    state.halfMoveClock= halfMoveClock;
    state.zobristHash= zobristHash;
    state.whiteKingPos= whiteKingPos;
    state.blackKingPos= blackKingPos;

    game_history.push_back(state);

    int fromIdx= move.from.ToIndex();
    int toIdx= move.to.ToIndex();
    Piece::Piece movingPiece= squares[fromIdx];
    Piece::PieceType type= Piece::GetPieceType(movingPiece);

    // --- ZOBRIST UPDATES ---
    zobristHash^= zPiece[getPieceIndex(movingPiece)][fromIdx];
    if(squares[toIdx] != Piece::NONE) {
        zobristHash^= zPiece[getPieceIndex(squares[toIdx])][toIdx];
    }
    zobristHash^= zCastling[castlingRights];
    if(enPassantIndex != -1) zobristHash^= zEnPassant[enPassantIndex];
    else zobristHash^= zEnPassant[64];

    // 2. Flags
    bool isCastling= (type == Piece::KING && std::abs(move.from.file - move.to.file) > 1);
    bool isEnPassant= (type == Piece::PAWN && toIdx == enPassantIndex && squares[toIdx] == Piece::NONE);

    // 3. Update Squares
    squares[toIdx]= movingPiece;
    squares[fromIdx]= Piece::NONE;

    if(Piece::GetPieceType(move.movedPiece) == Piece::KING) {
        if(activeColor == Piece::WHITE) whiteKingPos= move.to;
        else blackKingPos= move.to;
    }

    // 4. Handle Captures
    if(move.captured != Piece::NONE) {
        int capIdx= toIdx;
        if(isEnPassant) {
            // En passant: captured pawn is on a different square
            Coordinate capturePos= Coordinate(move.to.file, move.from.rank);
            capIdx= capturePos.ToIndex();
            squares[capIdx]= Piece::NONE;
        }
        zobristHash^= zPiece[getPieceIndex(move.captured)][capIdx];
    }

    // Update Kings' positions if needed
    if(type == Piece::KING) {
        if(Piece::IsColor(movingPiece, Piece::WHITE)) {
            whiteKingPos= move.to;
        } else {
            blackKingPos= move.to;
        }
    }

    // C. Handle Special Moves (Rooks/EP Squares)
    if(isCastling) {
        int rookFromIdx, rookToIdx;
        Coordinate rookFromCoord(0), rookToCoord(0);

        if(toIdx > fromIdx) { // King-side
            rookFromIdx= fromIdx + 3;
            rookToIdx= fromIdx + 1;
            rookFromCoord= Coordinate(move.from.file + 3, move.from.rank);
            rookToCoord= Coordinate(move.from.file + 1, move.from.rank);
        } else { // Queen-side
            rookFromIdx= fromIdx - 4;
            rookToIdx= fromIdx - 1;
            rookFromCoord= Coordinate(move.from.file - 4, move.from.rank);
            rookToCoord= Coordinate(move.from.file - 1, move.from.rank);
        }

        // Update Rook on Board
        Piece::Piece rook= squares[rookFromIdx];
        squares[rookToIdx]= rook;
        squares[rookFromIdx]= Piece::NONE;

        zobristHash^= zPiece[getPieceIndex(rook)][rookFromIdx];
        zobristHash^= zPiece[getPieceIndex(rook)][rookToIdx];
    } else if(isEnPassant) {
        int capIdx= (activeColor == Piece::WHITE) ? (toIdx - 8) : (toIdx + 8);
        Piece::Piece capPawn= squares[capIdx];
        squares[capIdx]= Piece::NONE;

        zobristHash^= zPiece[getPieceIndex(capPawn)][capIdx];
    }

    // D. Final Zobrist (Add Moving Piece at Dest)
    if(move.promotion != Piece::NONE) {
        squares[toIdx]= move.promotion;
        zobristHash^= zPiece[getPieceIndex(move.promotion)][toIdx];
    } else {
        zobristHash^= zPiece[getPieceIndex(movingPiece)][toIdx];
    }

    // 5. Update Castling Rights
    if(type == Piece::KING) {
        if(activeColor == Piece::WHITE) castlingRights&= ~(CASTLE_WK | CASTLE_WQ);
        else castlingRights&= ~(CASTLE_BK | CASTLE_BQ);
    }
    if(fromIdx == 0 || toIdx == 0) castlingRights&= ~CASTLE_WQ;
    if(fromIdx == 7 || toIdx == 7) castlingRights&= ~CASTLE_WK;
    if(fromIdx == 56 || toIdx == 56) castlingRights&= ~CASTLE_BQ;
    if(fromIdx == 63 || toIdx == 63) castlingRights&= ~CASTLE_BK;

    // 6. Update En Passant Target
    if(type == Piece::PAWN && std::abs(move.from.rank - move.to.rank) == 2) {
        enPassantIndex= (fromIdx + toIdx) / 2;
    } else {
        enPassantIndex= -1;
    }

    // 7. Finalize State
    zobristHash^= zCastling[castlingRights];
    if(enPassantIndex != -1) zobristHash^= zEnPassant[enPassantIndex];
    else zobristHash^= zEnPassant[64];

    zobristHash^= zSide;

    if(type == Piece::PAWN || move.captured != Piece::NONE) halfMoveClock= 0;
    else halfMoveClock++;

    if(activeColor == Piece::BLACK) fullMoveNumber++;
    activeColor= (activeColor == Piece::WHITE) ? Piece::BLACK : Piece::WHITE;
}

void Board::makeNullMove() {
    GameState state;
    state.move= Move(); // Dummy move
    state.capturedPiece= Piece::NONE;
    state.castlingRights= castlingRights;
    state.enPassantIndex= enPassantIndex;
    state.halfMoveClock= halfMoveClock;
    state.zobristHash= zobristHash;
    state.whiteKingPos= whiteKingPos;
    state.blackKingPos= blackKingPos;
    game_history.push_back(state);

    // Update en passant in hash
    if(enPassantIndex != -1) zobristHash^= zEnPassant[enPassantIndex];
    else zobristHash^= zEnPassant[64];
    enPassantIndex= -1;
    zobristHash^= zEnPassant[64];

    // Toggle side to move
    zobristHash^= zSide;
    activeColor= (activeColor == Piece::WHITE) ? Piece::BLACK : Piece::WHITE;
    halfMoveClock++;
}

void Board::undoNullMove() {
    if(game_history.empty()) return;
    GameState lastState= game_history.back();
    game_history.pop_back();

    castlingRights= lastState.castlingRights;
    enPassantIndex= lastState.enPassantIndex;
    halfMoveClock= lastState.halfMoveClock;
    zobristHash= lastState.zobristHash;
    whiteKingPos= lastState.whiteKingPos;
    blackKingPos= lastState.blackKingPos;
    activeColor= (activeColor == Piece::WHITE) ? Piece::BLACK : Piece::WHITE;
}

void Board::undoMove() {
    if(game_history.empty()) return;

    GameState lastState= game_history.back();
    game_history.pop_back();

    castlingRights= lastState.castlingRights;
    enPassantIndex= lastState.enPassantIndex;
    halfMoveClock= lastState.halfMoveClock;
    zobristHash= lastState.zobristHash;
    whiteKingPos= lastState.whiteKingPos;
    blackKingPos= lastState.blackKingPos;

    // Restore the move
    const Move& move= lastState.move;
    squares[move.from.ToIndex()]= move.movedPiece;       // Use original piece (important for promotions!)
    squares[move.to.ToIndex()]= lastState.capturedPiece; // Normal Capture undo

    // Handle EnPassant Undo - must check that a capture actually occurred
    if(move.captured != Piece::NONE && lastState.enPassantIndex != -1 && Piece::GetPieceType(move.movedPiece) == Piece::PAWN && move.to.ToIndex() == lastState.enPassantIndex) {
        squares[move.to.ToIndex()]= Piece::NONE; // En passant target square was empty
        int capIdx= (activeColor == Piece::WHITE) ? (lastState.enPassantIndex + 8) : (lastState.enPassantIndex - 8);
        squares[capIdx]= move.captured; // Restore captured pawn at its original position
    }

    // Handle Castling Undo
    auto diff= move.to.file - move.from.file;
    if(Piece::GetPieceType(move.movedPiece) == Piece::KING && std::abs(diff) == 2) {
        if(diff > 0) { // King-side
            int rookFromIdx= move.from.ToIndex() + 3;
            int rookToIdx= move.from.ToIndex() + 1;
            squares[rookFromIdx]= squares[rookToIdx];
            squares[rookToIdx]= Piece::NONE;
        } else { // Queen-side
            int rookFromIdx= move.from.ToIndex() - 4;
            int rookToIdx= move.from.ToIndex() - 1;
            squares[rookFromIdx]= squares[rookToIdx];
            squares[rookToIdx]= Piece::NONE;
        }
    }

    // Restore state variables
    castlingRights= lastState.castlingRights;
    enPassantIndex= lastState.enPassantIndex;
    halfMoveClock= lastState.halfMoveClock;
    zobristHash= lastState.zobristHash;
    whiteKingPos= lastState.whiteKingPos;
    blackKingPos= lastState.blackKingPos;

    activeColor= (activeColor == Piece::WHITE) ? Piece::BLACK : Piece::WHITE;
    if(activeColor == Piece::BLACK) fullMoveNumber--;
}

void Board::print() const {
    std::cout << "\n  +-----------------+\n";
    for(int rank= 7; rank >= 0; --rank) {
        std::cout << rank + 1 << " | ";
        for(int file= 0; file < 8; ++file) {
            int square= rank * 8 + file;
            Piece::Piece p= squares[square];
            char c= p != Piece::NONE ? Piece::GetSymbol(p) : '.';
            std::cout << c << " ";
        }
        std::cout << "|\n";
    }
    std::cout << "  +-----------------+\n";
    std::cout << "    a b c d e f g h\n\n";
    std::cout << "Zobrist Hash: " << std::hex << zobristHash << std::dec << "\n";
}
} // namespace talawachess::core::board