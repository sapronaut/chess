#include "movegen.h"
#include <cassert>

MoveGenerator::MoveGenerator() {
    initKnightAttacks();
    initKingAttacks();
    initPawnAttacks();
}

void MoveGenerator::initKnightAttacks() {
    for (Square sq = 0; sq < 64; sq++) {
        Bitboard b = squareBB(sq);
        Bitboard a = 0;
        a |= (b << 17) & ~FILE_A;
        a |= (b << 15) & ~FILE_H;
        a |= (b << 10) & ~(FILE_A | (FILE_A << 1));
        a |= (b <<  6) & ~(FILE_H | (FILE_H >> 1));
        a |= (b >>  6) & ~(FILE_A | (FILE_A << 1));
        a |= (b >> 10) & ~(FILE_H | (FILE_H >> 1));
        a |= (b >> 15) & ~FILE_A;
        a |= (b >> 17) & ~FILE_H;
        knightAttacks[sq] = a;
    }
}

void MoveGenerator::initKingAttacks() {
    for (Square sq = 0; sq < 64; sq++) {
        Bitboard b = squareBB(sq);
        Bitboard a = 0;
        a |= (b << 8);
        a |= (b >> 8);
        a |= (b << 1) & ~FILE_A;
        a |= (b >> 1) & ~FILE_H;
        a |= (b << 9) & ~FILE_A;
        a |= (b << 7) & ~FILE_H;
        a |= (b >> 7) & ~FILE_A;
        a |= (b >> 9) & ~FILE_H;
        kingAttacks[sq] = a;
    }
}

void MoveGenerator::initPawnAttacks() {
    for (Square sq = 0; sq < 64; sq++) {
        Bitboard b = squareBB(sq);
        pawnAttacks[WHITE][sq] = ((b << 9) & ~FILE_A) | ((b << 7) & ~FILE_H);
        pawnAttacks[BLACK][sq] = ((b >> 7) & ~FILE_A) | ((b >> 9) & ~FILE_H);
    }
}

Bitboard MoveGenerator::bishopAttacks(Square sq, Bitboard occupied) const {
    Bitboard attacks = 0;
    Bitboard b = squareBB(sq), cur;

    cur = b; while (!(cur & FILE_H) && !(cur & RANK_8)) { cur <<= 9; attacks |= cur; if (cur & occupied) break; }
    cur = b; while (!(cur & FILE_A) && !(cur & RANK_8)) { cur <<= 7; attacks |= cur; if (cur & occupied) break; }
    cur = b; while (!(cur & FILE_H) && !(cur & RANK_1)) { cur >>= 7; attacks |= cur; if (cur & occupied) break; }
    cur = b; while (!(cur & FILE_A) && !(cur & RANK_1)) { cur >>= 9; attacks |= cur; if (cur & occupied) break; }

    return attacks;
}

Bitboard MoveGenerator::rookAttacks(Square sq, Bitboard occupied) const {
    Bitboard attacks = 0;
    Bitboard b = squareBB(sq), cur;

    cur = b; while (!(cur & RANK_8)) { cur <<= 8; attacks |= cur; if (cur & occupied) break; }
    cur = b; while (!(cur & RANK_1)) { cur >>= 8; attacks |= cur; if (cur & occupied) break; }
    cur = b; while (!(cur & FILE_H)) { cur <<= 1; attacks |= cur; if (cur & occupied) break; }
    cur = b; while (!(cur & FILE_A)) { cur >>= 1; attacks |= cur; if (cur & occupied) break; }

    return attacks;
}

Bitboard MoveGenerator::queenAttacks(Square sq, Bitboard occupied) const {
    return bishopAttacks(sq, occupied) | rookAttacks(sq, occupied);
}

bool MoveGenerator::isSquareAttacked(const Position& pos, Square sq, Color attacker) const {
    Bitboard occ = pos.allPieces;
    Color defender = Color(attacker ^ 1);

    if (pawnAttacks[defender][sq] & pos.pieces[attacker][PAWN])   return true;
    if (knightAttacks[sq]         & pos.pieces[attacker][KNIGHT]) return true;
    if (kingAttacks[sq]           & pos.pieces[attacker][KING])   return true;
    if (bishopAttacks(sq, occ) & (pos.pieces[attacker][BISHOP] | pos.pieces[attacker][QUEEN])) return true;
    if (rookAttacks(sq, occ)   & (pos.pieces[attacker][ROOK]   | pos.pieces[attacker][QUEEN])) return true;

    return false;
}

