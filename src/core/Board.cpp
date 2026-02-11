#include "Board.hpp"
#include "Coordinate.hpp"
#include <sstream>

using namespace talawachess::core::board;
Board::Board(): squares(64, Piece::NONE) {}

void Board::setFen(const std::string& fen) {
    // Clear the board
    std::fill(squares.begin(), squares.end(), Piece::NONE);
    pieces.clear();
    game_history.clear();
    std::stringstream ss(fen);
    std::string boardPart, activeColor, castling, enPassant;
    int halfmoveClock, fullmoveNumber;

    std::vector<std::string> parts;
    std::string part;
    while(std::getline(ss, part, ' ')) {
        parts.push_back(part);
    }
    if(parts.size() != 6) {
        std::cerr << "Invalid FEN: Incorrect number of fields" << std::endl;
        return;
    }

    boardPart= parts[0];
    activeColor= parts[1];
    castling= parts[2];
    enPassant= parts[3];
    halfmoveClock= std::stoi(parts[4]);
    fullmoveNumber= std::stoi(parts[5]);

    // Piece placement
    Coordinate coord= Coordinate::FromAlgebraic("a8");
    for(char c: boardPart) {
        if(c == '/') {
            coord= Coordinate(0, coord.rank - 1);
            continue;
        }
        if(std::isdigit(c)) {
            // Skip empty squares
            coord= Coordinate(coord.file + (c - '0'), coord.rank);
        } else {
            int squareIndex= coord.ToIndex();
            Piece::Piece piece= Piece::FromSymbol(c);
            squares[squareIndex]= piece;
            pieces.push_back({piece, coord}); // Add to pieces list
            coord= Coordinate(coord.file + 1, coord.rank);
        }
    }

    // Active color
    this->activeColor= activeColor == "w" ? Piece::WHITE : Piece::BLACK;

    this->castlingRights= 0;
    if(parts[2] != "-") {
        for(char c: parts[2]) {
            if(c == 'K') this->castlingRights|= CASTLE_WK;
            else if(c == 'Q') this->castlingRights|= CASTLE_WQ;
            else if(c == 'k') this->castlingRights|= CASTLE_BK;
            else if(c == 'q') this->castlingRights|= CASTLE_BQ;
        }
    }

    if(parts[3] != "-") {
        this->enPassantIndex= Coordinate::FromAlgebraic(parts[3].c_str()).ToIndex();
    } else {
        this->enPassantIndex= -1;
    }

    // 6. Parse Clocks (parts[4] and parts[5])

    this->halfMoveClock= std::stoi(parts[4]);
    this->fullMoveNumber= std::stoi(parts[5]);
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

    // std::cout << "Side to move: " << (sideToMove == Color::White ? "White" : "Black") << "\n";
    // std::cout << "Castling: "
    //           << (whiteCanCastleKingSide ? "K" : "")
    //           << (whiteCanCastleQueenSide ? "Q" : "")
    //           << (blackCanCastleKingSide ? "k" : "")
    //           << (blackCanCastleQueenSide ? "q" : "")
    //           << "\n";
}

