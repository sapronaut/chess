#include "search.h"
#include <iostream>
#include <algorithm>

Search::Search() {
    // Clear killer move table
    for (int i = 0; i < MAX_DEPTH; i++)
        killers[i][0] = killers[i][1] = Move();
}

// ============================================================
//  MOVE ORDERING
//  Order: captures (MVV-LLA) > killers > quiet moves
//  Better ordering = more cutoffs = exponentially faster search
// ============================================================
void Search::orderMoves(const Position& pos, MoveList& list, int ply) {
    constexpr int pieceValue[6] = { 100, 320, 330, 500, 900, 20000 };
    int scores[256] = {};

    for (int i = 0; i < list.count; i++) {
        Move m = list.moves[i];

        Piece victim = pos.mailbox[m.to()];
        if (victim != NO_PIECE) {
            // MVV-LLA: capture score
            int victimVal   = pieceValue[pos.typeOf(victim)];
            int attackerVal = pieceValue[pos.typeOf(pos.mailbox[m.from()])];
            scores[i] = 10000 + victimVal - attackerVal;
        } else if (m == killers[ply][0]) {
            scores[i] = 9000;   // killer move slot 1
        } else if (m == killers[ply][1]) {
            scores[i] = 8000;   // killer move slot 2
        }

        if (m.isPromotion()) scores[i] += 7000;
    }

    // Insertion sort
    for (int i = 1; i < list.count; i++) {
        Move m = list.moves[i];
        int  s = scores[i];
        int  j = i - 1;
        while (j >= 0 && scores[j] < s) {
            list.moves[j+1] = list.moves[j];
            scores[j+1]     = scores[j];
            j--;
        }
        list.moves[j+1] = m;
        scores[j+1]     = s;
    }
}

void Search::addKiller(Move m, int ply) {
    if (ply >= MAX_DEPTH) return;
    if (m != killers[ply][0]) {
        killers[ply][1] = killers[ply][0];
        killers[ply][0] = m;
    }
}

// ============================================================
//  QUIESCENCE SEARCH
//
//  Called at depth 0 instead of returning evaluate() directly.
//  Keeps searching CAPTURES ONLY until no captures are available.
//
//  Why: imagine at depth 0 white just moved a queen next to
//  a black rook. evaluate() says "white is up material!" but
//  the engine missed that the rook takes the queen next move.
//  Quiescence search finds this by continuing until quiet.
//
//  "Standing pat": if the current position is already so good
//  that we don't need to search further (score >= beta), return.
//  This is safe because captures are optional.
// ============================================================
int Search::quiescence(Position& pos, int alpha, int beta) {
    nodes++;

    // Stand pat: evaluate the position as-is
    int standPat = evaluate(pos);

    if (standPat >= beta) return beta;   // fail-hard beta cutoff
    if (standPat > alpha) alpha = standPat;

    // Generate only capture moves
    MoveList list;
    gen.generateLegalMoves(pos, list);

    // Filter to captures only
    MoveList captures;
    for (Move m : list) {
        if (pos.mailbox[m.to()] != NO_PIECE || m.isEnPassant())
            captures.add(m);
    }

    // Order captures by MVV-LLA
    orderMoves(pos, captures, 0);

    for (Move m : captures) {
        MoveGenerator::UndoInfo undo = gen.makeMove(pos, m);
        int score = -quiescence(pos, -beta, -alpha);
        gen.unmakeMove(pos, m, undo);

        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }

    return alpha;
}

// ============================================================
//  NEGAMAX WITH ALPHA-BETA + QUIESCENCE
//  Same structure as Phase 5 but now:
//    - Calls quiescence() at depth 0 instead of evaluate()
//    - Tracks killer moves for better ordering
//    - Takes ply parameter (distance from root, for killers)
// ============================================================
int Search::negamax(Position& pos, int depth, int alpha, int beta, int ply) {
    nodes++;

    // At leaf: drop into quiescence search instead of raw eval
    if (depth == 0) return quiescence(pos, alpha, beta);

    MoveList list;
    gen.generateLegalMoves(pos, list);

    if (list.empty()) {
        if (gen.isInCheck(pos, pos.sideToMove))
            return -MATE_SCORE + ply;  // checkmate
        return 0;                       // stalemate
    }

    orderMoves(pos, list, ply);

    for (Move m : list) {
        MoveGenerator::UndoInfo undo = gen.makeMove(pos, m);
        int score = -negamax(pos, depth - 1, -beta, -alpha, ply + 1);
        gen.unmakeMove(pos, m, undo);

        if (score >= beta) {
            // Beta cutoff - store as killer if quiet move
            if (pos.mailbox[m.to()] == NO_PIECE)
                addKiller(m, ply);
            return beta;
        }
        if (score > alpha) alpha = score;
    }

    return alpha;
}

// ============================================================
//  ITERATIVE DEEPENING
//
//  Search depth 1, then 2, then 3, ... up to maxDepth.
//  Each iteration is cheap because earlier results guide
//  move ordering, giving massive pruning at deeper levels.
//
//  Benefits:
//    1. Best move from depth N is tried first at depth N+1
//    2. If time runs out, we always have a best move ready
//    3. Total time is dominated by the deepest search
//       (depth N takes ~30x longer than depth N-1, so
//        depths 1..N-1 combined are only ~3% of total time)
// ============================================================
SearchResult Search::findBestMove(Position& pos, int maxDepth) {
    nodes = 0;
    Move  bestMove;
    int   bestScore = NEG_INF;

    for (int depth = 1; depth <= maxDepth; depth++) {
        // Clear killers for fresh search at each depth
        for (int i = 0; i < MAX_DEPTH; i++)
            killers[i][0] = killers[i][1] = Move();

        MoveList list;
        gen.generateLegalMoves(pos, list);
        orderMoves(pos, list, 0);

        int alpha    = NEG_INF;
        int beta     = INF;
        Move depthBest;
        int  depthScore = NEG_INF;

        for (Move m : list) {
            MoveGenerator::UndoInfo undo = gen.makeMove(pos, m);
            int score = -negamax(pos, depth - 1, -beta, -alpha, 1);
            gen.unmakeMove(pos, m, undo);

            if (score > depthScore) {
                depthScore = score;
                depthBest  = m;
            }
            if (score > alpha) alpha = score;
        }

        bestMove  = depthBest;
        bestScore = depthScore;

        std::cout << "  depth=" << depth
                  << " best=" << bestMove.toString()
                  << " score=" << bestScore
                  << " nodes=" << nodes << "\n";

        if (stop) break;
    }

    return { bestMove, bestScore, maxDepth, nodes };
}