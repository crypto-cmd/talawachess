#pragma once
#include <stdexcept>
#include <string>

namespace talawachess::core::board {
struct Coordinate {
    int file; // 0-7 for a-h
    int rank; // 0-7 for 1-8

    Coordinate(): file(-1), rank(-1) {}
    Coordinate(int file, int rank): file(file), rank(rank) {}
    Coordinate(int square): file(square % 8), rank(square / 8) {}
    Coordinate(const char* alg): file(alg[0] - 'a'), rank(alg[1] - '1') {}
    static Coordinate FromAlgebraic(const char* alg_) {
        std::string alg(alg_);

        if(alg.length() != 2) {
            throw std::invalid_argument("Invalid algebraic notation");
        }
        int file= alg[0] - 'a';
        int rank= alg[1] - '1';
        return Coordinate(file, rank);
    }
    std::string toAlgebraic() const {
        if(!IsValid()) {
            throw std::invalid_argument("Invalid coordinate");
        }
        char fileChar= 'a' + file;
        char rankChar= '1' + rank;
        return std::string() + fileChar + rankChar;
    }
    bool isLightSquare() const {
        return (file + rank) % 2 == 0;
    }

    int ToIndex() const {
        return rank * 8 + file;
    }
    bool IsValid() const {
        return file >= 0 && file < 8 && rank >= 0 && rank < 8;
    }

    // Operators
    bool operator==(const Coordinate& other) const {
        return file == other.file && rank == other.rank;
    }
    Coordinate operator+(const Coordinate& other) const {
        return Coordinate(file + other.file, rank + other.rank);
    }
    Coordinate operator-(const Coordinate& other) const {
        return Coordinate(file - other.file, rank - other.rank);
    }
    Coordinate& operator=(const Coordinate& other) {
        file= other.file;
        rank= other.rank;
        return *this;
    }
};

} // namespace talawachess::core::board
