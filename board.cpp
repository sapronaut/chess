#include "board.h"
#include <iostream>
#include <sstream>
#include <cassert>

// ============================================================
//  POSITION::clear
//  Zero out everything. Called before setFromFEN.
// ============================================================
void Position::clear() {
    for (int c = 0; c < 2; c++)
        for (int pt = 0; pt < 6; pt++)
            pieces[c][pt] = 0;

    occupancy[WHITE] = occupancy[BLACK] = allPieces = 0;

    for (int sq = 0; sq < 64; sq++)
        mailbox[sq] = NO_PIECE;

    sideToMove      = WHITE;
    castlingRights  = NO_CASTLING;
    enPassantSquare = NO_SQUARE;
    halfmoveClock   = 0;
    fullmoveNumber  = 1;
}

// ============================================================
//  POSITION::putPiece
//  Place a piece on a square, keeping all 3 data structures
//  (bitboards, occupancy, mailbox) in sync.
//
//  Why maintain 3 redundant structures?
//  - pieces[][]  -> fast "where are all white rooks?"
//  - occupancy[] -> fast "is any piece blocking this square?"
//  - mailbox[]   -> fast "what's on e4?" (needed in move gen)
// ============================================================
void Position::putPiece(Piece pc, Square sq) {
    assert(pc != NO_PIECE);
    assert(sq >= 0 && sq < 64);
    assert(mailbox[sq] == NO_PIECE);  // square must be empty

    Color     c  = Color(pc / 6);
    PieceType pt = PieceType(pc % 6);

    setBit(pieces[c][pt], sq);
    setBit(occupancy[c],  sq);
    setBit(allPieces,     sq);
    mailbox[sq] = pc;
}

// ============================================================
//  POSITION::removePiece
//  Remove whatever piece is on sq.
// ============================================================
void Position::removePiece(Square sq) {
    Piece pc = mailbox[sq];
    assert(pc != NO_PIECE);

    Color     c  = Color(pc / 6);
    PieceType pt = PieceType(pc % 6);

    clearBit(pieces[c][pt], sq);
    clearBit(occupancy[c],  sq);
    clearBit(allPieces,     sq);
    mailbox[sq] = NO_PIECE;
}

// ============================================================
//  POSITION::setFromFEN
//
//  FEN has 6 space-separated fields:
//    1. Piece placement  (ranks 8 down to 1, '/' between ranks)
//    2. Active color     (w or b)
//    3. Castling rights  (KQkq or -)
//    4. En passant sq    (e3 or -)
//    5. Halfmove clock
//    6. Fullmove number
//
//  Example:
//    rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
// ============================================================
void Position::setFromFEN(const std::string& fen) {
    clear();

    std::istringstream ss(fen);
    std::string piecePlacement, activeColor, castling, enPassant;
    int halfmove, fullmove;

    ss >> piecePlacement >> activeColor >> castling >> enPassant >> halfmove >> fullmove;

    // --- Field 1: piece placement ---
    // FEN starts from rank 8 (top of board) and goes down to rank 1
    // Letters = pieces, digits = empty squares, '/' = next rank
    int rank = 7, file = 0;
    for (char c : piecePlacement) {
        if (c == '/') {
            rank--;
            file = 0;
        } else if (c >= '1' && c <= '8') {
            file += c - '0';  // skip N empty squares
        } else {
            // Map FEN character to Piece enum
            // Uppercase = white, lowercase = black
            Square sq = Square(rank * 8 + file);
            Piece pc = NO_PIECE;
            switch (c) {
                case 'P': pc = W_PAWN;   break;
                case 'N': pc = W_KNIGHT; break;
                case 'B': pc = W_BISHOP; break;
                case 'R': pc = W_ROOK;   break;
                case 'Q': pc = W_QUEEN;  break;
                case 'K': pc = W_KING;   break;
                case 'p': pc = B_PAWN;   break;
                case 'n': pc = B_KNIGHT; break;
                case 'b': pc = B_BISHOP; break;
                case 'r': pc = B_ROOK;   break;
                case 'q': pc = B_QUEEN;  break;
                case 'k': pc = B_KING;   break;
            }
            if (pc != NO_PIECE) putPiece(pc, sq);
            file++;
        }
    }

    // --- Field 2: side to move ---
    sideToMove = (activeColor == "w") ? WHITE : BLACK;

    // --- Field 3: castling rights ---
    castlingRights = NO_CASTLING;
    for (char c : castling) {
        switch (c) {
            case 'K': castlingRights |= WHITE_OO;  break;
            case 'Q': castlingRights |= WHITE_OOO; break;
            case 'k': castlingRights |= BLACK_OO;  break;
            case 'q': castlingRights |= BLACK_OOO; break;
        }
    }

    // --- Field 4: en passant square ---
    enPassantSquare = NO_SQUARE;
    if (enPassant != "-") {
        int epFile = enPassant[0] - 'a';  // 'a'=0, 'b'=1, ..., 'h'=7
        int epRank = enPassant[1] - '1';  // '1'=0, '2'=1, ..., '8'=7
        enPassantSquare = Square(epRank * 8 + epFile);
    }

    // --- Fields 5-6: clocks ---
    halfmoveClock  = halfmove;
    fullmoveNumber = fullmove;
}

