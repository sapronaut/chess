#include "board.h"
#include <iostream>
#include <cassert>

// ============================================================
//  Quick sanity tests for Phase 1
//  Run these after every commit to catch regressions early.
// ============================================================

void testBitboardOps() {
    std::cout << "Testing bitboard operations...\n";

    Bitboard bb = 0;
    setBit(bb, E4);
    assert(testBit(bb, E4));
    assert(!testBit(bb, E5));
    assert(popcount(bb) == 1);
    assert(lsb(bb) == E4);

    setBit(bb, D4);
    assert(popcount(bb) == 2);

    Square s = popLSB(bb);
    assert(s == D4);              // D4=27, E4=28 -> D4 is LSB
    assert(popcount(bb) == 1);    // only E4 remains

    clearBit(bb, E4);
    assert(bb == 0);

    std::cout << "  Bitboard ops: PASS\n";
}

void testStartPosition() {
    std::cout << "Testing start position...\n";

    Position pos;
    pos.setFromFEN(START_FEN);
    pos.print();

    // White should have 8 pawns on rank 2
    assert(pos.getPieces(WHITE, PAWN) == RANK_2);

    // Black should have 8 pawns on rank 7
    assert(pos.getPieces(BLACK, PAWN) == RANK_7);

    // Total pieces: 32
    assert(popcount(pos.allPieces) == 32);

    // White king on e1, black king on e8
    assert(testBit(pos.getPieces(WHITE, KING), E1));
    assert(testBit(pos.getPieces(BLACK, KING), E8));

    // Side to move is white
    assert(pos.sideToMove == WHITE);

    // All castling rights available
    assert(pos.castlingRights == ALL_CASTLING);

    std::cout << "  Start position: PASS\n";
}

void testFENRoundtrip() {
    std::cout << "Testing FEN round-trip...\n";

    // A mid-game position
    const std::string fen = "r1bqk2r/pppp1ppp/2n2n2/2b1p3/2B1P3/2N2N2/PPPP1PPP/R1BQK2R w KQkq - 4 4";
    Position pos;
    pos.setFromFEN(fen);

    std::string out = pos.toFEN();
    assert(out == fen);
    std::cout << "  FEN round-trip: PASS\n";

    pos.print();
}

void testMailbox() {
    std::cout << "Testing mailbox...\n";

    Position pos;
    pos.setFromFEN(START_FEN);

    // Check specific squares
    assert(pos.pieceOn(E1) == W_KING);
    assert(pos.pieceOn(D1) == W_QUEEN);
    assert(pos.pieceOn(E8) == B_KING);
    assert(pos.pieceOn(E4) == NO_PIECE);  // empty square

    std::cout << "  Mailbox: PASS\n";
}

int main() {
    std::cout << "=== Chess Engine - Phase 1: Board Representation ===\n\n";

    testBitboardOps();
    testStartPosition();
    testFENRoundtrip();
    testMailbox();

    std::cout << "\nAll Phase 1 tests passed!\n";
    std::cout << "Next step: Phase 2 - Move Generation\n";
    return 0;
}