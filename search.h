#pragma once
#include "movegen.h"
#include "eval.h"

constexpr int INF       =  1000000;
constexpr int NEG_INF   = -1000000;
constexpr int MATE_SCORE = 900000;
constexpr int MAX_DEPTH  = 64;

struct SearchResult {
    Move  bestMove;
    int   score;
    int   depth;
    long  nodesSearched;
};

class Search {
public:
    Search();
    SearchResult findBestMove(Position& pos, int maxDepth);
    bool stop = false;

private:
    MoveGenerator gen;
    long nodes;

    // Killer moves: quiet moves that caused a beta cutoff
    // stored per depth, 2 slots each
    Move killers[MAX_DEPTH][2];

    // Core search functions
    int negamax(Position& pos, int depth, int alpha, int beta, int ply);

    // Quiescence search: keep searching captures until position is quiet
    // Prevents the "horizon effect" - missing obvious captures at leaf nodes
    int quiescence(Position& pos, int alpha, int beta);

    void orderMoves(const Position& pos, MoveList& list, int ply);
    void addKiller(Move m, int ply);
};