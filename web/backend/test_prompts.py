"""Unit tests for backend.prompts"""

from backend.prompts import SYSTEM_PROMPT, build_explanation_prompt


def test_build_explanation_prompt_includes_all_fields():
    prompt = build_explanation_prompt(
        fen_before="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        move_san="e4",
        evaluation="+0.35",
        principal_variation=["e5", "Nf3", "Nc6"],
    )

    assert "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" in prompt
    assert "e4" in prompt
    assert "+0.35" in prompt
    assert "e5 Nf3 Nc6" in prompt


def test_build_explanation_prompt_handles_empty_pv():
    prompt = build_explanation_prompt(
        fen_before="8/8/8/8/8/8/8/8 w - - 0 1",
        move_san="Kf1",
        evaluation="0.00",
        principal_variation=[],
    )

    assert "(none given)" in prompt


def test_system_prompt_is_nonempty():
    assert len(SYSTEM_PROMPT) > 0
    assert "chess" in SYSTEM_PROMPT.lower()
