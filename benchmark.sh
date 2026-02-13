#!/bin/bash

# ==========================================
# CONFIGURATION
# ==========================================
BINARY="./build/talawachess.exe"
OUTPUT_FILE="benchmark_results.csv"
SEARCH_TIME=5000              # 5 seconds
LABEL=${1:-"Experiment"}      # Usage: ./benchmark.sh "MyLabel"

# 1. The FEN Strings
declare -a FENS=(
    "4r2k/1p3rbp/2p1N1pn/p3n3/P2NB3/1P4q1/4R1P1/B1Q2RK1 b - - 4 32"
    "1k6/8/8/8/8/5qP1/5P1P/5RKb b - - 0 1"
    "8/5pk1/1p2p3/1N2N3/PP1Pn1P1/R3P1nP/6K1/2r5 b - - 4 41"
    "r3k3/2p1bpp1/p1nqp1p1/1p6/6nB/P1NP3P/BPP2PP1/R2QR1K1 b q - 0 15"
    "r4b1k/1b3qNp/p5pP/2r1pN2/4P3/2pB1Q2/P1P5/KR4R1 b - - 1 29"
    "2r1k2B/1p1bn3/q3p2p/6pP/2P1N1Pn/b2Q4/P3B3/5R1K w - - 1 33"
)

# 2. The Human-Readable Names
declare -a NAMES=(
    "Mate_in_8"
    "Mate_in_1"
    "Mate_in_3"
    "Mate_in_4"
    "Queen_Sac_Mate_in_4"
    "Mate_in_12"
)

# 3. The Expected Best Moves (Must match indices above!)
declare -a EXPECTED=(
    "h6g4"
    "f3g2"
    "c1c2"
    "d6h2"
    "f7a2"
    "e4f6"
)

# ==========================================
# SETUP
# ==========================================

GREEN='\033[0;32m'
RED='\033[0;31m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

if [ ! -f "$BINARY" ]; then
    echo "Error: Engine not found at $BINARY"
    exit 1
fi

# Initialize CSV if it doesn't exist
if [ ! -f "$OUTPUT_FILE" ]; then
    echo "Timestamp,Label,Position_Name,Status,Depth,Nodes,Time_ms,NPS,BestMove,Expected" > "$OUTPUT_FILE"
fi

echo -e "${CYAN}Starting Benchmark: [${LABEL}]${NC}"
echo "--------------------------------------------------------"

# ==========================================
# MAIN LOOP
# ==========================================

for i in "${!FENS[@]}"; do
    fen="${FENS[$i]}"
    name="${NAMES[$i]}"
    expected="${EXPECTED[$i]}"
    
    echo -ne "Testing ${name}..."

    # Run Engine
    output=$(echo -e "position fen $fen\ngo movetime $SEARCH_TIME\nquit" | $BINARY)

    # Parse Results
    last_info=$(echo "$output" | grep "info depth" | tail -n 1)
    best_move=$(echo "$output" | grep "bestmove" | awk '{print $2}')
    
    # Metrics
    depth=$(echo "$last_info" | grep -o 'depth [0-9]*' | awk '{print $2}')
    nodes=$(echo "$last_info" | grep -o 'nodes [0-9]*' | awk '{print $2}')
    time_ms=$(echo "$last_info" | grep -o 'time [0-9]*' | awk '{print $2}')
    
    # Calculate NPS
    if [ -z "$time_ms" ] || [ "$time_ms" -eq 0 ]; then nps=0; else nps=$(( (nodes * 1000) / time_ms )); fi

    # Check Accuracy
    if [ "$best_move" == "$expected" ]; then
        status="PASS"
        color=$GREEN
    else
        status="FAIL"
        color=$RED
    fi

    # Save to CSV
    timestamp=$(date "+%Y-%m-%d %H:%M:%S")
    echo "$timestamp,$LABEL,$name,$status,$depth,$nodes,$time_ms,$nps,$best_move,$expected" >> "$OUTPUT_FILE"

    # Print to Console
    echo -e " ${color}${status}${NC}"
    if [ "$status" == "FAIL" ]; then
        echo -e "   -> Expected: $expected | Got: $best_move"
    fi
    echo -e "   -> Depth: $depth | NPS: $nps"
done

echo "--------------------------------------------------------"
echo "Benchmark Complete. Results saved to $OUTPUT_FILE"