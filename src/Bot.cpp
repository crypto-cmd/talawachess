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
    // Initialize Transposition Table (64 MB)
    resizeTT(512);
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
void Bot::resizeTT(size_t sizeInMB) {
    // Clear old entries and free their moves first
    for(auto& entry: _tt) {
        delete entry.bestMove;
    }
    size_t numEntries= (sizeInMB * 1024 * 1024) / sizeof(TTEntry);
    _tt.clear();
    _tt.resize(numEntries);
    // Initialize all entries properly
    for(auto& entry: _tt) {
        entry.bestMove= nullptr;
        entry.zobristHash= 0;
        entry.depth= -1;
        entry.flag= TT_EXACT;
        entry.score= -INF;
    }
}

void Bot::clearTT() {
    for(auto& entry: _tt) {
        delete entry.bestMove; // Free any existing move
        entry.bestMove= nullptr;
        entry.zobristHash= 0;
        entry.depth= -1;
        entry.flag= TT_EXACT;
        entry.score= -INF;
    }
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
void Bot::orderMoves(std::vector<core::Move>& moves, const core::Move* ttMove) const {
    for(auto& move: moves) {
        move.score= 0;
        // 1. Prioritize Transposition Table Move
        if(ttMove != nullptr && move.from == ttMove->from && move.to == ttMove->to) {
            move.score= 2000000; // Highest priority
            continue;
        }
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
int Bot::search(int depth, int ply, int alpha, int beta) {
    using namespace talawachess::core::Piece;
    using namespace talawachess::core::board;

    TTEntry& ttEntry= _tt[_board.zobristHash % _tt.size()];
    bool ttHit= (ttEntry.zobristHash == _board.zobristHash);
    core::Move* ttBestMove= nullptr;
    if(ttHit) {
        ttBestMove= ttEntry.bestMove;
        if(ttEntry.depth >= depth) {
            int score= ttEntry.score;

            if(score > MATE_VAL - 100) score-= ply; // Adjust mate scores for distance
            else if(score < -MATE_VAL + 100) score+= ply;

            if(ttEntry.flag == TT_EXACT) return score;
            if(ttEntry.flag == TT_ALPHA && score <= alpha) return alpha;
            if(ttEntry.flag == TT_BETA && score >= beta) return beta;
        }
    }

    if(depth == 0) return quiesce(alpha, beta, ply);
    auto moves= _moveGen.generateLegalMoves();

    orderMoves(moves, ttBestMove); // Move ordering for better alpha-beta performance
    if(moves.empty()) {
        // Check for checkmate or stalemate
        auto attackerColor= _board.activeColor == Color::WHITE ? Color::BLACK : Color::WHITE;
        auto kingPos= _board.activeColor == Color::WHITE ? _board.whiteKingPos : _board.blackKingPos;
        if(MoveGenerator::isSquareAttacked(_board, kingPos, attackerColor)) {
            checkMatesFound++;
            return -MATE_VAL + ply; // Checkmate, prefer faster mates
        } else {
            return 0; // Stalemate
        }
    }

    int originalAlpha= alpha;
    core::Move bestMoveThisNode;
    for(const auto& move: moves) {
        _board.makeMove(move);
        int evaluation= -search(depth - 1, ply + 1, -beta, -alpha);
        _board.undoMove();
        if(evaluation >= beta) {
            int storedScore= evaluation;
            if(storedScore > MATE_VAL - 100) storedScore+= ply;
            else if(storedScore < -MATE_VAL + 100) storedScore-= ply;

            ttEntry.zobristHash= _board.zobristHash;
            delete ttEntry.bestMove;                // Free old move before overwriting
            ttEntry.bestMove= new core::Move(move); // Store a copy of the move
            ttEntry.score= storedScore;
            ttEntry.depth= depth;
            ttEntry.flag= TT_BETA;
            return beta; // Fail hard beta cutoff
        }
        if(evaluation > alpha) {
            alpha= evaluation;
            bestMoveThisNode= move;
        }
    }
    int storedScore= alpha;
    if(storedScore > MATE_VAL - 100) storedScore+= ply;
    else if(storedScore < -MATE_VAL + 100) storedScore-= ply;

    ttEntry.zobristHash= _board.zobristHash;
    delete ttEntry.bestMove;                            // Free old move before overwriting
    ttEntry.bestMove= new core::Move(bestMoveThisNode); // Store a copy of the move
    ttEntry.score= storedScore;
    ttEntry.depth= depth;
    ttEntry.flag= (alpha > originalAlpha) ? TT_EXACT : TT_ALPHA;

    return alpha;
}
int Bot::evaluate() {
    positionsEvaluated++;
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
int Bot::quiesce(int alpha, int beta, int ply) {
    // Generate legal moves to check for checkmate/stalemate
    auto moves= _moveGen.generateLegalMoves();

    // Check for checkmate or stalemate
    if(moves.empty()) {
        auto attackerColor= _board.activeColor == Color::WHITE ? Color::BLACK : Color::WHITE;
        auto kingPos= _board.activeColor == Color::WHITE ? _board.whiteKingPos : _board.blackKingPos;
        if(MoveGenerator::isSquareAttacked(_board, kingPos, attackerColor)) {
            checkMatesFound++;
            return -MATE_VAL + ply; // Checkmate, prefer faster mates
        } else {
            return 0; // Stalemate
        }
    }

    // 1. Stand Pat: Assumes we can just "stop" and not capture anything if our position is good
    int stand_pat= evaluate();
    if(stand_pat >= beta) return beta;
    if(alpha < stand_pat) alpha= stand_pat;

    // 2. Search only Captures
    orderMoves(moves, nullptr); // Order captures by MVV-LVA, no TT move in quiescence

    for(const auto& move: moves) {
        // FILTER: Only look at Captures and Promotions
        if(move.captured == core::Piece::NONE && move.promotion == core::Piece::NONE) continue;

        _board.makeMove(move);
        int score= -quiesce(-beta, -alpha, ply + 1);
        _board.undoMove();

        if(score >= beta) return beta;
        if(score > alpha) alpha= score;
    }
    return alpha;
}

std::pair<core::Move, int> Bot::getBestMove(int timeLimitMs) {
    checkMatesFound= 0;    // Reset checkmate counter for this search
    positionsEvaluated= 0; // Reset positions evaluated counter for this search
    auto startTime= std::chrono::high_resolution_clock::now();

    core::Move bestMove;
    int bestScore= -INF;
    int depthReached= 0;

    for(int depth= 1; depth <= 21; ++depth) { // Iterative deepening
        auto moves= _moveGen.generateLegalMoves();
        if(moves.empty()) break; // No moves available

        TTEntry& ttEntry= _tt[_board.zobristHash % _tt.size()];
        core::Move* ttMove= (ttEntry.zobristHash == _board.zobristHash) ? ttEntry.bestMove : nullptr;
        orderMoves(moves, ttMove); // Move ordering for better alpha-beta performance

        // Reset best for this depth - each depth should find its own best move
        core::Move bestMoveThisDepth;
        int bestScoreThisDepth= -INF;
        int alpha= -INF;

        for(const auto& move: moves) {
            _board.makeMove(move);
            int score= -search(depth - 1, 1, -INF, -alpha);
            _board.undoMove();

            if(score > bestScoreThisDepth) {
                bestScoreThisDepth= score;
                bestMoveThisDepth= move;
            }
            if(score > alpha) {
                alpha= score;
            }
        }

        // Update overall best from this completed depth
        bestMove= bestMoveThisDepth;
        bestScore= bestScoreThisDepth;
        depthReached= depth;

        auto currentTime= std::chrono::high_resolution_clock::now();
        int elapsedMs= std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count();
        if(elapsedMs >= timeLimitMs) {
            break; // Time limit reached
        }
    }
    std::cout << "info" << " depth " << depthReached << " score cp " << bestScore << " time " << timeLimitMs << " nodes " << positionsEvaluated << std::endl;
    return {bestMove, bestScore};
}
} // namespace talawachess
