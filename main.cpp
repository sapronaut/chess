#include "board.h"
#include "movegen.h"
#include "eval.h"
#include <iostream>
#include <cassert>
#include <cstdint>

// ---- Perft (kept from Phase 3) ----
uint64_t perft(Position pos, MoveGenerator& gen, int depth) {
    MoveList list;
    gen.generateLegalMoves(pos, list);
    if (depth == 1) return (uint64_t)list.size();
    uint64_t nodes = 0;
    for (Move m : list) {
        Position next = pos;
        gen.makeMove(next, m);
        nodes += perft(next, gen, depth - 1);
    }
    return nodes;
}

// ---- Phase 4 tests ----
void testEvaluation() {
    Position pos;

    // Starting position should be 0 (perfectly symmetric)
    pos.setFromFEN(START_FEN);
    int startScore = evaluate(pos);
    std::cout << "  Start pos eval:     " << startScore
              << " (expected ~0)\n";
    assert(startScore == 0);

    // Remove a white queen - black should be winning (negative score)
    pos.setFromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNB1KBNR w KQkq - 0 1");
    int noQueenScore = evaluate(pos);
    std::cout << "  White missing queen: " << noQueenScore
              << " (expected ~-900)\n";
    assert(noQueenScore < -800);

    // Extra pawn for white - white should be slightly positive
    pos.setFromFEN("rnbqkbnr/p1pppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    int extraPawnScore = evaluate(pos);
    std::cout << "  White extra pawn:    " << extraPawnScore
              << " (expected ~+100)\n";
    assert(extraPawnScore > 50);

    // Knights in center vs rim - center knights should score higher
    // White knight on e4 (center) vs a1 (corner)
    pos.setFromFEN("7k/8/8/8/4N3/8/8/K7 w - - 0 1");
    int centerKnight = evaluate(pos);

    pos.setFromFEN("7k/8/8/8/8/8/8/KN6 w - - 0 1");
    int rimKnight = evaluate(pos);

    std::cout << "  Knight e4 score:     " << centerKnight << "\n";
    std::cout << "  Knight b1 score:     " << rimKnight << "\n";
    assert(centerKnight > rimKnight);
    std::cout << "  PST knight test:     PASS (center > rim)\n";

    std::cout << "\n  All eval tests PASSED\n";
}

void printEval(const std::string& fen) {
    Position pos;
    pos.setFromFEN(fen);
    pos.print();
    int score = evaluate(pos);
    std::cout << "  Eval: " << score << " centipawns"
              << " (+" << score/100 << "." << abs(score%100) << " pawns)"
              << " for " << (pos.sideToMove == WHITE ? "White" : "Black")
              << "\n\n";
}

int main() {
    std::cout << "=== Chess Engine - Phase 4: Static Evaluation ===\n\n";

    testEvaluation();

    std::cout << "\n--- Sample evaluations ---\n\n";

    printEval(START_FEN);
    printEval("r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4");
    printEval("8/8/8/4k3/8/8/4K3/8 w - - 0 1");  // K vs K endgame

    std::cout << "Next step: Phase 5 - Search (minimax + alpha-beta)\n";
    return 0;
}