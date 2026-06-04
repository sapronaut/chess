#include "board.h"
#include "movegen.h"
#include <iostream>
#include <cassert>
#include <cstdint>

// ============================================================
//  PERFT
//  Pass pos by value so each recursive call gets its own copy.
//  makeMove modifies it, but the caller's copy is untouched.
//  Clean, simple, correct.
// ============================================================
uint64_t perft(Position pos, MoveGenerator& gen, int depth) {
    MoveList list;
    gen.generateLegalMoves(pos, list);

    if (depth == 1) return (uint64_t)list.size();

    uint64_t nodes = 0;
    for (Move m : list) {
        Position next = pos;
        MoveGenerator::UndoInfo undo = gen.makeMove(next, m);
        nodes += perft(next, gen, depth - 1);
        // no unmake needed - next is a local copy, just discarded
    }
    return nodes;
}

// Perft with divide: shows node count per move at root.
// Incredibly useful for finding bugs - if one move has wrong count,
// that's exactly where your bug is.
void perftDivide(Position& pos, MoveGenerator& gen, int depth) {
    MoveList list;
    gen.generateLegalMoves(pos, list);

    uint64_t total = 0;
    for (Move m : list) {
        Position next = pos;
        gen.makeMove(next, m);
        uint64_t count = perft(next, gen, depth - 1);
        std::cout << "  " << m.toString() << ": " << count << "\n";
        total += count;
    }
    std::cout << "  Total: " << total << "\n";
}

// ============================================================
//  KNOWN PERFT VALUES (from chessprogramming.org)
//  These are gospel. If your numbers match, your engine is correct.
//
//  Starting position:
//    depth 1:         20
//    depth 2:        400
//    depth 3:       8902
//    depth 4:     197281
//    depth 5:   4865609
//
//  Position 2 (Kiwipete) - stresses castling, en passant, promotions:
//  "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"
//    depth 1:         48
//    depth 2:       2039
//    depth 3:      97862
// ============================================================

struct PerftTest {
    const char* fen;
    int depth;
    uint64_t expected;
    const char* label;
};

void runPerftSuite() {
    MoveGenerator gen;

    PerftTest tests[] = {
        { START_FEN, 1,       20, "Start pos depth 1" },
        { START_FEN, 2,      400, "Start pos depth 2" },
        { START_FEN, 3,     8902, "Start pos depth 3" },
        { START_FEN, 4,   197281, "Start pos depth 4" },

        // Kiwipete - the ultimate stress test
        { "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
          1, 48,   "Kiwipete depth 1" },
        { "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
          2, 2039, "Kiwipete depth 2" },
        { "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
          3, 97862,"Kiwipete depth 3" },
    };

    int passed = 0, failed = 0;
    for (auto& t : tests) {
        Position pos;
        pos.setFromFEN(t.fen);
        uint64_t result = perft(pos, gen, t.depth);

        bool ok = (result == t.expected);
        std::cout << (ok ? "  PASS" : "  FAIL")
                  << "  " << t.label
                  << "  got=" << result
                  << "  expected=" << t.expected << "\n";
        ok ? passed++ : failed++;
    }

    std::cout << "\n  " << passed << " passed, " << failed << " failed\n";

    if (failed > 0) {
        std::cout << "\n--- Debugging: perft divide on failing position ---\n";
        std::cout << "Start pos depth 3 divide:\n";
        Position pos; pos.setFromFEN(START_FEN);
        perftDivide(pos, gen, 3);
    }
}

int main() {
    std::cout << "=== Chess Engine - Phase 3: Perft Testing ===\n\n";
    runPerftSuite();
    std::cout << "\nIf all pass: move generator is CORRECT. On to Phase 4!\n";
    return 0;
}