bool MoveGenerator::isInCheck(const Position& pos, Color c) const {
    Bitboard kingBB = pos.pieces[c][KING];
    if (!kingBB) return false;
    return isSquareAttacked(pos, lsb(kingBB), Color(c ^ 1));
}

void MoveGenerator::generatePawnMoves(const Position& pos, MoveList& list) {
    Color us   = pos.sideToMove;
    Color them = Color(us ^ 1);

    Bitboard ourPawns  = pos.pieces[us][PAWN];
    Bitboard enemies   = pos.occupancy[them];
    Bitboard empty     = ~pos.allPieces;

    int pushDir  = (us == WHITE) ?  8 : -8;
    int push2Dir = (us == WHITE) ? 16 : -16;
    Bitboard startRank = (us == WHITE) ? RANK_2 : RANK_7;
    Bitboard promoRank = (us == WHITE) ? RANK_8 : RANK_1;

    Bitboard singlePush = (us == WHITE) ? (ourPawns << 8) & empty
                                        : (ourPawns >> 8) & empty;
    Bitboard doublePush = (us == WHITE) ? ((ourPawns & startRank) << 8 & empty) << 8 & empty
                                        : ((ourPawns & startRank) >> 8 & empty) >> 8 & empty;

    // Single pushes (non-promotion)
    Bitboard tmp = singlePush & ~promoRank;
    while (tmp) {
        Square to = popLSB(tmp);
        list.add(Square(to - pushDir), to);
    }

    // Single push promotions
    tmp = singlePush & promoRank;
    while (tmp) {
        Square to = popLSB(tmp);
        Square from = Square(to - pushDir);
        list.add(from, to, PROMOTION, PROMO_QUEEN);
        list.add(from, to, PROMOTION, PROMO_ROOK);
        list.add(from, to, PROMOTION, PROMO_BISHOP);
        list.add(from, to, PROMOTION, PROMO_KNIGHT);
    }

    // Double pushes
    tmp = doublePush;
    while (tmp) {
        Square to = popLSB(tmp);
        list.add(Square(to - push2Dir), to);
    }

    // Captures
    Bitboard pawns = ourPawns;
    while (pawns) {
        Square from   = popLSB(pawns);
        Bitboard atks = pawnAttacks[us][from] & enemies;
        while (atks) {
            Square to = popLSB(atks);
            if (squareBB(to) & promoRank) {
                list.add(from, to, PROMOTION, PROMO_QUEEN);
                list.add(from, to, PROMOTION, PROMO_ROOK);
                list.add(from, to, PROMOTION, PROMO_BISHOP);
                list.add(from, to, PROMOTION, PROMO_KNIGHT);
            } else {
                list.add(from, to);
            }
        }
    }

    // En passant
    if (pos.enPassantSquare != NO_SQUARE) {
        Bitboard ep = pawnAttacks[them][pos.enPassantSquare] & ourPawns;
        while (ep) list.add(popLSB(ep), pos.enPassantSquare, EN_PASSANT);
    }
}

void MoveGenerator::generateKnightMoves(const Position& pos, MoveList& list) {
    Color us = pos.sideToMove;
    Bitboard knights = pos.pieces[us][KNIGHT];
    Bitboard notUs   = ~pos.occupancy[us];
    while (knights) {
        Square from    = popLSB(knights);
        Bitboard moves = knightAttacks[from] & notUs;
        while (moves) list.add(from, popLSB(moves));
    }
}

void MoveGenerator::generateBishopMoves(const Position& pos, MoveList& list) {
    Color us = pos.sideToMove;
    Bitboard bishops = pos.pieces[us][BISHOP];
    Bitboard notUs   = ~pos.occupancy[us];
    Bitboard occ     = pos.allPieces;
    while (bishops) {
        Square from    = popLSB(bishops);
        Bitboard moves = bishopAttacks(from, occ) & notUs;
        while (moves) list.add(from, popLSB(moves));
    }
}

