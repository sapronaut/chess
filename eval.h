#pragma once
#include "board.h"

// ============================================================
//  EVALUATION
//  Returns a score in centipawns from the perspective of
//  the SIDE TO MOVE (positive = good for us, negative = bad).
//  100 centipawns = 1 pawn.
//
//  Two components:
//    1. Material: how many pieces does each side have?
//    2. Piece-square tables (PST): bonuses for good squares
//       e.g. knight in the center > knight on the rim
// ============================================================

// ---- Material values (in centipawns) ----
constexpr int PAWN_VALUE   = 100;
constexpr int KNIGHT_VALUE = 320;
constexpr int BISHOP_VALUE = 330;
constexpr int ROOK_VALUE   = 500;
constexpr int QUEEN_VALUE  = 900;
constexpr int KING_VALUE   = 20000;  // effectively infinite

// ---- Piece-Square Tables ----
// 64 values, one per square, from WHITE's perspective (a1=index 0).
// For black pieces, we mirror the table (index 63 - sq).
// Values are BONUSES added on top of material value.

// Pawns: want them advanced and in the center
constexpr int PST_PAWN[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
     5,  5, 10, 25, 25, 10,  5,  5,
     0,  0,  0, 20, 20,  0,  0,  0,
     5, -5,-10,  0,  0,-10, -5,  5,
     5, 10, 10,-20,-20, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0
};

// Knights: love the center, hate the rim ("a knight on the rim is dim")
constexpr int PST_KNIGHT[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
};

// Bishops: like open diagonals, avoid corners
constexpr int PST_BISHOP[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20
};

// Rooks: love open files and the 7th rank
constexpr int PST_ROOK[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
     5, 10, 10, 10, 10, 10, 10,  5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     0,  0,  0,  5,  5,  0,  0,  0
};

// Queens: mobile, avoid early development to edge
constexpr int PST_QUEEN[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
     -5,  0,  5,  5,  5,  5,  0, -5,
      0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
};

// King middlegame: hide behind pawns, castled is good
constexpr int PST_KING_MG[64] = {
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
     20, 20,  0,  0,  0,  0, 20, 20,
     20, 30, 10,  0,  0, 10, 30, 20
};

// ============================================================
//  evaluate()
//  The main evaluation function.
//  Returns score in centipawns from side-to-move's perspective.
// ============================================================
inline int evaluate(const Position& pos) {
    int score = 0;

    constexpr int materialValue[6] = {
        PAWN_VALUE, KNIGHT_VALUE, BISHOP_VALUE,
        ROOK_VALUE, QUEEN_VALUE,  KING_VALUE
    };

    constexpr const int* pst[6] = {
        PST_PAWN, PST_KNIGHT, PST_BISHOP,
        PST_ROOK, PST_QUEEN,  PST_KING_MG
    };

    for (int pt = 0; pt < 6; pt++) {
        // White pieces
        // PST index 0 = a8 (top-left), so we flip the rank
        Bitboard wb = pos.pieces[WHITE][pt];
        while (wb) {
            Square sq = popLSB(wb);
            int pstIdx = (7 - sq / 8) * 8 + (sq % 8);
            score += materialValue[pt] + pst[pt][pstIdx];
        }

        // Black pieces
        // Black sees the board mirrored, so rank is NOT flipped
        // but we subtract from score
        Bitboard bb = pos.pieces[BLACK][pt];
        while (bb) {
            Square sq = popLSB(bb);
            int pstIdx = (sq / 8) * 8 + (sq % 8);
            score -= materialValue[pt] + pst[pt][pstIdx];
        }
    }

    return (pos.sideToMove == WHITE) ? score : -score;
}