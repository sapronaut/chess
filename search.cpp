#include "search.h"
#include <algorithm>
#include <climits>
#include <iostream>

// Infinity score (but leave room for mate scores above this)
constexpr int INF      =  1000000;
constexpr int NEG_INF  = -1000000;

// Mate score: being checkmated = -INF + depth
// (prefer faster mates by using depth in the score)
constexpr int MATE_SCORE = 900000;

Search::Search() {}

// ============================================================
//  MOVE ORDERING
//  Search is exponential in branching factor.
//  Better moves searched first = more beta cutoffs = faster.
//
//  Simple heuristic: captures before quiet moves.
//  We score captures by MVV-LLA (Most Valuable Victim,
//  Least Valuable Attacker): capturing a queen with a pawn
//  is searched before capturing a pawn with a queen.
// ============================================================
void Search::orderMoves(const Position& pos, MoveList& list) {
    constexpr int pieceValue[6] = { 100, 320, 330, 500, 900, 20000 };

    // Score each move
    int scores[256] = {};
    for (int i = 0; i < list.count; i++) {
        Move m = list.moves[i];
        Square to = m.to();
        Piece victim = pos.mailbox[to];

        if (victim != NO_PIECE) {
            // MVV-LLA: big bonus for capturing valuable pieces with cheap pieces
            Piece attacker = pos.mailbox[m.from()];
            int victimVal   = pieceValue[pos.typeOf(victim)];
            int attackerVal = pieceValue[pos.typeOf(attacker)];
            scores[i] = 10000 + victimVal - attackerVal;
        }

        if (m.isPromotion()) scores[i] += 9000;
    }

    // Simple insertion sort (MoveList is small, this is fine)
    for (int i = 1; i < list.count; i++) {
        Move  m = list.moves[i];
        int   s = scores[i];
        int   j = i - 1;
        while (j >= 0 && scores[j] < s) {
            list.moves[j+1] = list.moves[j];
            scores[j+1]     = scores[j];
            j--;
        }
        list.moves[j+1] = m;
        scores[j+1]     = s;
    }
}

// ============================================================
//  NEGAMAX WITH ALPHA-BETA
//
//  At each node:
//    1. Generate legal moves
//    2. If no moves: checkmate or stalemate
//    3. For each move: recurse, negate score
//    4. Keep track of best score, update alpha
//    5. If score >= beta: prune (opponent won't allow this)
//
//  The negation (-negamax(...)) is the key insight:
//  a good position for the opponent is bad for us.
// ============================================================
int Search::negamax(Position& pos, int depth, int alpha, int beta) {
    nodes++;

    // Base case: evaluate the position
    if (depth == 0) return evaluate(pos);

    MoveList list;
    gen.generateLegalMoves(pos, list);

    // No legal moves = checkmate or stalemate
    if (list.empty()) {
        if (gen.isInCheck(pos, pos.sideToMove))
            return -MATE_SCORE + (1000 - depth); // checkmate (prefer faster mates)
        return 0; // stalemate
    }

    // Order moves for better pruning
    orderMoves(pos, list);

    int bestScore = NEG_INF;

    for (Move m : list) {
        // Make the move
        MoveGenerator::UndoInfo undo = gen.makeMove(pos, m);

        // Recurse: negate because opponent maximizes their score
        int score = -negamax(pos, depth - 1, -beta, -alpha);

        // Unmake the move
        gen.unmakeMove(pos, m, undo);

        if (score > bestScore) bestScore = score;
        if (score > alpha)     alpha = score;

        // Beta cutoff: opponent won't allow this position
        // No need to search further moves
        if (alpha >= beta) break;
    }

    return bestScore;
}

// ============================================================
//  FIND BEST MOVE
//  Searches all root moves and returns the best one.
//  This is essentially negamax but we also track which move
//  produced the best score.
// ============================================================
SearchResult Search::findBestMove(Position& pos, int depth) {
    nodes = 0;

    MoveList list;
    gen.generateLegalMoves(pos, list);
    orderMoves(pos, list);

    Move  bestMove;
    int   bestScore = NEG_INF;
    int   alpha     = NEG_INF;
    int   beta      = INF;

    for (Move m : list) {
        MoveGenerator::UndoInfo undo = gen.makeMove(pos, m);
        int score = -negamax(pos, depth - 1, -beta, -alpha);
        gen.unmakeMove(pos, m, undo);

        std::cout << "  " << m.toString()
                  << " score=" << score << "\n";

        if (score > bestScore) {
            bestScore = score;
            bestMove  = m;
        }
        if (score > alpha) alpha = score;
    }

    return { bestMove, bestScore, depth, nodes };
}