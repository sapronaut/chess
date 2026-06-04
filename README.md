# ♟️ Chess Engine

> Because apparently playing chess wasn't enough — I had to teach a computer how to do it too.

A chess engine built from scratch in **C++**, one bug, one blunder, and one questionable design decision at a time.

This repository documents the journey of building a functional chess engine from the ground up, starting with move generation and slowly evolving into something that (hopefully) doesn't hang its queen for free.

---

## Current Progress

* [x] Board representation
* [x] FEN parsing
* [x] Move generation
* [x] Make / unmake move
* [x] Perft testing
* [ ] Evaluation function
* [ ] Alpha-Beta search
* [ ] Move ordering
* [ ] Transposition tables
* [ ] UCI support
* [ ] Beat a human who knows what they're doing

---

## Why?

Three reasons:

1. Chess engines are cool.
2. C++ is fast.
3. I enjoy debugging mistakes that only occur 7 moves deep in a position from 1978.

---

## Features

### Board Representation

Efficient internal representation of chess positions.

### Move Generation

Generates legal moves while attempting not to violate the laws of chess.

### Perft Validation

Used to verify move generation correctness against known test positions.

Current test positions include:

* Starting Position
* Kiwipete

---

## Project Structure

```text
.
├── main.cpp
├── movegen.h
├── board.h
├── perft.h
└── ...
```

---

## Building

```bash
g++ -std=c++17 -O2 main.cpp -o chess-engine
```

Run:

```bash
./chess-engine
```

Windows:

```powershell
.\chess-engine.exe
```

---

## Roadmap

### Phase 1 — Foundation

* Board representation
* FEN parsing
* Move generation
* Perft testing

### Phase 2 — Search

* Minimax
* Alpha-Beta pruning
* Iterative deepening
* Move ordering

### Phase 3 — Strength

* Transposition tables
* Killer moves
* History heuristic
* Quiescence search

### Phase 4 — Engine Life Support

* UCI protocol
* Time management
* Opening book
* Endgame improvements

---

## Performance Goal

Reach a level where:

* Random move generator gets destroyed
* Casual players get punished
* Friends stop accepting challenges
* Stockfish remains completely unimpressed

---

## Notes

If the engine plays a brilliant sacrifice, it was intentional.

If it hangs a queen, that was a temporary exploratory feature.

---

Built with C++, caffeine, and an unreasonable amount of enthusiasm for a board game that's older than most countries.
