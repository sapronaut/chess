"""
UCI engine wrapper.

Spawns the SapChess engine binary as a subprocess and communicates
with it via the UCI (Universal Chess Interface) protocol over
stdin/stdout.

Usage:
    engine = UCIEngine("/path/to/sapchess")
    engine.start()
    engine.set_position(fen="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1")
    result = engine.go(depth=12)
    # result.best_move -> "e2e4"
    # result.score_cp  -> 35
    # result.pv        -> ["e2e4", "e7e5", "g1f3", ...]
    engine.quit()
"""

from __future__ import annotations

import subprocess
import threading
from dataclasses import dataclass, field


@dataclass
class SearchResult:
    """Parsed result of a UCI 'go' search."""

    best_move: str | None = None
    score_cp: int | None = None
    score_mate: int | None = None
    depth: int = 0
    pv: list[str] = field(default_factory=list)


class UCIEngineError(RuntimeError):
    """Raised when the engine process misbehaves or can't be started."""


class UCIEngine:
    """Thin wrapper around a UCI-compatible chess engine subprocess."""

    def __init__(self, binary_path: str, startup_timeout: float = 5.0):
        self.binary_path = binary_path
        self.startup_timeout = startup_timeout
        self._proc: subprocess.Popen | None = None
        self._lock = threading.Lock()

    def start(self) -> None:
        """Launch the engine process and perform the UCI handshake."""
        try:
            self._proc = subprocess.Popen(
                [self.binary_path],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                bufsize=1,  # line-buffered
            )
        except OSError as exc:
            raise UCIEngineError(
                f"Failed to start engine at '{self.binary_path}': {exc}"
            ) from exc

        self._send("uci")
        self._wait_for("uciok")
        self._send("isready")
        self._wait_for("readyok")

    def set_position(self, fen: str | None = None, moves: list[str] | None = None) -> None:
        """Set the board position via FEN and/or a move list from startpos."""
        if fen:
            cmd = f"position fen {fen}"
        else:
            cmd = "position startpos"
        if moves:
            cmd += " moves " + " ".join(moves)
        self._send(cmd)

    def go(self, depth: int = 12, movetime_ms: int | None = None) -> SearchResult:
        """
        Run a search and return the best move, evaluation, and principal
        variation (PV).

        Parses the final 'info' line before 'bestmove' for score/PV, and
        the 'bestmove' line for the chosen move.
        """
        if movetime_ms is not None:
            cmd = f"go movetime {movetime_ms}"
        else:
            cmd = f"go depth {depth}"

        self._send(cmd)

        result = SearchResult()

        while True:
            line = self._readline()
            if line is None:
                raise UCIEngineError("Engine process ended unexpectedly during search")

            if line.startswith("info"):
                self._parse_info_line(line, result)
            elif line.startswith("bestmove"):
                parts = line.split()
                if len(parts) >= 2:
                    result.best_move = parts[1]
                break

        return result

    def quit(self) -> None:
        """Send 'quit' and terminate the engine process."""
        if self._proc is None:
            return
        try:
            self._send("quit")
        except Exception:
            pass
        finally:
            self._proc.terminate()
            self._proc = None

    # -- internal helpers -------------------------------------------------

    def _send(self, command: str) -> None:
        if self._proc is None or self._proc.stdin is None:
            raise UCIEngineError("Engine process is not running")
        self._proc.stdin.write(command + "\n")
        self._proc.stdin.flush()

    def _readline(self) -> str | None:
        if self._proc is None or self._proc.stdout is None:
            raise UCIEngineError("Engine process is not running")
        line = self._proc.stdout.readline()
        if line == "":
            return None
        return line.strip()

    def _wait_for(self, token: str) -> None:
        """Read lines until one starts with the given token."""
        while True:
            line = self._readline()
            if line is None:
                raise UCIEngineError(f"Engine exited before sending '{token}'")
            if line.startswith(token):
                return

    @staticmethod
    def _parse_info_line(line: str, result: SearchResult) -> None:
        """
        Parse a UCI 'info' line, extracting depth, score (cp or mate),
        and the principal variation.

        Example line:
            info depth 12 score cp 35 pv e2e4 e7e5 g1f3 g8f6
            info depth 8 score mate 3 pv d1h5 g6h5 ...
        """
        tokens = line.split()
        i = 0
        while i < len(tokens):
            token = tokens[i]
            if token == "depth" and i + 1 < len(tokens):
                try:
                    result.depth = int(tokens[i + 1])
                except ValueError:
                    pass
                i += 2
            elif token == "score" and i + 2 < len(tokens):
                kind, value = tokens[i + 1], tokens[i + 2]
                try:
                    if kind == "cp":
                        result.score_cp = int(value)
                        result.score_mate = None
                    elif kind == "mate":
                        result.score_mate = int(value)
                        result.score_cp = None
                except ValueError:
                    pass
                i += 3
            elif token == "pv":
                result.pv = tokens[i + 1:]
                break
            else:
                i += 1
