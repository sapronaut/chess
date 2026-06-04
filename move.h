#pragma once
#include "board.h"

enum MoveType {
    NORMAL     = 0,
    CASTLING   = 1,
    EN_PASSANT = 2,
    PROMOTION  = 3
};

enum PromoPiece { PROMO_KNIGHT = 0, PROMO_BISHOP = 1, PROMO_ROOK = 2, PROMO_QUEEN = 3 };

struct Move {
    uint32_t data;

    Move() : data(0) {}
    Move(Square from, Square to, MoveType type = NORMAL, PromoPiece promo = PROMO_QUEEN)
        : data(from | (to << 6) | (type << 12) | (promo << 15)) {}

    Square    from()  const { return Square(data & 0x3F); }
    Square    to()    const { return Square((data >> 6) & 0x3F); }
    MoveType  type()  const { return MoveType((data >> 12) & 0x7); }
    PromoPiece promo() const { return PromoPiece((data >> 15) & 0x3); }

    bool isPromotion() const { return type() == PROMOTION; }
    bool isCastling()  const { return type() == CASTLING; }
    bool isEnPassant() const { return type() == EN_PASSANT; }
    bool isNull()      const { return data == 0; }

    std::string toString() const {
        static const char* sqNames[] = {
            "a1","b1","c1","d1","e1","f1","g1","h1",
            "a2","b2","c2","d2","e2","f2","g2","h2",
            "a3","b3","c3","d3","e3","f3","g3","h3",
            "a4","b4","c4","d4","e4","f4","g4","h4",
            "a5","b5","c5","d5","e5","f5","g5","h5",
            "a6","b6","c6","d6","e6","f6","g6","h6",
            "a7","b7","c7","d7","e7","f7","g7","h7",
            "a8","b8","c8","d8","e8","f8","g8","h8"
        };
        static const char promoChars[] = "nbrq";
        std::string s = sqNames[from()];
        s += sqNames[to()];
        if (isPromotion()) s += promoChars[promo()];
        return s;
    }

    bool operator==(const Move& o) const { return data == o.data; }
    bool operator!=(const Move& o) const { return data != o.data; }
};

struct MoveList {
    Move moves[256];
    int  count = 0;

    void add(Move m) { moves[count++] = m; }
    void add(Square from, Square to, MoveType t = NORMAL, PromoPiece p = PROMO_QUEEN) {
        add(Move(from, to, t, p));
    }

    Move* begin() { return moves; }
    Move* end()   { return moves + count; }
    const Move* begin() const { return moves; }
    const Move* end()   const { return moves + count; }

    int  size()  const { return count; }
    bool empty() const { return count == 0; }
};