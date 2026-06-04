#pragma once
#include <cstdint>
#include <string>

// ============================================================
//  TYPES
//  A chess engine is a numbers game. Every concept maps to an
//  integer type - no classes with virtual dispatch, no strings
//  in hot paths.
// ============================================================

// A Bitboard is a 64-bit integer.
// Each bit represents one square on the board.
//
// Bit layout (little-endian rank-file mapping):
//   bit 0  = a1,  bit 1  = b1, ... bit 7  = h1   (rank 1)
//   bit 8  = a2,  bit 9  = b2, ... bit 15 = h2   (rank 2)
//   ...
//   bit 56 = a8,  bit 57 = b8, ... bit 63 = h8   (rank 8)
//
// Example: white pawns on the starting position
//   0x000000000000FF00  (all 8 bits of rank 2 are set)
using Bitboard = uint64_t;

// A Square is an index 0-63 (a1=0, h8=63)
using Square = int;

// Piece types (0-5). No color info here.
enum PieceType { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, NO_PIECE_TYPE };

// Colors
enum Color { WHITE, BLACK, NO_COLOR };

// A Piece encodes both color and type: 0-5 = white pieces, 6-11 = black pieces
// Formula:  piece = color * 6 + pieceType
// Decode:   type  = piece % 6,  color = piece / 6
enum Piece {
    W_PAWN, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
    B_PAWN, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING,
    NO_PIECE
};

// Castling rights stored as 4 flags packed in one byte
// bit 0 = white kingside, bit 1 = white queenside
// bit 2 = black kingside, bit 3 = black queenside
enum CastlingRight {
    NO_CASTLING       = 0,
    WHITE_OO          = 1,   // white kingside  (O-O)
    WHITE_OOO         = 2,   // white queenside (O-O-O)
    BLACK_OO          = 4,
    BLACK_OOO         = 8,
    ALL_CASTLING      = 15
};

// Named square constants - makes code readable
enum Squares : Square {
    A1=0,  B1, C1, D1, E1, F1, G1, H1,
    A2=8,  B2, C2, D2, E2, F2, G2, H2,
    A3=16, B3, C3, D3, E3, F3, G3, H3,
    A4=24, B4, C4, D4, E4, F4, G4, H4,
    A5=32, B5, C5, D5, E5, F5, G5, H5,
    A6=40, B6, C6, D6, E6, F6, G6, H6,
    A7=48, B7, C7, D7, E7, F7, G7, H7,
    A8=56, B8, C8, D8, E8, F8, G8, H8,
    NO_SQUARE = 64
};

// ============================================================
//  BITBOARD HELPERS
//  These are tiny inline functions. The compiler inlines them
//  and they compile down to 1-2 CPU instructions each.
// ============================================================

// Set/clear/test a single bit (square) in a bitboard
inline Bitboard squareBB(Square s)             { return Bitboard(1) << s; }
inline void     setBit(Bitboard& bb, Square s)  { bb |= squareBB(s); }
inline void     clearBit(Bitboard& bb, Square s){ bb &= ~squareBB(s); }
inline bool     testBit(Bitboard bb, Square s)  { return (bb >> s) & 1; }

// Count set bits ("popcount") - tells you how many pieces are on the board
// __builtin_popcountll is a GCC/Clang intrinsic that maps to a single CPU instruction
inline int popcount(Bitboard bb) { return __builtin_popcountll(bb); }

// Find the index of the lowest set bit (least significant bit)
// Used to iterate over pieces: "give me the next piece square"
// __builtin_ctzll = "count trailing zeros" - also a single CPU instruction
inline Square lsb(Bitboard bb) { return Square(__builtin_ctzll(bb)); }

// Pop the lowest set bit: returns its square AND removes it from bb
// Classic pattern for iterating all pieces:
//   while (bb) { Square s = popLSB(bb); /* do something with s */ }
inline Square popLSB(Bitboard& bb) {
    Square s = lsb(bb);
    bb &= bb - 1;   // clears the lowest set bit (bit trick)
    return s;
}

// File/rank masks - useful for pawn logic, passed pawns, etc.
constexpr Bitboard FILE_A = 0x0101010101010101ULL;  // a1,a2,...,a8
constexpr Bitboard FILE_H = FILE_A << 7;
constexpr Bitboard RANK_1 = 0x00000000000000FFULL;  // a1-h1
constexpr Bitboard RANK_2 = RANK_1 << 8;
constexpr Bitboard RANK_7 = RANK_1 << 48;
constexpr Bitboard RANK_8 = RANK_1 << 56;

// ============================================================
//  POSITION
//  Everything needed to fully describe a chess position.
//  No history here - that lives in the search stack.
// ============================================================
struct Position {
    // 12 bitboards: one per (color, pieceType) combination
    // Access: pieces[color][pieceType]
    // e.g. pieces[WHITE][ROOK] = bitboard of all white rooks
    Bitboard pieces[2][6];

    // Convenience: all pieces of one color combined
    Bitboard occupancy[2];

    // All pieces on the board (occupancy[WHITE] | occupancy[BLACK])
    // Kept in sync - critical for fast sliding piece attack generation
    Bitboard allPieces;

    // Mailbox: for each square, which piece is on it?
    // Complement to bitboards. Bitboards answer "where are all rooks?"
    // Mailbox answers "what piece is on e4?"
    Piece mailbox[64];

    // Whose turn is it?
    Color sideToMove;

    // Castling availability (bitmask of CastlingRight flags)
    int castlingRights;

    // En passant target square (NO_SQUARE if not available)
    // This is the square a pawn can capture TO, not the captured pawn's square
    Square enPassantSquare;

    // Halfmove clock: moves since last capture or pawn move
    // Used for the 50-move draw rule (draw at 100 halfmoves)
    int halfmoveClock;

    // Fullmove number: starts at 1, increments after Black's move
    int fullmoveNumber;

    // --------------------------------------------------------
    //  Methods declared here, defined in board.cpp
    // --------------------------------------------------------

    // Reset to empty board
    void clear();

    // Load a position from a FEN string
    // FEN = Forsyth-Edwards Notation, the standard way to describe positions
    // Starting position FEN:
    //   "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
    void setFromFEN(const std::string& fen);

    // Place/remove a piece on a square (keeps all data structures in sync)
    void putPiece(Piece pc, Square sq);
    void removePiece(Square sq);

    // Pretty-print the board to stdout (great for debugging)
    void print() const;

    // Return FEN string for this position
    std::string toFEN() const;

    // Derived helpers
    Bitboard getPieces(Color c, PieceType pt) const { return pieces[c][pt]; }
    Bitboard getOccupancy(Color c)            const { return occupancy[c]; }
    Piece    pieceOn(Square sq)               const { return mailbox[sq]; }
    Color    colorOf(Piece pc)                const { return Color(pc / 6); }
    PieceType typeOf(Piece pc)                const { return PieceType(pc % 6); }
};

// The standard starting position FEN string
constexpr const char* START_FEN =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
