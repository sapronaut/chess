#include "board.h"
#include "movegen.h"
#include "eval.h"
#include "search.h"
#include <iostream>
#include <cassert>

void testSearch() {
    Search search;
    Position pos;

    // Test 1: mate in 1
    std::cout << "--- Mate in 1 ---\n";
    pos.setFromFEN("r1bqkb1r/pppp1ppp/2n2n2/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq - 0 1");
    SearchResult r1 = search.findBestMove(pos, 4);
    std::cout << "Best: " << r1.bestMove.toString()
              << " nodes=" << r1.nodesSearched << "\n";
    assert(r1.bestMove.toString() == "h5f7");
    std::cout << "PASS\n\n";

    // Test 2: free queen
    std::cout << "--- Capture free queen ---\n";
    pos.setFromFEN("3qk3/8/8/8/8/8/8/3RK3 w - - 0 1");
    SearchResult r2 = search.findBestMove(pos, 5);
    std::cout << "Best: " << r2.bestMove.toString()
              << " nodes=" << r2.nodesSearched << "\n";
    assert(r2.bestMove.toString() == "d1d8");
    std::cout << "PASS\n\n";

    // Test 3: start position - compare node counts with Phase 6
    std::cout << "--- Start position depth 6 ---\n";
    pos.setFromFEN(START_FEN);
    SearchResult r3 = search.findBestMove(pos, 6);
    std::cout << "Best: " << r3.bestMove.toString()
              << " score=" << r3.score
              << " nodes=" << r3.nodesSearched << "\n\n";
}

int main() {
    std::cout << "=== Chess Engine - Phase 7: Transposition Table ===\n\n";
    testSearch();
    std::cout << "All tests passed!\n";
    std::cout << "Next: Phase 8 - UCI interface (play vs GUIs!)\n";
    return 0;
}