void MoveGenerator::generateRookMoves(const Position& pos, MoveList& list) {
    Color us = pos.sideToMove;
    Bitboard rooks = pos.pieces[us][ROOK];
    Bitboard notUs = ~pos.occupancy[us];
    Bitboard occ   = pos.allPieces;
    while (rooks) {
        Square from    = popLSB(rooks);
        Bitboard moves = rookAttacks(from, occ) & notUs;
        while (moves) list.add(from, popLSB(moves));
    }
}

void MoveGenerator::generateQueenMoves(const Position& pos, MoveList& list) {
    Color us = pos.sideToMove;
    Bitboard queens = pos.pieces[us][QUEEN];
    Bitboard notUs  = ~pos.occupancy[us];
    Bitboard occ    = pos.allPieces;
    while (queens) {
        Square from    = popLSB(queens);
        Bitboard moves = queenAttacks(from, occ) & notUs;
        while (moves) list.add(from, popLSB(moves));
    }
}

void MoveGenerator::generateKingMoves(const Position& pos, MoveList& list) {
    Color us = pos.sideToMove;
    Bitboard king  = pos.pieces[us][KING];
    Bitboard notUs = ~pos.occupancy[us];
    while (king) {
        Square from    = popLSB(king);
        Bitboard moves = kingAttacks[from] & notUs;
        while (moves) list.add(from, popLSB(moves));
    }
}

void MoveGenerator::generateCastlingMoves(const Position& pos, MoveList& list) {
    Color us   = pos.sideToMove;
    Color them = Color(us ^ 1);
    Bitboard occ = pos.allPieces;

    if (us == WHITE) {
        if ((pos.castlingRights & WHITE_OO)
            && !(occ & (squareBB(F1) | squareBB(G1)))
            && !isSquareAttacked(pos, E1, them)
            && !isSquareAttacked(pos, F1, them)
            && !isSquareAttacked(pos, G1, them))
            list.add(E1, G1, CASTLING);

        if ((pos.castlingRights & WHITE_OOO)
            && !(occ & (squareBB(B1) | squareBB(C1) | squareBB(D1)))
            && !isSquareAttacked(pos, E1, them)
            && !isSquareAttacked(pos, D1, them)
            && !isSquareAttacked(pos, C1, them))
            list.add(E1, C1, CASTLING);
    } else {
        if ((pos.castlingRights & BLACK_OO)
            && !(occ & (squareBB(F8) | squareBB(G8)))
            && !isSquareAttacked(pos, E8, them)
            && !isSquareAttacked(pos, F8, them)
            && !isSquareAttacked(pos, G8, them))
            list.add(E8, G8, CASTLING);

        if ((pos.castlingRights & BLACK_OOO)
            && !(occ & (squareBB(B8) | squareBB(C8) | squareBB(D8)))
            && !isSquareAttacked(pos, E8, them)
            && !isSquareAttacked(pos, D8, them)
            && !isSquareAttacked(pos, C8, them))
            list.add(E8, C8, CASTLING);
    }
}

