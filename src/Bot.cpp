#include "Bot.hpp"
#include "Board.hpp"
#include "Evaluator.hpp"
#include <chrono>
namespace talawachess {
using namespace core::board;
using namespace core::Piece;
using namespace core;

Bot::Bot(): _board(),
            _moveGen(_board) {
    // Initialize Transposition Table with default size (e.g., 512 MB)
    resizeTT(512);
}
void Bot::resizeTT(size_t sizeInMB) {
    // Clear old entries and free their moves first

    size_t numEntries= (sizeInMB * 1024 * 1024) / sizeof(TTEntry);
    _tt.clear();
    _tt.resize(numEntries);
    // Initialize all entries properly
    for(auto& entry: _tt) {
        entry.bestMove= core::Move(); // Initialize to default move
        entry.zobristHash= 0;
        entry.depth= -1;
        entry.flag= TT_EXACT;
        entry.score= -INF;
    }
}

void Bot::clearTT() {
    for(auto& entry: _tt) {
        entry.bestMove= core::Move(); // Initialize to default move
        entry.zobristHash= 0;
        entry.depth= -1;
        entry.flag= TT_EXACT;
        entry.score= -INF;
    }
}

void Bot::clearKillers() {
    for(int i= 0; i < MAX_PLY; ++i) {
        _killers[i][0]= core::Move();
        _killers[i][1]= core::Move();
    }
}

void Bot::updateKillers(const core::Move& move, int ply) {
    if(ply >= MAX_PLY) return;
    // Don't store captures or promotions as killers (they're already ordered high)
    if(move.captured != core::Piece::NONE) return;
    if(move.promotion != core::Piece::NONE) return;
    // Don't store if it's already the first killer
    if(_killers[ply][0].from == move.from && _killers[ply][0].to == move.to) return;
    // Shift killer 0 to killer 1, store new killer in slot 0
    _killers[ply][1]= _killers[ply][0];
    _killers[ply][0]= move;
}
void Bot::setFen(const std::string& fen) {
    _board.setFen(fen);
}
void Bot::performMove(const std::string& moveStr) {
    // Convert moveStr (e.g., "e2e4") to a Move object
    MoveList moves;
    _moveGen.generateMoves(moves);
    for(const auto& move: moves) {
        if(move.ToString() == moveStr) {
            _board.makeMove(move);

            // Determine legality after making the move (e.g., not leaving king in check)
            if(!MoveGenerator::IsLegalPosition(_board)) {
                _board.undoMove(); // Illegal move, undo it
            }
            return;
        }
    }
    throw std::invalid_argument("Invalid move string: " + moveStr);
}
void Bot::orderMoves(MoveList& moves, const core::Move* ttMove, int ply) const {
    using namespace bot;
    for(auto& move: moves) {
        move.score= 0;
        // 1. Prioritize Transposition Table Move
        if(ttMove != nullptr && move.from == ttMove->from && move.to == ttMove->to) {
            move.score= 2000000; // Highest priority
            continue;
        }
        // 2. Prioritize Captures using MVV-LVA (1,000,000 - 1,900,000)
        if(move.captured != core::Piece::NONE) {
            int victimType= core::Piece::GetPieceType(move.captured);
            int attackerType= core::Piece::GetPieceType(move.movedPiece);
            // MVV-LVA: higher victim value and lower attacker value = better
            move.score= 1000000 + (evaluator::PieceValues[victimType] * 100) - evaluator::PieceValues[attackerType];
        }
        // 3. Prioritize Promotions (also tactical, score in capture range)
        else if(move.promotion != core::Piece::NONE) {
            int promoType= core::Piece::GetPieceType(move.promotion);
            move.score= 1000000 + evaluator::PieceValues[promoType];
        }
        // 4. Killer Moves (quiet moves that caused cutoffs at this ply)
        // Only apply in main search (ply >= 0), not in quiescence (ply = -1)
        else if(ply >= 0 && ply < MAX_PLY) {
            if(_killers[ply][0].from == move.from && _killers[ply][0].to == move.to) {
                move.score= 900000; // First killer - below all captures
            } else if(_killers[ply][1].from == move.from && _killers[ply][1].to == move.to) {
                move.score= 800000; // Second killer
            }
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

    // 1. Check for time limit every 512 nodes (more responsive to stop command)
    if((positionsEvaluated & 511) == 0) {
        checkTime();
    }

    // 2. Don't search if we've been signaled to stop
    if(_stopSearch) return 0;

    // 3. Prevent search explosions
    const int MAX_PLY= 100; // Define a safe maximum depth
    if(ply >= MAX_PLY) {
        // Evaluate immediately to break the infinite loop
        return bot::evaluator::evaluate(_board); // Or call quiesce(alpha, beta, ply);
    }
    // 4. CRITICAL: Draw Detection (Repetition & 50-Move Rule)
    if(ply > 0) {
        if(_board.halfMoveClock >= 100) return 0; // 50-move rule

        // 1-Fold Repetition Detection
        int limit= _board.game_history.size() - _board.halfMoveClock;
        if(limit < 0) limit= 0;

        // Step back by 2 to check positions where it was the same side's turn to move
        for(int i= _board.game_history.size() - 2; i >= limit; i-= 2) {
            if(_board.game_history[i].zobristHash == _board.zobristHash) {
                return 0; // Instant cutoff: We are in a looping check sequence
            }
        }
    }

    TTEntry& ttEntry= _tt[_board.zobristHash % _tt.size()];
    bool ttHit= (ttEntry.zobristHash == _board.zobristHash);
    core::Move* ttBestMove= nullptr;
    if(ttHit) {
        ttBestMove= &ttEntry.bestMove;
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

    // Null Move Pruning
    // Skip when: at root, in check, or beta is a mate score
    if(depth >= 3 && ply > 0 && beta < MATE_VAL - 100 && beta > -MATE_VAL + 100) {
        // Verify the side to move is not in check
        Coordinate ourKingPos= (_board.activeColor == Color::WHITE) ? _board.whiteKingPos : _board.blackKingPos;
        Color opponentColor= (_board.activeColor == Color::WHITE) ? Color::BLACK : Color::WHITE;
        if(!MoveGenerator::isSquareAttacked(_board, ourKingPos, opponentColor)) {
            int R= 2 + depth / 6;
            _board.makeNullMove();
            int nullScore= -search(depth - 1 - R, ply + 1, -beta, -beta + 1);
            _board.undoNullMove();

            if(_stopSearch) return 0;
            if(nullScore >= beta) {
                return beta;
            }
        }
    }

    MoveList moveList;
    _moveGen.generateMoves(moveList);

    orderMoves(moveList, ttBestMove, ply); // Move ordering for better alpha-beta performance
    int originalAlpha= alpha;
    core::Move bestMoveThisNode;

    Coordinate myKingPos= (_board.activeColor == Piece::WHITE) ? _board.whiteKingPos : _board.blackKingPos;
    Piece::Color oppColor= (_board.activeColor == Piece::WHITE) ? Piece::BLACK : Piece::WHITE;
    bool inCheck= MoveGenerator::isSquareAttacked(_board, myKingPos, oppColor);

    int legalMoveCount= 0; // Count of legal moves for statistics
    for(int i= 0; i < moveList.count; ++i) {
        auto& move= moveList.moves[i];
        auto side= _board.activeColor;
        _board.makeMove(move);

        // Deferred Legality Check:
        if(!MoveGenerator::IsLegalPosition(_board)) {
            _board.undoMove(); // Illegal move, undo it
            continue;          // Skip to next move
        }
        legalMoveCount++;

        // Check extension: if we give check, extend depth by 1
        int extension= 0;
        Coordinate oppKingPos= (_board.activeColor == Color::WHITE) ? _board.whiteKingPos : _board.blackKingPos;
        Color ourColor= (_board.activeColor == Color::WHITE) ? Color::BLACK : Color::WHITE;
        if(MoveGenerator::isSquareAttacked(_board, oppKingPos, ourColor)) {
            // POOR MAN'S SEE: Is the piece giving check sitting on a square attacked by the opponent?
            // If the opponent can just capture the checking piece, it is a spite check. Do NOT extend.
            if(!MoveGenerator::isSquareAttacked(_board, move.to, _board.activeColor)) {
                extension= 1;
            }
        }

        // Late Move Reduction:
        bool isKiller= false;
        if(ply >= 0 && ply < MAX_PLY) {
            if(_killers[ply][0].from == move.from && _killers[ply][0].to == move.to) isKiller= true;
            if(_killers[ply][1].from == move.from && _killers[ply][1].to == move.to) isKiller= true;
        }

        int reduction= 0;
        if(depth >= 3 && i >= 3 && !inCheck && !isKiller && extension == 0 && move.captured == Piece::NONE && move.promotion == Piece::NONE) {
            // Base reduction of 1, plus scaling based on depth and move index
            reduction= 1 + (depth / 4) + (i / 8);

            // Safety cap: Never reduce the depth to 0 or below, always search at least depth 1
            if(reduction >= depth) {
                reduction= depth - 1;
            }
        }

        int evaluation= -search(depth - 1 + extension - reduction, ply + 1, -beta, -alpha);

        // If we reduced and the move looks better than alpha, research at full depth (Late Move Reduction with Research)
        if(evaluation > alpha && reduction > 0) {
            evaluation= -search(depth - 1 + extension, ply + 1, -beta, -alpha);
        }
        _board.undoMove();

        if(_stopSearch) return 0; // If we were signaled to stop during the search, return immediately
        if(evaluation >= beta) {
            // Update killer moves for quiet moves that cause beta cutoff
            updateKillers(move, ply);

            int storedScore= evaluation;
            if(storedScore > MATE_VAL - 100) storedScore+= ply;
            else if(storedScore < -MATE_VAL + 100) storedScore-= ply;

            if(ttEntry.zobristHash != _board.zobristHash || depth >= ttEntry.depth) {
                ttEntry.zobristHash= _board.zobristHash;
                ttEntry.bestMove= move;
                ttEntry.score= storedScore;
                ttEntry.depth= depth;
                ttEntry.flag= TT_BETA;
            }
            return beta; // Fail hard beta cutoff
        }
        if(evaluation > alpha) {
            alpha= evaluation;
            bestMoveThisNode= move;
        }
    }

    if(legalMoveCount == 0) {
        // No legal moves found (all moves were illegal due to checks)
        auto attackerColor= _board.activeColor == Color::WHITE ? Color::BLACK : Color::WHITE;
        auto kingPos= _board.activeColor == Color::WHITE ? _board.whiteKingPos : _board.blackKingPos;
        if(MoveGenerator::isSquareAttacked(_board, kingPos, attackerColor)) {
            checkMatesFound++;
            return -MATE_VAL + ply; // Checkmate, prefer faster mates
        } else {
            return 0; // Stalemate
        }
    }
    int storedScore= alpha;
    if(storedScore > MATE_VAL - 100) storedScore+= ply;
    else if(storedScore < -MATE_VAL + 100) storedScore-= ply;

    if(ttEntry.zobristHash != _board.zobristHash || depth >= ttEntry.depth) {

        // CRITICAL: Preserve the old best move if we failed low (Alpha node)
        if(alpha > originalAlpha) {
            ttEntry.bestMove= bestMoveThisNode; // Exact node: we found a new best move
        } else if(ttEntry.zobristHash != _board.zobristHash) {
            ttEntry.bestMove= core::Move(); // New position: clear the move
        }
        // If it was the same position but failed low, we leave ttEntry.bestMove untouched!

        ttEntry.zobristHash= _board.zobristHash;
        ttEntry.score= storedScore;
        ttEntry.depth= depth;
        ttEntry.flag= (alpha > originalAlpha) ? TT_EXACT : TT_ALPHA;
    }

    return alpha;
}

int Bot::quiesce(int alpha, int beta, int ply) {
    // Generate legal moves to check for checkmate/stalemate
    MoveList moves;
    _moveGen.generateMoves(moves);

    // 1. Stand Pat: Assumes we can just "stop" and not capture anything if our position is good
    int stand_pat= bot::evaluator::evaluate(_board);
    positionsEvaluated++;
    if(stand_pat >= beta) return beta;
    if(alpha < stand_pat) alpha= stand_pat;

    // 2. Search only Captures and Promotions
    orderMoves(moves, nullptr, -1); // Order captures by MVV-LVA, no killers in quiescence

    int legalMoveCount= 0; // Count of legal moves for statistics
    for(const auto& move: moves) {
        // FILTER: Only look at Captures and Promotions
        if(move.captured == core::Piece::NONE && move.promotion == core::Piece::NONE) continue;

        auto side= _board.activeColor;
        _board.makeMove(move);

        // Deferred Legality Check:
        if(!MoveGenerator::IsLegalPosition(_board)) {
            _board.undoMove(); // Illegal move, undo it
            continue;          // Skip to next move
        }
        legalMoveCount++;
        int score= -quiesce(-beta, -alpha, ply + 1);
        _board.undoMove();

        if(score >= beta) return beta;
        if(score > alpha) alpha= score;
    }
    if(legalMoveCount == 0) {
        // No legal moves found (all moves were illegal due to checks)
        auto attackerColor= _board.activeColor == Color::WHITE ? Color::BLACK : Color::WHITE;
        auto kingPos= _board.activeColor == Color::WHITE ? _board.whiteKingPos : _board.blackKingPos;
        if(MoveGenerator::isSquareAttacked(_board, kingPos, attackerColor)) {
            checkMatesFound++;
            return -MATE_VAL + ply; // Checkmate, prefer faster mates
        } else {
            return 0; // Stalemate
        }
    }
    return alpha;
}

std::pair<core::Move, int> Bot::getBestMove(int timeLimitMs, int maxDepth) {
    checkMatesFound= 0;    // Reset checkmate counter for this search
    positionsEvaluated= 0; // Reset positions evaluated counter for this search
    clearKillers();        // Clear killer moves for new search

    startTimer(timeLimitMs); // Start the timer as we are about to begin searching

    // If no depth limit specified, use a high default
    int depthLimit= (maxDepth > 0) ? maxDepth : 64;

    core::Move bestMove;
    int bestScore= -INF;
    int depthReached= 0;

    for(int depth= 1; depth <= depthLimit; ++depth) { // Iterative deepening
        MoveList moves;
        _moveGen.generateMoves(moves);

        TTEntry& ttEntry= _tt[_board.zobristHash % _tt.size()];
        core::Move* ttMove= (ttEntry.zobristHash == _board.zobristHash) ? &ttEntry.bestMove : nullptr;
        orderMoves(moves, ttMove, 0); // Move ordering for better alpha-beta performance

        // Reset best for this depth - each depth should find its own best move
        core::Move bestMoveThisDepth;
        int bestScoreThisDepth= -INF;
        int alpha= -INF;
        int legalMoveCount= 0; // Count of legal moves for statistics
        for(const auto& move: moves) {
            _board.makeMove(move);

            // Deferred Legality Check:
            if(!MoveGenerator::IsLegalPosition(_board)) {
                _board.undoMove(); // Illegal move, undo it
                continue;          // Skip to next move
            }
            legalMoveCount++;
            int score= -search(depth - 1, 1, -INF, -alpha);
            _board.undoMove();

            if(_stopSearch) break; // If we were signaled to stop during the search, break out of move loop

            if(score > bestScoreThisDepth) {
                bestScoreThisDepth= score;
                bestMoveThisDepth= move;
            }
            if(score > alpha) {
                alpha= score;
            }
        }

        if(legalMoveCount == 0) {

            // We are at the root and found no legal moves - this means the position is either checkmate or stalemate and we should return and say error because there is no best move
            std::cout << "info" << " depth " << depth << " score " << "cp 0" << " time " << getElapsedTimeMs() << " nodes " << positionsEvaluated << " nps 0 pv" << std::endl;
            return {core::Move(), 0};
        }

        if(_stopSearch) break; // If we were signaled to stop during the search, break out of depth loop

        // Update overall best from this completed depth
        bestMove= bestMoveThisDepth;
        bestScore= bestScoreThisDepth;
        depthReached= depth;

        // printing

        auto elapsedMs= getElapsedTimeMs();
        // Output search info for GUI
        std::string pvLine= extractPV(bestMove, depthReached);
        std::string scoreStr;
        // UCI scores are always from side-to-move's perspective
        if(bestScore > MATE_VAL - 100) {
            // Side to move is delivering mate
            int pliesToMate= MATE_VAL - bestScore;
            int movesToMate= (pliesToMate + 1) / 2;
            scoreStr= "mate " + std::to_string(movesToMate);
        } else if(bestScore < -MATE_VAL + 100) {
            // Side to move is getting mated
            int pliesToMate= bestScore + MATE_VAL;
            int movesToMate= (pliesToMate + 1) / 2;
            scoreStr= "mate -" + std::to_string(movesToMate);
        } else {
            scoreStr= "cp " + std::to_string(bestScore);
        }

        uint64_t nps= elapsedMs > 0 ? (positionsEvaluated * 1000ULL / elapsedMs) : positionsEvaluated;
        std::cout << "info" << " depth " << depthReached << " score " << scoreStr << " time " << elapsedMs << " nodes " << positionsEvaluated << " nps " << nps << " pv" << pvLine << std::endl;
    }
    return {bestMove, bestScore};
}

std::string Bot::extractPV(const core::Move& bestMove, int depth) {
    std::string pv= " " + bestMove.ToString();
    std::vector<core::Move> movesMade;

    // Make the first move (the one we already found)
    _board.makeMove(bestMove);
    movesMade.push_back(bestMove);

    // Follow the TT for the rest of the PV
    for(int i= 1; i < depth; ++i) {
        TTEntry& ttEntry= _tt[_board.zobristHash % _tt.size()];
        if(ttEntry.zobristHash != _board.zobristHash) break;

        core::Move& move= ttEntry.bestMove;
        if(move.from == move.to) break; // Invalid move

        // Verify move is legal
        auto attackerColor= _board.activeColor == core::Piece::WHITE ? core::Piece::BLACK : core::Piece::WHITE;
        auto kingPos= _board.activeColor == core::Piece::WHITE ? _board.whiteKingPos : _board.blackKingPos;
        if(MoveGenerator::isSquareAttacked(_board, kingPos, attackerColor)) {
            break; // Current position is in check, so any move in TT is suspect, break PV extraction
        }

        pv+= " " + move.ToString();
        _board.makeMove(move);
        movesMade.push_back(move);
    }

    // Undo all moves to restore board state
    for(int i= movesMade.size() - 1; i >= 0; --i) {
        _board.undoMove();
    }

    return pv;
}
} // namespace talawachess
