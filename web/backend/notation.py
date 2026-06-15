"""
Helpers for converting UCI-format moves (e.g. "e2e4", "e7e8q") into
human-readable SAN notation (e.g. "e4", "exe8=Q+"), given a board position.

Used to turn an engine's principal variation into something readable
enough to hand to an LLM (and to display in the UI if needed).
"""

from __future__ import annotations

import chess


def fen_to_board(fen: str | None) -> chess.Board:
    """Return a python-chess Board for the given FEN, or the start position."""
    if fen:
        return chess.Board(fen)
    return chess.Board()


def pv_to_san(fen: str | None, pv: list[str], max_moves: int | None = None) -> list[str]:
    """
    Convert a UCI principal variation (list of moves like 'e2e4') into
    SAN notation (e.g. 'e4'), playing each move on a copy of the board
    starting from `fen`.

    Stops early (without raising) if it encounters an illegal/unparseable
    move, returning whatever was successfully converted so far.

    Args:
        fen: starting position, or None for the standard start position.
        pv: list of UCI moves, e.g. ["e2e4", "e7e5", "g1f3"].
        max_moves: optional cap on how many PV moves to convert.

    Returns:
        List of SAN strings, e.g. ["e4", "e5", "Nf3"].
    """
    board = fen_to_board(fen)
    san_moves: list[str] = []

    moves = pv if max_moves is None else pv[:max_moves]

    for uci_move in moves:
        try:
            move = chess.Move.from_uci(uci_move)
        except (ValueError, chess.InvalidMoveError):
            break

        if move not in board.legal_moves:
            break

        san_moves.append(board.san(move))
        board.push(move)

    return san_moves


def uci_move_to_san(fen: str | None, uci_move: str) -> str | None:
    """
    Convert a single UCI move (e.g. 'g1f3') to SAN (e.g. 'Nf3') for the
    given position. Returns None if the move is illegal or unparseable.
    """
    board = fen_to_board(fen)
    try:
        move = chess.Move.from_uci(uci_move)
    except (ValueError, chess.InvalidMoveError):
        return None

    if move not in board.legal_moves:
        return None

    return board.san(move)


def score_to_readable(score_cp: int | None, score_mate: int | None, side_to_move_white: bool) -> str:
    """
    Turn a raw centipawn/mate score into a human-readable evaluation
    string from White's perspective, e.g. "+0.35", "-1.20", "Mate in 3".

    UCI scores are reported from the perspective of the side to move,
    so this flips sign when it's Black to move.
    """
    if score_mate is not None:
        mate_in = abs(score_mate)
        favored = "White" if (score_mate > 0) == side_to_move_white else "Black"
        return f"Mate in {mate_in} for {favored}"

    if score_cp is None:
        return "Unknown"

    cp = score_cp if side_to_move_white else -score_cp
    pawns = cp / 100.0
    sign = "+" if pawns >= 0 else ""
    return f"{sign}{pawns:.2f}"
