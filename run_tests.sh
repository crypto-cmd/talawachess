#!/bin/bash

# Configuration
BINARY="./build/talawachess.exe"
TEST_FILE="tests.txt"

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Check if binary exists
if [ ! -f "$BINARY" ]; then
    echo -e "${RED}Error: Engine not found at $BINARY${NC}"
    exit 1
fi

echo "=========================================================="
echo "Running Test Suite..."
echo "=========================================================="

# Read the file line by line
# We use '|' as the separator (IFS)
while IFS='|' read -r fen expected_move_raw; do
    # 1. Clean up inputs (trim whitespace/comments)
    fen=$(echo "$fen" | xargs) 
    expected_move=$(echo "$expected_move_raw" | xargs)

    # Skip empty lines or comments
    if [[ -z "$fen" || "$fen" == \#* ]]; then
        continue
    fi

    echo -n "Testing: $expected_move ... "

    # 2. Prepare Engine Input
    # We ask the engine to go, then quit.
    input="position fen $fen\ngo movetime 10000 \nquit\n"

    # 3. Run Engine and Capture Output + Time
    # We use date +%s%N to get nanoseconds for precision
    start_time=$(date +%s%N)
    
    # Run the engine
    engine_output=$(printf "$input" | $BINARY)
    
    end_time=$(date +%s%N)
    
    # Calculate duration in milliseconds
    duration=$(( (end_time - start_time) / 1000000 ))
    # 4. Print info line (optional, can be commented out)
    echo -e "   Engine Output:\n$engine_output"
    # 5. Extract 'bestmove'
    # Grep finds the line, awk gets the second word (e.g., "bestmove e2e4")
    engine_move=$(echo "$engine_output" | grep "bestmove" | awk '{print $2}')

    # 6. Compare Results
    if [ "$engine_move" == "$expected_move" ]; then
        echo -e "[ ${GREEN}PASS${NC} ] Time: ${duration}ms"
    else
        echo -e "[ ${RED}FAIL${NC} ]"
        echo -e "   Expected: $expected_move"
        echo -e "   Got:      $engine_move"
        echo -e "   Time:     ${duration}ms"
        # Optional: Uncomment to see full FEN on fail
        # echo -e "   FEN:      $fen" 
    fi

done < "$TEST_FILE"

echo "=========================================================="
echo "Done."