#include "UCI.hpp"
#include "Coordinate.hpp"
#include "MoveGenerator.hpp"
#include <random>
#include <sstream>

namespace talawachess {

void UCI::listen() {
    std::string line;
    std::string token;

    // Check for "startpos" initialization
    _bot.setFen(core::board::Board::STARTING_POS);

    while(std::getline(std::cin, line)) {
        std::stringstream ss(line);
        ss >> token;

        if(token == "uci") {
            std::cout << "id name " << ENGINE_NAME << "\n";
            std::cout << "id author You\n";
            std::cout << "uciok" << std::endl;
        } else if(token == "isready") {
            std::cout << "readyok" << std::endl;
        } else if(token == "quit") {
            break;
        } else if(token == "position") {
            // Format: "position startpos moves e2e4 e7e5 ..."
            std::string posType;
            ss >> posType;
            if(posType == "startpos") {
                _bot.setFen(core::board::Board::STARTING_POS);
            } else if(posType == "fen") {
                // FEN strings in UCI are 6 parts separated by spaces.
                // We must stitch them back together because 'ss >>' splits by space.
                std::string fenPart, fen;
                for(int i= 0; i < 6; ++i) {
                    if(ss >> fenPart) {
                        fen+= (i > 0 ? " " : "") + fenPart;
                    }
                }
                _bot.setFen(fen);
                ss >> token; // Attempt to read "moves"
            }

            ss >> token; // Expect "moves" or end of line
            if(token == "moves") {
                while(ss >> token) {
                    _bot.performMove(token); // Apply the move to the board
                }
            }
        } else if(token == "go") {

            _bot.getBoard().print(); // DEBUG: Print the board before thinking
            // "go" asks the engine to start thinking.
            // 1. Initialize variables with default values
            int wtime= 0;      // White time remaining (ms)
            int btime= 0;      // Black time remaining (ms)
            int winc= 0;       // White increment (ms)
            int binc= 0;       // Black increment (ms)
            int movestogo= 30; // Default moves to go if not specified

            // 2. Parse the line to find time parameters
            std::string type;
            while(ss >> type) {
                if(type == "wtime") ss >> wtime;
                else if(type == "btime") ss >> btime;
                else if(type == "winc") ss >> winc;
                else if(type == "binc") ss >> binc;
                else if(type == "movestogo") ss >> movestogo;
            }

            // 3. Determine "Our" time
            int myTime= (_bot.getBoard().activeColor == core::Piece::WHITE) ? wtime : btime;
            int myInc= (_bot.getBoard().activeColor == core::Piece::WHITE) ? winc : binc;

            // 4. Calculate allocated time
            // Simple formula: Use 1/30th of remaining time + increment
            // (Safety: If we have very little time, move nearly instantly)
            int timeToThink= 0;

            if(myTime > 0) {
                timeToThink= (myTime / movestogo) + (myInc / 2);

                // Safety buffer: leave at least 50ms on the clock
                if(timeToThink >= myTime) timeToThink= myTime - 50;
                if(timeToThink < 0) timeToThink= 10; // Minimum snap move
            }
            // DEBUG: Tell the GUI how much time we are using
            std::cout << "info string Time Left: " << myTime << "ms. Thinking for: " << timeToThink << "ms." << std::endl;

            auto move= _bot.getBestMove(timeToThink);
            std::cout << "bestmove " << move.ToString() << std::endl;


        }
    }
}
} // namespace talawachess