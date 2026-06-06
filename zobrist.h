#pragma once
#include "board.h"
#include <random>

// ============================================================
//  ZOBRIST HASHING
//
//  Every unique chess position gets a unique 64-bit hash.
//  Used by the transposition table to cache search results.
//
//  How it works:
//  - At startup, generate a random 64-bit number for every
//    (piece, square) combination - 12 pieces x 64 squares
//  - Also random numbers for: side to move, castling rights,
//    en passant file
//  - Hash of a position = XOR of all active random numbers
//
//  Why XOR?
//  - Making a move = XOR out the piece's old square,
//    XOR in the new square. O(1) incremental update.
//  - Reversible: XOR the same value twice = back to original
// ============================================================

struct Zobrist {
    // Random number for each piece on each square
    // pieceSquare[piece][square]
    uint64_t pieceSquare[12][64];

    // Random number for side to move (XOR in when black to move)
    uint64_t sideToMove;

    // Random number for each castling right combination (16 possibilities)
    uint64_t castling[16];

    // Random number for en passant file (0-7)
    uint64_t enPassant[8];

    // Initialize all random numbers
    Zobrist() {
        std::mt19937_64 rng(1234567890ULL); // fixed seed = reproducible
        std::uniform_int_distribution<uint64_t> dist;

        for (int p = 0; p < 12; p++)
            for (int sq = 0; sq < 64; sq++)
                pieceSquare[p][sq] = dist(rng);

        sideToMove = dist(rng);

        for (int c = 0; c < 16; c++)
            castling[c] = dist(rng);

        for (int f = 0; f < 8; f++)
            enPassant[f] = dist(rng);
    }

    // Compute hash from scratch for a position
    uint64_t compute(const Position& pos) const {
        uint64_t hash = 0;

        for (int sq = 0; sq < 64; sq++) {
            Piece pc = pos.mailbox[sq];
            if (pc != NO_PIECE)
                hash ^= pieceSquare[pc][sq];
        }

        if (pos.sideToMove == BLACK)
            hash ^= sideToMove;

        hash ^= castling[pos.castlingRights & 15];

        if (pos.enPassantSquare != NO_SQUARE)
            hash ^= enPassant[pos.enPassantSquare % 8];

        return hash;
    }
};

// Global zobrist table - initialized once at startup
inline Zobrist& zobrist() {
    static Zobrist z;
    return z;
}