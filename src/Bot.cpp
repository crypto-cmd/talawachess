#include "Bot.hpp"
#include "Board.hpp"
#include <chrono>
namespace talawachess {
using namespace core::board;
using namespace core::Piece;
using namespace core;
const int Bot::PieceValues[7]= {0, 100, 300, 350, 500, 900, 20000}; // NONE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING
std::array<std::array<int, 64>, 7> Bot::PieceSquareTables= {{{0}}}; // Initialize all PSTs to 0

Bot::Bot(): _board(),
            _moveGen(_board) {
    // Initialize pawn piece-square table
    PieceSquareTables[core::Piece::PAWN]=
        {0, 0, 0, 0, 0, 0, 0, 0,
         78, 83, 86, 73, 102, 82, 85, 90,
         7, 29, 21, 44, 40, 31, 44, 7,
         -17, 16, -2, 15, 14, 0, 15, -13,
         -26, 3, 10, 9, 6, 1, 0, -23,
         -22, 9, 5, -11, -10, -2, 3, -19,
         -31, 8, -7, -37, -36, -14, 3, -31,
         0, 0, 0, 0, 0, 0, 0, 0};
    PieceSquareTables[core::Piece::KNIGHT]=
        {-66, -53, -75, -75, -10, -55, -58, -70,
         -3, -6, 100, -36, 4, 62, -4, -14,
         10, 67, 1, 74, 73, 27, 62, -2,
         24, 24, 45, 37, 33, 41, 25, 17,
         -1, 5, 31, 21, 22, 35, 2, 0,
         -18, 10, 13, 22, 18, 15, 11, -14,
         -23, -15, 2, 0, 2, 0, -23, -20,
         -74, -23, -26, -24, -19, -35, -22, -69};
    PieceSquareTables[core::Piece::BISHOP]=
        {-59, -78, -82, -76, -23, -107, -37, -50,
         -11, 20, 35, -42, -39, 31, 2, -22,
         -9, 39, -32, 41, 52, -10, 28, -14,
         25, 17, 20, 34, 26, 25, 15, 10,
         13, 10, 17, 23, 17, 16, 0, 7,
         14, 25, 24, 15, 8, 25, 20, 15,
         19, 20, 11, 6, 7, 6, 20, 16,
         -7, 2, -15, -12, -14, -15, -10, -10};
    PieceSquareTables[core::Piece::ROOK]=
        {35, 29, 33, 4, 37, 33, 56, 50,
         55, 29, 56, 67, 55, 62, 34, 60,
         19, 35, 28, 33, 45, 27, 25, 15,
         0, 5, 16, 13, 18, -4, -9, -6,
         -28, -35, -16, -21, -13, -29, -46, -30,
         -42, -28, -42, -25, -25, -35, -26, -46,
         -53, -38, -31, -26, -29, -43, -44, -53,
         -30, -24, -18, 5, -2, -18, -31, -32};
    PieceSquareTables[core::Piece::QUEEN]=
        {6, 1, -8, -104, 69, 24, 88, 26,
         14, 32, 60, -10, 20, 76, 57, 24,
         -2, 43, 32, 60, 72, 63, 43, 2,
         1, -16, 22, 17, 25, 20, -13, -6,
         -14, -15, -2, -5, -1, -10, -20, -22,
         -30, -6, -13, -11, -16, -11, -16, -27,
         -36, -18, 0, -19, -15, -15, -21, -38,
         -39, -30, -31, -13, -31, -36, -34, -42};
    PieceSquareTables[core::Piece::KING]=
        {4, 54, 47, -99, -99, 60, 83, -62,
         -32, 10, 55, 56, 56, 55, 10, 3,
         -62, 12, -57, 44, -67, 28, 37, -31,
         -55, 50, 11, -4, -19, 13, 0, -49,
         -55, -43, -52, -28, -51, -47, -8, -50,
         -47, -42, -43, -79, -64, -32, -29, -32,
         -4, 3, -14, -50, -57, -18, 13, 4,
         17, 30, -3, -14, 6, -1, 40, 18};
}
void Bot::setFen(const std::string& fen) {
    _board.setFen(fen);
}
void Bot::performMove(const std::string& moveStr) {
    // Convert moveStr (e.g., "e2e4") to a Move object
    auto& moves= _moveGen.generateLegalMoves();
    for(const auto& move: moves) {
        if(move.ToString() == moveStr) {
            _board.makeMove(move);
            return;
        }
    }
    throw std::invalid_argument("Invalid move string: " + moveStr);
}
void Bot::orderMoves(std::vector<core::Move>& moves) const{
    for(auto& move: moves) {
        move.score= 0;
        // 2. Prioritize Captures using MVV-LVA
        if(move.captured != core::Piece::NONE) {
            // (Victim Value * 10) - Attacker Value
            // e.g., P takes Q = 9000 - 100 = 8900
            // e.g., Q takes P = 1000 - 900 = 100
            move.score= 10000 + (PieceValues[move.captured] * 10) - (PieceValues[move.movedPiece]);
        }
        // 3. Prioritize Promotions
        else if(move.promotion != core::Piece::NONE) {
            move.score= 9000 + PieceValues[move.promotion];
        } else {
            move.score= 0;
        }
    }

    // Sort descending by score
    std::sort(moves.begin(), moves.end(), [](const core::Move& a, const core::Move& b) {
        return a.score > b.score;
    });
}
int Bot::search(int depth, int alpha, int beta) {
    using namespace talawachess::core::Piece;
    using namespace talawachess::core::board;
    if(depth == 0) return evaluate(); // quiesce(alpha, beta);
    auto moves= _moveGen.generateLegalMoves();

    orderMoves(moves); // Move ordering for better alpha-beta performance
    if(moves.empty()) {
        // Check for checkmate or stalemate
        auto attackerColor= _board.activeColor == Color::WHITE ? Color::BLACK : Color::WHITE;
        auto kingPos= _board.activeColor == Color::WHITE ? _board.whiteKingPos : _board.blackKingPos;
        if(MoveGenerator::isSquareAttacked(_board, kingPos, attackerColor)) {
            return -MATE_VAL + (10 - depth); // Checkmate, prefer faster mates
        } else {
            return 0; // Stalemate
        }
    }

    for(const auto& move: moves) {
        _board.makeMove(move);
        int evaluation= -search(depth - 1, -beta, -alpha);
        _board.undoMove();
        if(evaluation >= beta) {
            return beta; // Snip
        }
        alpha= std::max(alpha, evaluation);
    }
    return alpha;
}
int Bot::evaluate() const {
    int score= 0;
    for(const auto& [piece, coord]: _board.pieces) {
        if(piece == Piece::NONE) continue;
        int pieceType= Piece::GetPieceType(piece);
        int pieceValue= PieceValues[pieceType];
        int pstValue= PieceSquareTables[pieceType][coord.ToIndex()];
        int value= pieceValue + pstValue;
        int colorMultiplier= Piece::IsColor(piece, Piece::WHITE) ? 1 : -1;
        score+= colorMultiplier * value;
    }
    int perspectiveMultiplier= _board.activeColor == Piece::WHITE ? 1 : -1;
    score*= perspectiveMultiplier; // Positive if good for side to move, negative if bad
    return score;
}
int Bot::quiesce(int alpha, int beta) {
    return evaluate(); // Placeholder: In a full implementation, this would explore captures and checks until a quiet position is reached
}

std::pair<core::Move, int> Bot::getBestMove(int timeLimitMs) {
    auto startTime= std::chrono::high_resolution_clock::now();

    core::Move bestMove;
    int bestScore= -INF;

    for(int depth= 1; depth <= 10; ++depth) { // Iterative deepening
        auto moves= _moveGen.generateLegalMoves();
        if(moves.empty()) break; // No moves available

        for(const auto& move: moves) {
            _board.makeMove(move);
            int score= -search(depth - 1, -INF, INF);
            _board.undoMove();

            if(score > bestScore) {
                bestScore= score;
                bestMove= move;
            }
        }

        auto currentTime= std::chrono::high_resolution_clock::now();
        int elapsedMs= std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count();
        if(elapsedMs >= timeLimitMs) {
            break; // Time limit reached
        }
    }
    return {bestMove, bestScore};
}
} // namespace talawachess
