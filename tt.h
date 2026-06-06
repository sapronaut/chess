#pragma once
#include "move.h"
#include <vector>
#include <cstring>

// ============================================================
//  TRANSPOSITION TABLE
//
//  A hash map from position hash -> search result.
//  When we reach a position we've seen before, we can reuse
//  the cached result instead of re-searching.
//
//  Each entry stores:
//  - hash:  full 64-bit hash (to verify it's the right position)
//  - score: the evaluation from search
//  - depth: how deep this result was searched
//  - flag:  EXACT, LOWER_BOUND, or UPPER_BOUND
//  - best:  the best move found (for move ordering)
//
//  Flag types:
//  - EXACT:       score is the true value (within alpha-beta window)
//  - LOWER_BOUND: score >= beta caused cutoff (we stopped early)
//  - UPPER_BOUND: score <= alpha, no move improved alpha
//
//  Size: 1 million entries x ~16 bytes = ~16MB. Reasonable.
// ============================================================

enum TTFlag : uint8_t {
    TT_NONE  = 0,
    TT_EXACT = 1,
    TT_LOWER = 2,  // lower bound (beta cutoff)
    TT_UPPER = 3   // upper bound (alpha cutoff)
};

struct TTEntry {
    uint64_t hash  = 0;
    int      score = 0;
    Move     best;
    int8_t   depth = 0;
    TTFlag   flag  = TT_NONE;
};

class TranspositionTable {
public:
    TranspositionTable(size_t sizeMB = 16) {
        size_t entries = (sizeMB * 1024 * 1024) / sizeof(TTEntry);
        table.resize(entries);
        clear();
    }

    void clear() {
        memset(table.data(), 0, table.size() * sizeof(TTEntry));
    }

    // Store a result in the table
    void store(uint64_t hash, int score, int depth, TTFlag flag, Move best) {
        TTEntry& entry = table[hash % table.size()];
        // Replace if: empty, same position, or deeper search
        if (entry.flag == TT_NONE || entry.hash == hash || depth >= entry.depth) {
            entry.hash  = hash;
            entry.score = score;
            entry.depth = (int8_t)depth;
            entry.flag  = flag;
            entry.best  = best;
        }
    }

    // Look up a position - returns nullptr if not found
    TTEntry* probe(uint64_t hash) {
        TTEntry& entry = table[hash % table.size()];
        if (entry.hash == hash && entry.flag != TT_NONE)
            return &entry;
        return nullptr;
    }

private:
    std::vector<TTEntry> table;
};