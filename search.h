#pragma once
#include "movegen.h"
#include "eval.h"

// ============================================================
//  SEARCH
//  Finds the best move in a position by exploring the game tree.
//
//  Algorithm: Negamax + Alpha-Beta pruning
//
//  Negamax: a clean version of minimax that exploits the fact
//  that eval is always from side-to-move's perspective.
//  Instead of separate min/max logic, every node just maximizes.
//  The score is negated when returning to the parent.
//
//  Alpha-Beta: prunes branches that can't affect the result.
//  Alpha = best score the maximizing player is guaranteed
//  Beta  = best score the minimizing player is guaranteed
//  If a move scores >= beta, the opponent won't allow this
//  position - stop searching (beta cutoff).
//
//  This reduces the effective branching factor from ~30 to ~6
//  in practice, letting us search ~2x the depth for free.
// ============================================================

struct SearchResult {
    Move  bestMove;
    int   score;
    int   depth;
    long  nodesSearched;
};

class Search {
public:
    Search();

    // Find the best move searching to given depth
    SearchResult findBestMove(Position& pos, int depth);

    // Stop flag (for future UCI time management)
    bool stop = false;

private:
    MoveGenerator gen;
    long nodes;

    // Core negamax with alpha-beta pruning
    // alpha: minimum score current player is guaranteed
    // beta:  maximum score current player will allow opponent
    int negamax(Position& pos, int depth, int alpha, int beta);

    // Simple move ordering: try captures first
    // Better move ordering = more alpha-beta cutoffs = faster search
    void orderMoves(const Position& pos, MoveList& list);
};