MoveGenerator::UndoInfo MoveGenerator::makeMove(Position& pos, Move m) const {
    UndoInfo undo;
    undo.captured        = NO_PIECE;
    undo.castlingRights  = pos.castlingRights;
    undo.enPassantSquare = pos.enPassantSquare;
    undo.halfmoveClock   = pos.halfmoveClock;

    Square from = m.from();
    Square to   = m.to();
    Color  us   = pos.sideToMove;
    Color  them = Color(us ^ 1);

    Piece movingPiece = pos.mailbox[from];
    Piece captured    = pos.mailbox[to];

    if (captured != NO_PIECE) {
        undo.captured = captured;
        pos.removePiece(to);
        pos.halfmoveClock = 0;
    }

    pos.removePiece(from);

    if (m.isPromotion()) {
        static const PieceType promoPT[] = { KNIGHT, BISHOP, ROOK, QUEEN };
        pos.putPiece(Piece(us * 6 + promoPT[m.promo()]), to);
        pos.halfmoveClock = 0;

    } else if (m.isEnPassant()) {
        pos.putPiece(movingPiece, to);
        Square capSq = Square(to + (us == WHITE ? -8 : 8));
        undo.captured = pos.mailbox[capSq];
        pos.removePiece(capSq);
        pos.halfmoveClock = 0;

    } else if (m.isCastling()) {
        pos.putPiece(movingPiece, to);
        Square rookFrom, rookTo;
        if      (to == G1) { rookFrom = H1; rookTo = F1; }
        else if (to == C1) { rookFrom = A1; rookTo = D1; }
        else if (to == G8) { rookFrom = H8; rookTo = F8; }
        else               { rookFrom = A8; rookTo = D8; }
        Piece rook = pos.mailbox[rookFrom];
        pos.removePiece(rookFrom);
        pos.putPiece(rook, rookTo);

    } else {
        pos.putPiece(movingPiece, to);
        if (pos.typeOf(movingPiece) == PAWN) pos.halfmoveClock = 0;
    }

    pos.enPassantSquare = NO_SQUARE;
    if (pos.typeOf(movingPiece) == PAWN && abs(to - from) == 16)
        pos.enPassantSquare = Square((from + to) / 2);

    const int castlingMask[64] = {
        ~WHITE_OOO,15,15,15,~(WHITE_OO|WHITE_OOO),15,15,~WHITE_OO,
        15,15,15,15,15,15,15,15, 15,15,15,15,15,15,15,15,
        15,15,15,15,15,15,15,15, 15,15,15,15,15,15,15,15,
        15,15,15,15,15,15,15,15, 15,15,15,15,15,15,15,15,
        ~BLACK_OOO,15,15,15,~(BLACK_OO|BLACK_OOO),15,15,~BLACK_OO
    };
    pos.castlingRights &= castlingMask[from] & castlingMask[to];

    pos.sideToMove = them;
    if (us == BLACK) pos.fullmoveNumber++;
    pos.halfmoveClock++;

    return undo;
}

void MoveGenerator::unmakeMove(Position& pos, Move m, const UndoInfo& undo) const {
    pos.sideToMove = Color(pos.sideToMove ^ 1);
    Color us   = pos.sideToMove;
    Color them = Color(us ^ 1);

    Square from = m.from();
    Square to   = m.to();

    pos.castlingRights  = undo.castlingRights;
    pos.enPassantSquare = undo.enPassantSquare;
    pos.halfmoveClock   = undo.halfmoveClock;
    if (them == BLACK) pos.fullmoveNumber--;

    if (m.isPromotion()) {
        pos.removePiece(to);
        pos.putPiece(Piece(us * 6 + PAWN), from);
        if (undo.captured != NO_PIECE) pos.putPiece(undo.captured, to);

    } else if (m.isEnPassant()) {
        Piece pawn = pos.mailbox[to];
        pos.removePiece(to);
        pos.putPiece(pawn, from);
        pos.putPiece(undo.captured, Square(to + (us == WHITE ? -8 : 8)));

    } else if (m.isCastling()) {
        Piece king = pos.mailbox[to];
        pos.removePiece(to);
        pos.putPiece(king, from);
        Square rookFrom, rookTo;
        if      (to == G1) { rookFrom = H1; rookTo = F1; }
        else if (to == C1) { rookFrom = A1; rookTo = D1; }
        else if (to == G8) { rookFrom = H8; rookTo = F8; }
        else               { rookFrom = A8; rookTo = D8; }
        Piece rook = pos.mailbox[rookTo];
        pos.removePiece(rookTo);
        pos.putPiece(rook, rookFrom);

    } else {
        Piece piece = pos.mailbox[to];
        pos.removePiece(to);
        pos.putPiece(piece, from);
        if (undo.captured != NO_PIECE) pos.putPiece(undo.captured, to);
    }
}

void MoveGenerator::generatePseudoLegal(const Position& pos, MoveList& list) {
    generatePawnMoves(pos, list);
    generateKnightMoves(pos, list);
    generateBishopMoves(pos, list);
    generateRookMoves(pos, list);
    generateQueenMoves(pos, list);
    generateKingMoves(pos, list);
    generateCastlingMoves(pos, list);
}

void MoveGenerator::generateLegalMoves(const Position& pos, MoveList& list) {
    MoveList pseudoList;
    generatePseudoLegal(pos, pseudoList);

    Position posCopy = pos;
    for (Move m : pseudoList) {
        UndoInfo undo = makeMove(posCopy, m);
        Color movedSide = Color(posCopy.sideToMove ^ 1);
        if (!isInCheck(posCopy, movedSide)) list.add(m);
        unmakeMove(posCopy, m, undo);
    }
}