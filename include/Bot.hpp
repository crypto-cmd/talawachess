#pragma once

#include "Board.hpp"
#include "MoveGenerator.hpp"
#include <random>
namespace talawachess {
class Bot {
  private:
    core::board::Board _board;
    core::board::MoveGenerator _moveGen;

  public:
    Bot()= default;

    inline void setFen(const std::string& fen) {
        _board.setFen(fen);
    }

    inline void performMove(const std::string& moveStr) {
        auto moves= _moveGen.generateLegalMoves(_board);
        for(const auto& move: moves) {
            if(moveStr == move.ToString()) {
                _board.makeMove(move);
                return;
            }
        }

        std::cerr << "Invalid move string: " << moveStr << std::endl;
    }
    core::Move getBestMove(int timeLimitMs= 1000) {
        auto moves= _moveGen.generateLegalMoves(_board);
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, moves.size() - 1);

        const auto& bestMove= moves[dis(gen)];
        return bestMove;
    }
    const core::board::Board& getBoard() const {
        return _board;
    }
};
} // namespace talawachess
