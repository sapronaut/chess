#pragma once
#include "search.h"
#include <string>
#include <sstream>

// ============================================================
//  UCI - Universal Chess Interface
//
//  A simple stdin/stdout text protocol.
//  Every chess GUI (Arena, Cute Chess, Lichess bot) speaks UCI.
//  Once this is done you can play against your own engine!
//
//  Flow:
//    GUI sends "uci"           → engine sends id + uciok
//    GUI sends "isready"       → engine sends readyok
//    GUI sends "position ..."  → engine sets up the board
//    GUI sends "go depth N"    → engine searches and sends bestmove
//    GUI sends "quit"          → engine exits
// ============================================================

class UCI {
public:
    UCI();
    void loop();  // main UCI loop - reads stdin, writes stdout

private:
    Position pos;
    Search   search;

    // Command handlers
    void handleUCI();
    void handleIsReady();
    void handlePosition(std::istringstream& ss);
    void handleGo(std::istringstream& ss);
    void handleNewGame();

    // Parse a move string like "e2e4" or "e7e8q" into a Move
    Move parseMove(const std::string& moveStr);
};