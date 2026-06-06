import subprocess
import threading
import queue
import os

# ============================================================
#  UCI ENGINE WRAPPER
#  Spawns SapChess.exe as a subprocess and communicates
#  via stdin/stdout using the UCI protocol.
#  Thread-safe: uses a queue for reading engine output.
# ============================================================

class UCIEngine:
    def __init__(self, engine_path: str):
        self.process = subprocess.Popen(
            engine_path,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1  # line buffered
        )
        self.output_queue = queue.Queue()
        self.running = True

        # Read engine output in a background thread
        # (engine output is async - we can't block waiting for it)
        self.reader_thread = threading.Thread(
            target=self._read_output,
            daemon=True
        )
        self.reader_thread.start()

        # Initialize UCI handshake
        self._send("uci")
        self._wait_for("uciok")
        self._send("isready")
        self._wait_for("readyok")

    def _read_output(self):
        """Background thread: reads engine stdout into queue"""
        for line in self.process.stdout:
            line = line.strip()
            if line:
                self.output_queue.put(line)

    def _send(self, command: str):
        """Send a command to the engine"""
        self.process.stdin.write(command + "\n")
        self.process.stdin.flush()

    def _wait_for(self, expected: str, timeout: float = 5.0) -> list[str]:
        """Read lines until we see the expected token"""
        lines = []
        import time
        start = time.time()
        while time.time() - start < timeout:
            try:
                line = self.output_queue.get(timeout=0.1)
                lines.append(line)
                if expected in line:
                    return lines
            except queue.Empty:
                continue
        return lines

    def new_game(self):
        """Reset engine for a new game"""
        self._send("ucinewgame")
        self._send("isready")
        self._wait_for("readyok")

    def get_best_move(self, moves: list[str], depth: int = 6) -> str:
        """
        Given a list of moves played so far (in UCI format),
        return the engine's best move.

        moves: ["e2e4", "e7e5", "g1f3", ...]
        returns: "b8c6" (UCI move string)
        """
        # Set up position
        if moves:
            move_str = " ".join(moves)
            self._send(f"position startpos moves {move_str}")
        else:
            self._send("position startpos")

        # Search
        self._send(f"go depth {depth}")

        # Wait for bestmove
        lines = self._wait_for("bestmove", timeout=30.0)
        for line in lines:
            if line.startswith("bestmove"):
                return line.split()[1]

        return None

    def quit(self):
        """Shut down the engine cleanly"""
        self._send("quit")
        self.process.wait(timeout=2)