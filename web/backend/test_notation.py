"""Unit tests for backend.notation"""

import chess
import pytest

from backend.notation import pv_to_san, score_to_readable, uci_move_to_san


def test_uci_move_to_san_opening():
    # e2e4 from the start position is "e4"
    assert uci_move_to_san(None, "e2e4") == "e4"


def test_uci_move_to_san_illegal_returns_none():
    # e2e5 is not a legal pawn move
    assert uci_move_to_san(None, "e2e5") is None


def test_pv_to_san_sequence():
    pv = ["e2e4", "e7e5", "g1f3"]
    san = pv_to_san(None, pv)
    assert san == ["e4", "e5", "Nf3"]


def test_pv_to_san_stops_on_illegal_move():
    pv = ["e2e4", "a1a8"]  # second move illegal from this position
    san = pv_to_san(None, pv)
    assert san == ["e4"]


def test_pv_to_san_respects_max_moves():
    pv = ["e2e4", "e7e5", "g1f3", "g8f6"]
    san = pv_to_san(None, pv, max_moves=2)
    assert san == ["e4", "e5"]


def test_score_to_readable_cp_white_to_move():
    # +35 cp reported with White to move -> +0.35 for White
    assert score_to_readable(score_cp=35, score_mate=None, side_to_move_white=True) == "+0.35"


def test_score_to_readable_cp_black_to_move_flips_sign():
    # +35 cp reported with Black to move means it's good for Black -> -0.35 for White
    assert score_to_readable(score_cp=35, score_mate=None, side_to_move_white=False) == "-0.35"


def test_score_to_readable_mate_for_white():
    result = score_to_readable(score_cp=None, score_mate=3, side_to_move_white=True)
    assert result == "Mate in 3 for White"


def test_score_to_readable_mate_for_black_when_black_to_move():
    # Positive mate score reported with Black to move -> mate for Black
    result = score_to_readable(score_cp=None, score_mate=3, side_to_move_white=False)
    assert result == "Mate in 3 for Black"


def test_score_to_readable_unknown_when_no_score():
    assert score_to_readable(score_cp=None, score_mate=None, side_to_move_white=True) == "Unknown"
