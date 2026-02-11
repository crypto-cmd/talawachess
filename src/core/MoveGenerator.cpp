#include "MoveGenerator.hpp"
#include "Board.hpp"
#include "Coordinate.hpp"
#include "Move.hpp"
#include "Piece.hpp"
#include <algorithm>
#include <vector>

using namespace talawachess::core::board;
using namespace talawachess::core;

// --- Helper: Directions ---
// We define these locally so we can use them in isSquareAttacked without relying on class members
const std::vector<Coordinate> KNIGHT_DIRS= {
    {-1, -2}, {1, -2}, {-2, -1}, {2, -1}, {-2, 1}, {2, 1}, {-1, 2}, {1, 2}};
const std::vector<Coordinate> BISHOP_DIRS= {{-1, -1}, {1, -1}, {-1, 1}, {1, 1}};
const std::vector<Coordinate> ROOK_DIRS= {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
const std::vector<Coordinate> KING_DIRS= {
    {-1, -1}, {0, -1}, {1, -1}, {-1, 0}, {1, 0}, {-1, 1}, {0, 1}, {1, 1}};

// --- Helper: Attack Detection ---
// Returns true if 'square' is being attacked by 'attackerColor'
bool isSquareAttacked(const Board& board, Coordinate square, Piece::Color attackerColor) {
    // 1. Check for Knight attacks (If a knight is on a square a knight would jump to)
    for(const auto& dir: KNIGHT_DIRS) {
        Coordinate target= square + dir;
        if(target.IsValid()) {
            Piece::Piece p= board.squares[target.ToIndex()];
            if(p != Piece::NONE && Piece::IsColor(p, attackerColor) && Piece::IsType(p, Piece::KNIGHT)) {
                return true;
            }
        }
    }

    // 2. Check for Pawn attacks
    // Pawns attack "backwards" from the perspective of the square
    int pawnRankDir= (attackerColor == Piece::WHITE) ? -1 : 1;
    for(int fileOffset: {-1, 1}) {
        Coordinate target(square.file + fileOffset, square.rank + pawnRankDir);
        if(target.IsValid()) {
            Piece::Piece p= board.squares[target.ToIndex()];
            if(p != Piece::NONE && Piece::IsColor(p, attackerColor) && Piece::IsType(p, Piece::PAWN)) {
                return true;
            }
        }
    }

    // 3. Check for Sliding pieces (Rook/Queen)
    for(const auto& dir: ROOK_DIRS) {
        Coordinate target= square + dir;
        while(target.IsValid()) {
            Piece::Piece p= board.squares[target.ToIndex()];
            if(p != Piece::NONE) {
                if(Piece::IsColor(p, attackerColor) && (Piece::IsType(p, Piece::ROOK) || Piece::IsType(p, Piece::QUEEN))) {
                    return true;
                }
                break; // Blocked
            }
            target= target + dir;
        }
    }

    // 4. Check for Sliding pieces (Bishop/Queen)
    for(const auto& dir: BISHOP_DIRS) {
        Coordinate target= square + dir;
        while(target.IsValid()) {
            Piece::Piece p= board.squares[target.ToIndex()];
            if(p != Piece::NONE) {
                if(Piece::IsColor(p, attackerColor) && (Piece::IsType(p, Piece::BISHOP) || Piece::IsType(p, Piece::QUEEN))) {
                    return true;
                }
                break; // Blocked
            }
            target= target + dir;
        }
    }

    // 5. Check for King attacks (adjacent king)
    for(const auto& dir: KING_DIRS) {
        Coordinate target= square + dir;
        if(target.IsValid()) {
            Piece::Piece p= board.squares[target.ToIndex()];
            if(p != Piece::NONE && Piece::IsColor(p, attackerColor) && Piece::IsType(p, Piece::KING)) {
                return true;
            }
        }
    }

    return false;
}

std::vector<Move> MoveGenerator::generateMoves(const Board& board) {
    std::vector<Move> moves;
    for(auto& [piece, coord]: board.pieces) {
        if(Piece::GetColor(piece) != board.activeColor)
            continue; // Skip opponent's pieces

        Piece::PieceType type= Piece::GetPieceType(piece);
        switch(type) {
        case Piece::PAWN:
            generatePawnMoves(board, piece, coord, moves);
            break;
        case Piece::KNIGHT:
            generateKnightMoves(board, piece, coord, moves);
            break;
        case Piece::BISHOP:
            generateSlidingMoves(board, piece, coord, moves, pieceDirections.at(Piece::BISHOP));
            break;
        case Piece::ROOK:
            generateSlidingMoves(board, piece, coord, moves, pieceDirections.at(Piece::ROOK));
            break;
        case Piece::QUEEN:
            generateSlidingMoves(board, piece, coord, moves, pieceDirections.at(Piece::QUEEN));
            break;
        case Piece::KING:
            generateKingMoves(board, piece, coord, moves);
            break;
        }
    }
    return moves;
}

std::vector<Move> MoveGenerator::generateLegalMoves(Board& board) {
    auto pseudoMoves= generateMoves(board);
    std::vector<Move> legalMoves;
    for(const Move& move: pseudoMoves) {
        board.makeMove(move);

        // Find our king:
        Coordinate myKingPos(0, 0);
        bool kingFound= false;
        // Search pieces list for king
        Piece::Color myColor= (board.activeColor == Piece::WHITE) ? Piece::BLACK : Piece::WHITE;
        // Note: board.activeColor flipped after makeMove, so we want the "old" active color

        for(auto& [p, c]: board.pieces) {
            if(Piece::IsColor(p, myColor) && Piece::IsType(p, Piece::KING)) {
                myKingPos= c;
                kingFound= true;
                break;
            }
        }

        if(kingFound) {
            if(!isSquareAttacked(board, myKingPos, board.activeColor)) {
                legalMoves.push_back(move);
            }
        } else {
            // If king is captured/missing, move is illegal (or variant logic)
        }

        board.undoMove();
    }
    return legalMoves;
}

void MoveGenerator::generatePawnMoves(const Board& board, Piece::Piece piece, Coordinate coord, std::vector<Move>& moves) {
    auto direction= Piece::GetColor(piece) == Piece::WHITE ? Coordinate(0, 1) : Coordinate(0, -1);
    auto isStartingRank= (coord.rank == 1 && Piece::GetColor(piece) == Piece::WHITE) || (coord.rank == 6 && Piece::GetColor(piece) == Piece::BLACK);

    auto normalForward= coord + direction;

    auto normalForwardAvailable= normalForward.IsValid() && board.squares[normalForward.ToIndex()] == Piece::NONE;
    if(normalForwardAvailable) {

        auto move= Move{coord, normalForward, Piece::NONE, Piece::NONE, piece};
        auto isGoingToPromotionRank= (normalForward.rank == 7) || (normalForward.rank == 0);
        if(isGoingToPromotionRank) {
            // Add 4 promotion moves for each possible piece
            for(Piece::PieceType promoType: {Piece::QUEEN, Piece::ROOK, Piece::BISHOP, Piece::KNIGHT}) {
                move.promotion= static_cast<Piece::Piece>(Piece::GetColor(piece) | promoType);
                moves.push_back(move);
            }
        } else {
            moves.push_back(move);
        }
    }
    auto doubleForward= normalForward + direction;
    if(isStartingRank && normalForwardAvailable && doubleForward.IsValid() && board.squares[doubleForward.ToIndex()] == Piece::NONE) {
        moves.push_back(Move{coord, doubleForward, Piece::NONE, Piece::NONE, piece});
    }
    // Captures
    for(int fileOffset: {-1, 1}) {
        auto captureCoord= coord + direction + Coordinate(fileOffset, 0);
        auto isGoingToPromotionRank= (captureCoord.rank == 7) || (captureCoord.rank == 0);

        auto move= Move{coord, captureCoord, Piece::NONE, Piece::NONE, piece};
        if(captureCoord.IsValid() && board.squares[captureCoord.ToIndex()] != Piece::NONE && Piece::GetColor(board.squares[captureCoord.ToIndex()]) != Piece::GetColor(piece)) {
            auto targetPiece= board.squares[captureCoord.ToIndex()];
            move.captured= targetPiece;
            if(isGoingToPromotionRank) {
                // Add 4 promotion moves for each possible piece
                for(Piece::PieceType promoType: {Piece::QUEEN, Piece::ROOK, Piece::BISHOP, Piece::KNIGHT}) {
                    move.promotion= static_cast<Piece::Piece>(Piece::GetColor(piece) | promoType);
                    moves.push_back(move);
                }
            } else {
                moves.push_back(move);
            }
        }
    }

    // En passant
    const auto besideMeOffsets= {Coordinate(-1, 0), Coordinate(1, 0)};
    for(const Coordinate& offset: besideMeOffsets) {
        auto besideMe= coord + offset;
        if(!besideMe.IsValid()) continue;
        auto targetPiece= board.squares[besideMe.ToIndex()];
        if(Piece::GetColor(targetPiece) == Piece::GetColor(piece) || Piece::GetPieceType(targetPiece) != Piece::PAWN) continue;
        if(board.game_history.empty()) continue;

        const auto& lastMove= board.game_history.back();
        const auto behindEnemyPawn= besideMe + direction;

        if(!behindEnemyPawn.IsValid()) continue;

        const auto squares= lastMove.squares;
        if(squares[behindEnemyPawn.ToIndex()] == Piece::NONE && squares[besideMe.ToIndex()] == Piece::NONE) {

            moves.push_back(Move{coord, behindEnemyPawn, Piece::NONE, targetPiece, piece});
        }
    }
}

void MoveGenerator::generateKnightMoves(const Board& board, Piece::Piece piece, Coordinate coord, std::vector<Move>& moves) {
    for(const Coordinate& dir: pieceDirections.at(Piece::KNIGHT)) {
        auto targetCoord= coord + dir;
        if(!targetCoord.IsValid())
            continue;

        auto targetPiece= board.squares[targetCoord.ToIndex()];
        if(targetPiece == Piece::NONE || Piece::GetColor(targetPiece) != Piece::GetColor(piece)) {
            moves.push_back(Move{coord, targetCoord, Piece::NONE, targetPiece, piece});
        }
    }
}

void MoveGenerator::generateSlidingMoves(const Board& board, Piece::Piece piece, Coordinate coord, std::vector<Move>& moves, const std::vector<Coordinate>& directions) {
    for(const Coordinate& dir: directions) {
        auto targetCoord= coord + dir;
        while(targetCoord.IsValid()) {
            auto targetPiece= board.squares[targetCoord.ToIndex()];
            if(targetPiece == Piece::NONE) {
                moves.push_back(Move{coord, targetCoord, Piece::NONE, Piece::NONE, piece});
            } else {
                if(Piece::GetColor(targetPiece) != Piece::GetColor(piece)) {
                    moves.push_back(Move{coord, targetCoord, Piece::NONE, targetPiece, piece});
                }
                break; // Blocked by a piece
            }
            targetCoord= targetCoord + dir;
        }
    }
}

void MoveGenerator::generateKingMoves(const Board& board, Piece::Piece piece, Coordinate coord, std::vector<Move>& moves) {
    for(const Coordinate& dir: pieceDirections.at(Piece::KING)) {
        auto targetCoord= coord + dir;
        if(!targetCoord.IsValid())
            continue;

        auto targetPiece= board.squares[targetCoord.ToIndex()];
        if(targetPiece == Piece::NONE || Piece::GetColor(targetPiece) != Piece::GetColor(piece)) {
            moves.push_back(Move{coord, targetCoord, Piece::NONE, targetPiece, piece});
        }
    }

    //
    // 2. Castling Moves
    // Conditions:
    // A. Correct Rights
    // B. Path Empty
    // C. Not in check, not passing through check, not landing in check

    Piece::Color myColor= Piece::GetColor(piece);
    Piece::Color oppColor= (myColor == Piece::WHITE) ? Piece::BLACK : Piece::WHITE;

    // We first check if we are currently in check. If so, castling is illegal.
    if(isSquareAttacked(board, coord, oppColor)) return;

    if(myColor == Piece::WHITE) {
        // King-side (e1 -> g1)
        if((board.castlingRights & Board::CASTLE_WK) &&
           board.squares[Coordinate("f1").ToIndex()] == Piece::NONE &&
           board.squares[Coordinate("g1").ToIndex()] == Piece::NONE) {

            if(!isSquareAttacked(board, Coordinate("f1"), oppColor) &&
               !isSquareAttacked(board, Coordinate("g1"), oppColor)) {
                moves.push_back(Move{coord, Coordinate("g1"), Piece::NONE, Piece::NONE, piece});
            }
        }
        // Queen-side (e1 -> c1)
        if((board.castlingRights & Board::CASTLE_WQ) &&
           board.squares[Coordinate("d1").ToIndex()] == Piece::NONE &&
           board.squares[Coordinate("c1").ToIndex()] == Piece::NONE &&
           board.squares[Coordinate("b1").ToIndex()] == Piece::NONE) { // b1 must be empty too!

            if(!isSquareAttacked(board, Coordinate("d1"), oppColor) &&
               !isSquareAttacked(board, Coordinate("c1"), oppColor)) {
                moves.push_back(Move{coord, Coordinate("c1"), Piece::NONE, Piece::NONE, piece});
            }
        }
    } else {
        // Black King-side (e8 -> g8)
        if((board.castlingRights & Board::CASTLE_BK) &&
           board.squares[Coordinate("f8").ToIndex()] == Piece::NONE &&
           board.squares[Coordinate("g8").ToIndex()] == Piece::NONE) {

            if(!isSquareAttacked(board, Coordinate("f8"), oppColor) &&
               !isSquareAttacked(board, Coordinate("g8"), oppColor)) {
                moves.push_back(Move{coord, Coordinate("g8"), Piece::NONE, Piece::NONE, piece});
            }
        }
        // Black Queen-side (e8 -> c8)
        if((board.castlingRights & Board::CASTLE_BQ) &&
           board.squares[Coordinate("d8").ToIndex()] == Piece::NONE &&
           board.squares[Coordinate("c8").ToIndex()] == Piece::NONE &&
           board.squares[Coordinate("b8").ToIndex()] == Piece::NONE) {

            if(!isSquareAttacked(board, Coordinate("d8"), oppColor) &&
               !isSquareAttacked(board, Coordinate("c8"), oppColor)) {
                moves.push_back(Move{coord, Coordinate("c8"), Piece::NONE, Piece::NONE, piece});
            }
        }
    }
}
