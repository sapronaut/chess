# Chesszzz

It plays chess. It plays chess pretty well actually. It will beat you. Do not let it beat you without a fight.

---

## What is this

This is a fully functional chess engine built from scratch in C++ with a web interface so you can play against it in your browser, or embarrass your friends in multiplayer. No third party chess library was used for the engine itself. Every single legal move, every search, every evaluation — written by hand.

It started as a learning project and slowly became a obsession. There are 8 phases of development committed to this repo and each one is a small miracle.

---

## How it works

The engine is not magic. It is just math done very fast.

**Board representation** uses bitboards — 64-bit integers where each bit is a square. There are 12 of them, one per piece type per color. The entire board fits in a few integers. This is fast.

**Move generation** precomputes attack tables for knights and kings at startup. Sliding pieces (bishops, rooks, queens) iterate outward until they hit something. All 20 starting moves are generated in microseconds.

**Perft testing** proved the move generator correct. At depth 5 from the starting position it counts exactly 4,865,609 leaf nodes. The known correct value is also 4,865,609. This is not a coincidence.

**Evaluation** counts material and adds piece-square bonuses. A knight in the center is worth more than a knight in the corner. A pawn on rank 7 is worth more than a pawn on rank 2. Revolutionary stuff.

**Search** uses negamax with alpha-beta pruning. It assumes both sides play perfectly and cuts off branches that cannot possibly affect the result. This turns a branching factor of 30 into something manageable.

**Iterative deepening** searches depth 1, then 2, then 3, and so on. Each shallow search is nearly free but dramatically improves move ordering for the next depth. The best move from depth 4 is tried first at depth 5. This sounds pointless but it is not.

**Quiescence search** keeps searching captures at leaf nodes until the position is quiet. Without this the engine would evaluate a position where its queen is hanging and say "looks fine." With this it does not make that mistake.

**Transposition table** caches positions using Zobrist hashing. If the engine reaches the same position via different move orders it does not search it again. 64 random numbers XOR'd together. Somehow this works.

**UCI protocol** makes it talk to chess GUIs. It reads from stdin, writes to stdout, and follows a text protocol from the 1990s that every serious chess program still uses.

---

## Tech stack

```
Engine     C++17, bitboards, ~1500 lines, zero dependencies
Backend    Python, FastAPI, WebSockets, python-chess
Frontend   HTML/CSS/JS, chessboard.js, chess.js
```

---

## Running locally

**Build the engine:**
```bash
g++ -std=c++17 -O3 -o SapChess.exe board.cpp movegen.cpp search.cpp uci.cpp main.cpp
```

**Install backend dependencies:**
```bash
cd web/backend
pip install fastapi uvicorn python-chess websockets
```

**Start the server:**
```bash
python -m uvicorn main:app --reload
```

**Open your browser:**
```
http://localhost:8000
```

---

## Playing against it

Click "Play vs Bot". Make a move. Watch it think for a second. Lose.

If you win, the engine was going easy on you. This is not true but it is a comforting thing to believe.

---

## Multiplayer

Click "Play vs Friend". Create a room. Send the room code to your friend. They join. You play chess. The engine watches from a distance, judging your decisions.

---



## Known limitations

- No opening book. The engine has never heard of the Ruy Lopez.
- No endgame tablebases. King and pawn endgames are vibes-based.
- No time management. Tell it depth 6 and it will take however long it takes.
- Multiplayer has no authentication. Anyone with the room code can join. Do not share your room code with people you do not want to play chess with.

---

## Author

Saptarshi. Built this instead of doing something more sensible.

---

## License

MIT. Take the code. Learn from it. Build something better. Come back and tell me about it.
