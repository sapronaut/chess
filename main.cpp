#include "board.h"
#include "movegen.h"
#include "eval.h"
#include "search.h"
#include <iostream>
#include <cassert>

void testSearch() {
    Search search;
    Position pos;

    // ---- Test 1: find mate in 1 ----
    // White to move, Qh5# is mate in 1
    pos.setFromFEN("r1bqkb1r/pppp1ppp/2n2n2/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq - 0 1");
    pos.print();
    std::cout << "Searching for mate in 1 (depth 2)...\n";
    SearchResult r1 = search.findBestMove(pos, 2);
    std::cout << "\nBest move: " << r1.bestMove.toString()
              << " score=" << r1.score
              << " nodes=" << r1.nodesSearched << "\n";
    assert(r1.bestMove.toString() == "h5f7");
    std::cout << "Mate in 1: PASS\n\n";

    // ---- Test 2: capture the free queen ----
    // White rook can take black queen for free
    pos.setFromFEN("3qk3/8/8/8/8/8/8/3RK3 w - - 0 1");
    pos.print();
    std::cout << "Searching for free queen capture (depth 3)...\n";
    SearchResult r2 = search.findBestMove(pos, 3);
    std::cout << "\nBest move: " << r2.bestMove.toString()
              << " score=" << r2.score
              << " nodes=" << r2.nodesSearched << "\n";
    assert(r2.bestMove.toString() == "d1d8");
    std::cout << "Capture queen: PASS\n\n";

    // ---- Test 3: search from start position ----
    pos.setFromFEN(START_FEN);
    std::cout << "Searching start position (depth 4)...\n";
    SearchResult r3 = search.findBestMove(pos, 4);
    std::cout << "\nBest move: " << r3.bestMove.toString()
              << " score=" << r3.score
              << " nodes=" << r3.nodesSearched << "\n\n";
}

int main() {
    std::cout << "=== Chess Engine - Phase 5: Search ===\n\n";
    testSearch();
    std::cout << "All search tests passed!\n";
    std::cout << "Next: Phase 6 - Iterative deepening + move ordering improvements\n";
    return 0;
}