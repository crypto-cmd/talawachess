#!/bin/bash

# 1. Configuration
BINARY="./build/talawachess.exe"
FEN="8/5pk1/1p2p3/1N2N3/PP1Pn1P1/R3P1nP/6K1/2r5 b - - 4 41"

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