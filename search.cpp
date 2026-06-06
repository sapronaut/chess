#include "search.h"
#include <iostream>

Search::Search() : tt(16) {
    for (int i = 0; i < MAX_DEPTH; i++)
        killers[i][0] = killers[i][1] = Move();
}

void Search::orderMoves(const Position& pos, MoveList& list,
                        int ply, Move ttMove) {
    constexpr int pieceValue[6] = { 100, 320, 330, 500, 900, 20000 };
    int scores[256] = {};

    for (int i = 0; i < list.count; i++) {
        Move m = list.moves[i];

        if (!ttMove.isNull() && m == ttMove) {
            scores[i] = 20000;  // TT move first - it's the best we know
            continue;
        }

        Piece victim = pos.mailbox[m.to()];
        if (victim != NO_PIECE) {
            int victimVal   = pieceValue[pos.typeOf(victim)];
            int attackerVal = pieceValue[pos.typeOf(pos.mailbox[m.from()])];
            scores[i] = 10000 + victimVal - attackerVal;
        } else if (m == killers[ply][0]) {
            scores[i] = 9000;
        } else if (m == killers[ply][1]) {
            scores[i] = 8000;
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

int Search::quiescence(Position& pos, int alpha, int beta, uint64_t hash) {
    nodes++;

    int standPat = evaluate(pos);
    if (standPat >= beta) return beta;
    if (standPat > alpha) alpha = standPat;

    MoveList list;
    gen.generateLegalMoves(pos, list);

    MoveList captures;
    for (Move m : list) {
        if (pos.mailbox[m.to()] != NO_PIECE || m.isEnPassant())
            captures.add(m);
    }

    orderMoves(pos, captures, 0, Move());

    for (Move m : captures) {
        MoveGenerator::UndoInfo undo = gen.makeMove(pos, m);

        // Update hash incrementally
        uint64_t newHash = zobrist().compute(pos); // simple for now
        int score = -quiescence(pos, -beta, -alpha, newHash);

        gen.unmakeMove(pos, m, undo);

        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }

    return alpha;
}

int Search::negamax(Position& pos, int depth, int alpha, int beta,
                    int ply, uint64_t hash) {
    nodes++;

    // ---- Transposition table lookup ----
    Move ttMove;
    TTEntry* entry = tt.probe(hash);
    if (entry && entry->depth >= depth) {
        if (entry->flag == TT_EXACT) return entry->score;
        if (entry->flag == TT_LOWER && entry->score > alpha) alpha = entry->score;
        if (entry->flag == TT_UPPER && entry->score < beta)  beta  = entry->score;
        if (alpha >= beta) return entry->score;
        ttMove = entry->best;
    } else if (entry) {
        ttMove = entry->best; // use best move for ordering even if depth insufficient
    }

    if (depth == 0) return quiescence(pos, alpha, beta, hash);

    MoveList list;
    gen.generateLegalMoves(pos, list);

    if (list.empty()) {
        if (gen.isInCheck(pos, pos.sideToMove))
            return -MATE_SCORE + ply;
        return 0;
    }

    orderMoves(pos, list, ply, ttMove);

    int   origAlpha = alpha;
    Move  bestMove;
    int   bestScore = NEG_INF;

    for (Move m : list) {
        MoveGenerator::UndoInfo undo = gen.makeMove(pos, m);
        uint64_t newHash = zobrist().compute(pos);
        int score = -negamax(pos, depth-1, -beta, -alpha, ply+1, newHash);
        gen.unmakeMove(pos, m, undo);

        if (score > bestScore) {
            bestScore = score;
            bestMove  = m;
        }
        if (score > alpha) alpha = score;
        if (alpha >= beta) {
            if (pos.mailbox[m.to()] == NO_PIECE)
                addKiller(m, ply);
            break;
        }
    }

    // ---- Store in transposition table ----
    TTFlag flag;
    if      (bestScore <= origAlpha) flag = TT_UPPER;
    else if (bestScore >= beta)      flag = TT_LOWER;
    else                             flag = TT_EXACT;

    tt.store(hash, bestScore, depth, flag, bestMove);

    return bestScore;
}

SearchResult Search::findBestMove(Position& pos, int maxDepth) {
    nodes = 0;
    tt.clear();
    Move  bestMove;
    int   bestScore = NEG_INF;

    uint64_t rootHash = zobrist().compute(pos);

    for (int depth = 1; depth <= maxDepth; depth++) {
        for (int i = 0; i < MAX_DEPTH; i++)
            killers[i][0] = killers[i][1] = Move();

        MoveList list;
        gen.generateLegalMoves(pos, list);
        orderMoves(pos, list, 0, bestMove); // use prev best move first

        int alpha = NEG_INF, beta = INF;
        Move  depthBest;
        int   depthScore = NEG_INF;

        for (Move m : list) {
            MoveGenerator::UndoInfo undo = gen.makeMove(pos, m);
            uint64_t newHash = zobrist().compute(pos);
            int score = -negamax(pos, depth-1, -beta, -alpha, 1, newHash);
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
                  << " best="   << bestMove.toString()
                  << " score="  << bestScore
                  << " nodes="  << nodes  << "\n";

        if (stop) break;
    }

    return { bestMove, bestScore, maxDepth, nodes };
}