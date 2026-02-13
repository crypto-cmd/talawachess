#include "Evaluator.hpp"
#include "Board.hpp"
#include "Piece.hpp"
namespace talawachess::bot::evaluator {
int GetStaticPositionalPieceValue(core::Piece::Piece piece, int squareIndex) {
    if(piece == core::Piece::NONE) return 0;
    auto pieceType= core::Piece::GetPieceType(piece);
    auto pieceColor= core::Piece::GetColor(piece);

    int trueSquareIndex= pieceColor == core::Piece::WHITE ? squareIndex : squareIndex ^ 56; // Flip the index for black pieces to use the same table
    int pstValue= PieceSquareTables[pieceType][trueSquareIndex];
    return pstValue;
}
int evaluate(const talawachess::core::board::Board& _board) {
    using namespace talawachess::core;
    int score= 0;

    for(int i= 0; i < 64; ++i) {
        auto piece= _board.squares[i];
        if(piece == Piece::NONE) continue;

        auto pieceType= Piece::GetPieceType(piece);
        auto pieceValue= PieceValues[pieceType];

        int positionalPieceValue= GetStaticPositionalPieceValue(piece, i);
        int value= pieceValue + positionalPieceValue;
        int colorMultiplier= Piece::IsColor(piece, Piece::WHITE) ? 1 : -1;
        score+= colorMultiplier * value;
    }
    auto perspectiveMultiplier= _board.activeColor == Piece::WHITE ? 1 : -1;
    return score * perspectiveMultiplier;
};
} // namespace talawachess::bot::evaluator