void Board::makeMove(const Move& move) {
    // 1. Save current state for undo
    GameState state;
    state.squares= squares;
    state.castlingRights= castlingRights;
    state.enPassantIndex= enPassantIndex;
    state.halfMoveClock= halfMoveClock;
    game_history.push_back(state);

    int fromIdx= move.from.ToIndex();
    int toIdx= move.to.ToIndex();
    Piece::Piece movingPiece= squares[fromIdx];
    Piece::PieceType type= Piece::GetPieceType(movingPiece);

    // 2. Logic to detect special moves
    bool isCastling= (type == Piece::KING && std::abs(move.from.file - move.to.file) > 1);
    // En Passant: Pawn moves diagonally but destination was empty
    bool isEnPassant= (type == Piece::PAWN && toIdx == enPassantIndex && squares[toIdx] == Piece::NONE);

    // 3. Move the piece (Basic Move)
    squares[toIdx]= movingPiece;
    squares[fromIdx]= Piece::NONE;

    // 4. Handle Special Moves
    if(isCastling) {
        // Move the Rook
        int rookFromIdx, rookToIdx;
        if(toIdx > fromIdx) {         // King-side
            rookFromIdx= fromIdx + 3; // e.g., h1
            rookToIdx= fromIdx + 1;   // e.g., f1
        } else {                      // Queen-side
            rookFromIdx= fromIdx - 4; // e.g., a1
            rookToIdx= fromIdx - 1;   // e.g., d1
        }
        // Perform Rook move
        if(rookFromIdx >= 0 && rookFromIdx < 64) {
            squares[rookToIdx]= squares[rookFromIdx];
            squares[rookFromIdx]= Piece::NONE;
        }
    } else if(isEnPassant) {
        // Remove the captured pawn (which is "behind" the moving pawn)
        int capturedPawnIdx= (activeColor == Piece::WHITE) ? (toIdx - 8) : (toIdx + 8);
        squares[capturedPawnIdx]= Piece::NONE;
    } else if(move.promotion != Piece::NONE) {
        squares[toIdx]= move.promotion;
    }

    // 5. Update Castling Rights
    // If King moves, lose all rights
    if(type == Piece::KING) {
        if(activeColor == Piece::WHITE) castlingRights&= ~(CASTLE_WK | CASTLE_WQ);
        else castlingRights&= ~(CASTLE_BK | CASTLE_BQ);
    }
    // If Rook moves or is captured, update specific rights
    // (This is a simplified check; technically checking corners is usually sufficient)
    // For simplicity: Clear rights if start squares are touched
    if(fromIdx == 0 || toIdx == 0) castlingRights&= ~CASTLE_WQ;   // a1
    if(fromIdx == 7 || toIdx == 7) castlingRights&= ~CASTLE_WK;   // h1
    if(fromIdx == 56 || toIdx == 56) castlingRights&= ~CASTLE_BQ; // a8
    if(fromIdx == 63 || toIdx == 63) castlingRights&= ~CASTLE_BK; // h8

    // 6. Update En Passant Target
    if(type == Piece::PAWN && std::abs(move.from.rank - move.to.rank) == 2) {
        // Set en passant index to the square skipped
        enPassantIndex= (fromIdx + toIdx) / 2;
    } else {
        enPassantIndex= -1;
    }

    // 7. Update Clocks and Side
    if(type == Piece::PAWN || move.captured != Piece::NONE) {
        halfMoveClock= 0;
    } else {
        halfMoveClock++;
    }

    if(activeColor == Piece::BLACK) {
        fullMoveNumber++;
    }
    activeColor= (activeColor == Piece::WHITE) ? Piece::BLACK : Piece::WHITE;

    // 8. Rebuild piece list (Slow but safe for now)
    pieces.clear();
    for(int i= 0; i < 64; ++i) {
        if(squares[i] != Piece::NONE) {
            pieces.push_back({squares[i], Coordinate(i)});
        }
    }
}
void Board::undoMove() {
    if(game_history.empty())
        return;

    GameState lastState= game_history.back();
    game_history.pop_back();

    // Restore State
    squares= lastState.squares;
    castlingRights= lastState.castlingRights;
    enPassantIndex= lastState.enPassantIndex;
    halfMoveClock= lastState.halfMoveClock;

    // Flip turn back
    activeColor= (activeColor == Piece::WHITE) ? Piece::BLACK : Piece::WHITE;
    if(activeColor == Piece::BLACK) fullMoveNumber--; // Technically optional for logic but good for correctness

    // Rebuild pieces
    pieces.clear();
    for(int i= 0; i < 64; ++i) {
        if(squares[i] != Piece::NONE) {
            pieces.push_back({squares[i], Coordinate(i)});
        }
    }
}