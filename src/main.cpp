#include "Board.hpp"
#include "MoveGenerator.hpp"
#include "UCI.hpp"
#include <iostream>
#include <random>

using namespace talawachess::core;
using namespace talawachess::core::board;
int main() {
    talawachess::UCI uci;
    uci.listen(); // Start listening for GUI commands
    return 0;
}