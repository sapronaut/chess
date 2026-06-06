#include "uci.h"
#include <iostream>

UCI::UCI() {
    pos.setFromFEN(START_FEN);
}

// ============================================================
//  MAIN UCI LOOP
//  Reads one line at a time from stdin.
//  Dispatches to the right handler based on the command.
// ============================================================
void UCI::loop() {
    std::string line, token;

    while (std::getline(std::cin, line)) {
        std::istringstream ss(line);
        ss >> token;

        if (token == "uci")          handleUCI();
        else if (token == "isready") handleIsReady();
        else if (token == "ucinewgame") handleNewGame();
        else if (token == "position") handlePosition(ss);
        else if (token == "go")      handleGo(ss);
        else if (token == "quit")    break;
        else if (token == "d")       pos.print(); // debug: print board

        std::cout.flush();
    }
}

// ============================================================
//  UCI COMMAND: uci
//  Engine introduces itself and lists options.
// ============================================================
void UCI::handleUCI() {
    std::cout << "id name SapChess\n";
    std::cout << "id author Saptarshi\n";
    std::cout << "\n";
    // Options could go here (hash size, threads, etc.)
    std::cout << "uciok\n";
}

// ============================================================
//  UCI COMMAND: isready
//  GUI asks if engine is ready. Always respond readyok.
//  (In complex engines this might wait for loading opening books etc.)
// ============================================================
void UCI::handleIsReady() {
    std::cout << "readyok\n";
}

// ============================================================
//  UCI COMMAND: ucinewgame
//  Reset everything for a new game.
// ============================================================
void UCI::handleNewGame() {
    pos.setFromFEN(START_FEN);
}

// ============================================================
//  UCI COMMAND: position
//  Two forms:
//    "position startpos"
//    "position startpos moves e2e4 e7e5 ..."
//    "position fen <fenstring>"
//    "position fen <fenstring> moves e2e4 ..."
//
//  We set up the position then apply each move in order.
// ============================================================
void UCI::handlePosition(std::istringstream& ss) {
    std::string token;
    ss >> token;

    if (token == "startpos") {
        pos.setFromFEN(START_FEN);
        ss >> token; // consume "moves" if present
    } else if (token == "fen") {
        // FEN is next 6 tokens
        std::string fen, part;
        for (int i = 0; i < 6 && ss >> part; i++)
            fen += (i ? " " : "") + part;
        pos.setFromFEN(fen);
        ss >> token; // consume "moves" if present
    }

    // Apply each move
    // token should now be "moves" or nothing
    if (token == "moves") {
        MoveGenerator gen;
        std::string moveStr;
        while (ss >> moveStr) {
            Move m = parseMove(moveStr);
            if (!m.isNull()) {
                gen.makeMove(pos, m);
            }
        }
    }
}

// ============================================================
//  UCI COMMAND: go
//  Start searching. Many parameters possible (movetime, wtime,
//  btime, depth, infinite...). We handle depth and movetime.
//
//  Must output: "bestmove <move>"
// ============================================================
void UCI::handleGo(std::istringstream& ss) {
    std::string token;
    int depth    = 6;    // default depth
    int movetime = -1;   // ms per move (-1 = not set)

    while (ss >> token) {
        if (token == "depth" && ss >> token)
            depth = std::stoi(token);
        else if (token == "movetime" && ss >> token)
            movetime = std::stoi(token);
        else if (token == "infinite")
            depth = 8;  // cap at 8 for infinite (no time management yet)
    }

    SearchResult result = search.findBestMove(pos, depth);

    std::cout << "bestmove " << result.bestMove.toString() << "\n";
}

// ============================================================
//  PARSE MOVE
//  Convert UCI move string to Move object.
//  Format: "e2e4", "e7e8q" (promotion)
//
//  We generate all legal moves and find the matching one.
//  This handles all edge cases (castling encoded as king move,
//  en passant, promotions) automatically.
// ============================================================
Move UCI::parseMove(const std::string& s) {
    if (s.size() < 4) return Move();

    int fromFile = s[0] - 'a';
    int fromRank = s[1] - '1';
    int toFile   = s[2] - 'a';
    int toRank   = s[3] - '1';

    Square from = Square(fromRank * 8 + fromFile);
    Square to   = Square(toRank   * 8 + toFile);

    // Promotion piece
    PromoPiece promo = PROMO_QUEEN;
    if (s.size() == 5) {
        switch (s[4]) {
            case 'n': promo = PROMO_KNIGHT; break;
            case 'b': promo = PROMO_BISHOP; break;
            case 'r': promo = PROMO_ROOK;   break;
            case 'q': promo = PROMO_QUEEN;  break;
        }
    }

    // Find the matching legal move
    MoveGenerator gen;
    MoveList list;
    gen.generateLegalMoves(pos, list);

    for (Move m : list) {
        if (m.from() == from && m.to() == to) {
            if (!m.isPromotion() || m.promo() == promo)
                return m;
        }
    }

    return Move(); // null move if not found
}