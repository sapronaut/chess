"""
/api/explain endpoint.

Given a board position (FEN) and a move (UCI format), returns the
engine's evaluation, principal variation, and a plain-English
explanation of why the engine considers it a good move.

Phase 1: skeleton only. Engine search wiring is connected, but the
explanation text is a placeholder until Phase 2 (LLM integration).
"""

from __future__ import annotations

import os

from fastapi import APIRouter, HTTPException
from pydantic import BaseModel, Field

from .notation import pv_to_san, score_to_readable, uci_move_to_san
from .uci_engine import UCIEngine, UCIEngineError

router = APIRouter()

ENGINE_BINARY_PATH = os.environ.get("SAPCHESS_ENGINE_PATH", "./sapchess")
SEARCH_DEPTH = int(os.environ.get("SAPCHESS_EXPLAIN_DEPTH", "12"))


class ExplainRequest(BaseModel):
    fen: str | None = Field(
        default=None,
        description="FEN of the position BEFORE the move. Defaults to the start position.",
    )
    move: str = Field(
        description="The move to explain, in UCI format, e.g. 'e2e4'."
    )


class ExplainResponse(BaseModel):
    move_san: str | None
    evaluation: str
    score_cp: int | None
    score_mate: int | None
    principal_variation: list[str]
    explanation: str


@router.post("/api/explain", response_model=ExplainResponse)
def explain_move(request: ExplainRequest) -> ExplainResponse:
    """
    Analyze `request.move` from `request.fen` and return an explanation.

    Phase 1 behavior: runs the engine to get an evaluation and PV for
    the resulting position, but returns a placeholder explanation
    string. Phase 2 will replace the placeholder with an LLM-generated
    explanation.
    """
    move_san = uci_move_to_san(request.fen, request.move)
    if move_san is None:
        raise HTTPException(status_code=400, detail="Illegal or unparseable move for given position")

    engine = UCIEngine(ENGINE_BINARY_PATH)
    try:
        engine.start()
        engine.set_position(fen=request.fen, moves=[request.move])
        result = engine.go(depth=SEARCH_DEPTH)
    except UCIEngineError as exc:
        raise HTTPException(status_code=500, detail=f"Engine error: {exc}") from exc
    finally:
        engine.quit()

    # Determine side to move *after* the played move, for score perspective.
    side_to_move_white = _white_to_move_after(request.fen, request.move)

    evaluation = score_to_readable(result.score_cp, result.score_mate, side_to_move_white)
    post_move_fen = _fen_after_move(request.fen, request.move)
    pv_san = pv_to_san(post_move_fen, result.pv, max_moves=5)

    return ExplainResponse(
        move_san=move_san,
        evaluation=evaluation,
        score_cp=result.score_cp,
        score_mate=result.score_mate,
        principal_variation=pv_san,
        explanation="Explanation generation not yet implemented (Phase 2).",
    )


def _fen_after_move(fen: str | None, move: str) -> str:
    """Return the FEN of the position after `move` is played from `fen`."""
    import chess

    board = chess.Board(fen) if fen else chess.Board()
    board.push_uci(move)
    return board.fen()


def _white_to_move_after(fen: str | None, move: str) -> bool:
    """Return True if it's White to move in the position after `move` is played."""
    import chess

    board = chess.Board(fen) if fen else chess.Board()
    board.push_uci(move)
    return board.turn == chess.WHITE
