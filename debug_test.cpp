#include "Board.hpp"
#include "MoveGenerator.hpp"
#include <iostream>
using namespace talawachess::core::board;
using namespace talawachess::core;

int main() {
    Board board;
    MoveGenerator gen(board);

    board.setFen("8/5pk1/1p2p3/1N2N3/PP1Pn1P1/R3P1nP/6K1/2r5 b - - 4 41");
    std::cout << "Starting position:" << std::endl;
    board.print();

    // Play c1g1
    std::string moves[]= {"c1g1", "g2g1", "g3e2"};
    for(const auto& mv: moves) {
        auto legal= gen.generateLegalMoves();
        bool found= false;
        std::cout << "\nLooking for " << mv << " in " << legal.size() << " legal moves" << std::endl;
        for(const auto& m: legal) {
            if(m.ToString() == mv) {
                std::cout << "Making " << mv << std::endl;
                board.makeMove(m);
                board.print();
                found= true;
                break;
            }
        }
        if(!found) {
            std::cout << "Move not found! Available: ";
            for(const auto& m: legal) std::cout << m.ToString() << " ";
            std::cout << std::endl;
            break;
        }
    }

    // Check white's responses
    std::cout << "\nWhite to move. Legal moves:" << std::endl;
    auto white= gen.generateLegalMoves();
    std::cout << "Count: " << white.size() << std::endl;
    for(const auto& m: white) std::cout << m.ToString() << " ";
    std::cout << std::endl;

    return 0;
}
