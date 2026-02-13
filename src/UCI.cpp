#include "UCI.hpp"
#include "Coordinate.hpp"
#include "MoveGenerator.hpp"
#include <random>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/select.h>
#include <unistd.h>
#endif

namespace talawachess {

// Non-blocking check if input is available on stdin
static bool inputAvailable() {
#ifdef _WIN32
    static HANDLE hStdin= GetStdHandle(STD_INPUT_HANDLE);
    static bool isPipe= (GetFileType(hStdin) == FILE_TYPE_PIPE);
    if(isPipe) {
        DWORD bytesAvailable= 0;
        if(!PeekNamedPipe(hStdin, nullptr, 0, nullptr, &bytesAvailable, nullptr)) {
            return true; // Pipe broken (GUI closed), signal as input available to trigger EOF
        }
        return bytesAvailable > 0;
    } else {
        DWORD events= 0;
        if(!GetNumberOfConsoleInputEvents(hStdin, &events) || events < 2) return false;
        // Check if any of the events are key presses
        INPUT_RECORD record;
        DWORD read;
        while(events > 0) {
            PeekConsoleInput(hStdin, &record, 1, &read);
            if(record.EventType == KEY_EVENT && record.Event.KeyEvent.bKeyDown) return true;
            // Discard non-key events
            ReadConsoleInput(hStdin, &record, 1, &read);
            events--;
        }
        return false;
    }
#else
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    struct timeval tv= {0, 0};
    return select(1, &readfds, nullptr, nullptr, &tv) > 0;
#endif
}

// Buffer for pending input line
static std::string pendingInput;
static bool stopRequested= false;

// Check if stop command was received (called by bot during search)
static bool checkForStop() {
    if(stopRequested) return true;
    if(inputAvailable()) {
        std::string line;
        if(std::getline(std::cin, line)) {
            if(line == "stop" || line == "quit") {
                stopRequested= true;
                return true;
            }
            // Store other commands for later processing
            pendingInput= line;
        } else {
            // EOF - pipe closed (GUI exited), stop immediately
            stopRequested= true;
            return true;
        }
    }
    // Also check if stdin has hit EOF (pipe broken)
    if(std::cin.eof()) {
        stopRequested= true;
        return true;
    }
    return false;
}

void UCI::listen() {
    std::string line;
    std::string token;

    // Check for "startpos" initialization
    _bot.setFen(core::board::Board::STARTING_POS);

    // Set up input checker for the bot to poll during search
    _bot.setInputChecker(checkForStop);

    while(std::getline(std::cin, line)) {
        std::stringstream ss(line);
        ss >> token;

        if(token == "uci") {
            std::cout << "id name " << ENGINE_NAME << "\n";
            std::cout << "id author Orville\n";
            std::cout << "uciok" << std::endl;
        } else if(token == "isready") {
            std::cout << "readyok" << std::endl;
        } else if(token == "quit") {
            break;
        } else if(token == "stop") {
            // Stop is handled during search via polling, ignore here
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
            }

            ss >> token; // Expect "moves" or end of line
            if(token == "moves") {
                while(ss >> token) {
                    _bot.performMove(token);
                }
            }
        } else if(token == "go") {

            // _bot.getBoard().print(); // DEBUG: Print the board before thinking
            // "go" asks the engine to start thinking.
            // 1. Initialize variables with default values
            int wtime= 0;      // White time remaining (ms)
            int btime= 0;      // Black time remaining (ms)
            int winc= 0;       // White increment (ms)
            int binc= 0;       // Black increment (ms)
            int movestogo= 30; // Default moves to go if not specified
            int movetime= 0;   // Fixed move time (ms), if specified

            // 2. Parse the line to find time parameters
            bool infinite= false;
            int maxDepth= 0;
            std::string type;
            while(ss >> type) {
                if(type == "wtime") ss >> wtime;
                else if(type == "btime") ss >> btime;
                else if(type == "winc") ss >> winc;
                else if(type == "binc") ss >> binc;
                else if(type == "movestogo") ss >> movestogo;
                else if(type == "movetime") ss >> movetime;
                else if(type == "infinite") infinite= true;
                else if(type == "depth") ss >> maxDepth;
            }

            // 3. Determine "Our" time
            int myTime= (_bot.getBoard().activeColor == core::Piece::WHITE) ? wtime : btime;
            int myInc= (_bot.getBoard().activeColor == core::Piece::WHITE) ? winc : binc;

            // 4. Calculate allocated time
            int timeToThink= 0; // 0 means no time limit (infinite or depth-only)
            if(movetime > 0) {
                timeToThink= movetime;
            } else if(!infinite && maxDepth == 0 && myTime > 0) {
                // Time control: use fraction of remaining time + portion of increment
                timeToThink= (myTime / movestogo) + (myInc / 2);
                // Safety buffer: leave at least 50ms on the clock
                if(timeToThink >= myTime) timeToThink= myTime - 50;
                if(timeToThink < 10) timeToThink= 10; // Minimum time
            } else if(!infinite && maxDepth == 0) {
                // No time info and not infinite/depth - default to 5 seconds
                timeToThink= 5000;
            }

            // Reset stop flag before search
            stopRequested= false;

            // Run search (will poll for stop command internally)
            auto result= _bot.getBestMove(timeToThink, maxDepth);
            auto bestMove= result.first;
            std::cout << "bestmove " << bestMove.ToString() << std::endl;
        }
    }
}
} // namespace talawachess