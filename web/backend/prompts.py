"""
Prompt template for move explanations.

Builds a concise, well-structured prompt from a position, the move
played, the engine's evaluation, and its principal variation, asking
the LLM to explain the move in plain English.
"""

from __future__ import annotations

SYSTEM_PROMPT = (
    "You are a chess coach explaining engine analysis to an intermediate "
    "player. Given a position, a move, the engine's evaluation, and its "
    "planned continuation, explain in 2-3 sentences why the move is good "
    "(or what it accomplishes). Focus on concrete ideas: piece activity, "
    "king safety, pawn structure, tactical threats, or plans. Avoid vague "
    "phrases like 'this is a good move'. Do not restate the move notation "
    "verbatim more than once. Keep it concise and conversational."
)


def build_explanation_prompt(
    fen_before: str,
    move_san: str,
    evaluation: str,
    principal_variation: list[str],
) -> str:
    """
    Build the user prompt for explaining a move.

    Args:
        fen_before: FEN of the position before the move was played.
        move_san: the move in SAN notation, e.g. "Nf3".
        evaluation: human-readable evaluation after the move, e.g. "+0.35".
        principal_variation: engine's planned continuation in SAN, e.g.
            ["e5", "Nf3", "Nc6"].

    Returns:
        A prompt string ready to send to the LLM.
    """
    pv_text = " ".join(principal_variation) if principal_variation else "(none given)"

    return (
        f"Position (FEN): {fen_before}\n"
        f"Move played: {move_san}\n"
        f"Engine evaluation after this move: {evaluation}\n"
        f"Engine's planned continuation: {pv_text}\n\n"
        f"Explain why {move_san} is a reasonable move here."
    )