// ============================================================
//  POSITION::toFEN
//  Serialize the position back to a FEN string.
//  Useful for saving positions and debugging.
// ============================================================
std::string Position::toFEN() const {
    static const char pieceChars[] = "PNBRQKpnbrqk";
    std::string fen;

    // --- Piece placement ---
    for (int rank = 7; rank >= 0; rank--) {
        int empty = 0;
        for (int file = 0; file < 8; file++) {
            Square sq = Square(rank * 8 + file);
            Piece  pc = mailbox[sq];
            if (pc == NO_PIECE) {
                empty++;
            } else {
                if (empty) { fen += ('0' + empty); empty = 0; }
                fen += pieceChars[pc];
            }
        }
        if (empty) fen += ('0' + empty);
        if (rank > 0) fen += '/';
    }

    // --- Side to move ---
    fen += (sideToMove == WHITE) ? " w " : " b ";

    // --- Castling ---
    std::string cast;
    if (castlingRights & WHITE_OO)  cast += 'K';
    if (castlingRights & WHITE_OOO) cast += 'Q';
    if (castlingRights & BLACK_OO)  cast += 'k';
    if (castlingRights & BLACK_OOO) cast += 'q';
    fen += cast.empty() ? "-" : cast;

    // --- En passant ---
    fen += ' ';
    if (enPassantSquare == NO_SQUARE) {
        fen += '-';
    } else {
        fen += char('a' + enPassantSquare % 8);
        fen += char('1' + enPassantSquare / 8);
    }

    // --- Clocks ---
    fen += ' ';
    fen += std::to_string(halfmoveClock);
    fen += ' ';
    fen += std::to_string(fullmoveNumber);

    return fen;
}

// ============================================================
//  POSITION::print
//  Pretty-print the board. Useful for debugging in the terminal.
//
//  Output looks like:
//    8  r n b q k b n r
//    7  p p p p p p p p
//    6  . . . . . . . .
//    ...
//    1  R N B Q K B N R
//       a b c d e f g h
// ============================================================
void Position::print() const {
    static const char* pieceSymbols[] = {
        "P","N","B","R","Q","K",   // white
        "p","n","b","r","q","k"    // black
    };

    std::cout << "\n";
    for (int rank = 7; rank >= 0; rank--) {
        std::cout << "  " << (rank + 1) << "  ";
        for (int file = 0; file < 8; file++) {
            Square sq = Square(rank * 8 + file);
            Piece  pc = mailbox[sq];
            std::cout << (pc == NO_PIECE ? "." : pieceSymbols[pc]) << " ";
        }
        std::cout << "\n";
    }
    std::cout << "\n     a b c d e f g h\n\n";
    std::cout << "  FEN: " << toFEN() << "\n";
    std::cout << "  Side to move: " << (sideToMove == WHITE ? "White" : "Black") << "\n";

    // Print castling rights in human-readable form
    std::cout << "  Castling: ";
    if (castlingRights == NO_CASTLING) std::cout << "none";
    if (castlingRights & WHITE_OO)  std::cout << "K";
    if (castlingRights & WHITE_OOO) std::cout << "Q";
    if (castlingRights & BLACK_OO)  std::cout << "k";
    if (castlingRights & BLACK_OOO) std::cout << "q";
    std::cout << "\n";

    if (enPassantSquare != NO_SQUARE) {
        char epFile = 'a' + enPassantSquare % 8;
        char epRank = '1' + enPassantSquare / 8;
        std::cout << "  En passant: " << epFile << epRank << "\n";
    }
    std::cout << "\n";
}
