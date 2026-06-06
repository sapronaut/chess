#pragma once
#include "movegen.h"
#include "eval.h"
#include "zobrist.h"
#include "tt.h"

constexpr int INF        =  1000000;
constexpr int NEG_INF    = -1000000;
constexpr int MATE_SCORE =  900000;
constexpr int MAX_DEPTH  =  64;

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
    MoveGenerator     gen;
    TranspositionTable tt;
    long nodes;

    Move killers[MAX_DEPTH][2];

    int negamax(Position& pos, int depth, int alpha, int beta,
                int ply, uint64_t hash);
    int quiescence(Position& pos, int alpha, int beta, uint64_t hash);

    void orderMoves(const Position& pos, MoveList& list,
                    int ply, Move ttMove);
    void addKiller(Move m, int ply);
};