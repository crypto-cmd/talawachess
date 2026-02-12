#!/bin/bash

# 1. Configuration
BINARY="./build/talawachess.exe"
FEN="4r2k/1p3rbp/2p1N1pn/p3n3/P2NB3/1P4q1/4R1P1/B1Q2RK1 b - - 4 32"

# 2. Check if engine exists
if [ ! -f "$BINARY" ]; then
    echo "Error: Engine not found at $BINARY"
    echo "Please run ./run.sh first to build it."
    exit 1
fi

echo "---------------------------------------"
echo "Testing FEN: $FEN"
echo "---------------------------------------"

# 3. Prepare the input commands
# We send 'position fen', then 'go', then 'quit' to ensure the engine exits.
input_commands="position fen $FEN\ngo\nquit\n"

# 4. Run the engine and time it
# We use 'printf' to pipe the commands into the engine.
# We pipe the engine's standard output to cat (so you see it), 
# but 'time' will print its stats to stderr.

# Note: "real" is the wall-clock time (what you care about).
time printf "$input_commands" | $BINARY

echo "---------------------------------------"
echo "Done."