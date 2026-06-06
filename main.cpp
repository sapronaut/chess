#include "uci.h"
#include "search.h"
#include <iostream>
#include <cassert>

void smokeTest() {
    // Quick sanity check before entering UCI loop
    Search search;
    Position pos;

    pos.setFromFEN("r1bqkb1r/pppp1ppp/2n2n2/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq - 0 1");
    SearchResult r = search.findBestMove(pos, 3);
    assert(r.bestMove.toString() == "h5f7");

    std::cerr << "Smoke test passed. Starting UCI loop.\n";
}

int main(int argc, char* argv[]) {
    // Run smoke test unless "--uci" flag passed
    // (some GUIs pass flags, skip test in that case)
    if (argc < 2) smokeTest();

    UCI uci;
    uci.loop();
    return 0;
}