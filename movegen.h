#pragma once
#include "board.h"
#include "move.h"

class MoveGenerator {
public:
    MoveGenerator();

    void generateLegalMoves(const Position& pos, MoveList& list);
    void generatePseudoLegal(const Position& pos, MoveList& list);
    bool isInCheck(const Position& pos, Color c) const;

private:
    Bitboard knightAttacks[64];
    Bitboard kingAttacks[64];
    Bitboard pawnAttacks[2][64];

    void initKnightAttacks();
    void initKingAttacks();
    void initPawnAttacks();

    Bitboard bishopAttacks(Square sq, Bitboard occupied) const;
    Bitboard rookAttacks(Square sq, Bitboard occupied) const;
    Bitboard queenAttacks(Square sq, Bitboard occupied) const;

    void generatePawnMoves(const Position& pos, MoveList& list);
    void generateKnightMoves(const Position& pos, MoveList& list);
    void generateBishopMoves(const Position& pos, MoveList& list);
    void generateRookMoves(const Position& pos, MoveList& list);
    void generateQueenMoves(const Position& pos, MoveList& list);
    void generateKingMoves(const Position& pos, MoveList& list);
    void generateCastlingMoves(const Position& pos, MoveList& list);

    bool isSquareAttacked(const Position& pos, Square sq, Color attacker) const;

    struct UndoInfo {
        Piece  captured;
        int    castlingRights;
        Square enPassantSquare;
        int    halfmoveClock;
    };
    UndoInfo makeMove(Position& pos, Move m) const;
    void     unmakeMove(Position& pos, Move m, const UndoInfo& undo) const;
};