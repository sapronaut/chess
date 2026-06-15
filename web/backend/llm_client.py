
from __future__ import annotations

import os

from anthropic import Anthropic, APIError

DEFAULT_MODEL = "claude-haiku-4-5-20251001"
DEFAULT_MAX_TOKENS = 200


class LLMConfigError(RuntimeError):
    """Raised when required LLM configuration is missing."""


class LLMRequestError(RuntimeError):
    """Raised when a call to the LLM API fails."""


def get_client() -> Anthropic:
    """
    Return an Anthropic client configured from the ANTHROPIC_API_KEY
    environment variable.

    Raises:
        LLMConfigError: if ANTHROPIC_API_KEY is not set.
    """
    api_key = os.environ.get("ANTHROPIC_API_KEY")
    if not api_key:
        raise LLMConfigError(
            "ANTHROPIC_API_KEY environment variable is not set. "
            "Set it to a valid Anthropic API key to enable move explanations."
        )
    return Anthropic(api_key=api_key)


def get_model() -> str:
    """Return the configured model name, or a sensible default."""
    return os.environ.get("EXPLAIN_MODEL", DEFAULT_MODEL)


def get_max_tokens() -> int:
    """Return the configured max output tokens, or a sensible default."""
    raw = os.environ.get("EXPLAIN_MAX_TOKENS")
    if raw is None:
        return DEFAULT_MAX_TOKENS
    try:
        return int(raw)
    except ValueError:
        return DEFAULT_MAX_TOKENS


def generate_text(prompt: str, system: str | None = None) -> str:
    """
    Send `prompt` (and optional `system` prompt) to the configured
    model and return the response text.

    Raises:
        LLMConfigError: if the API key is missing.
        LLMRequestError: if the API call fails for any other reason.
    """
    client = get_client()

    try:
        response = client.messages.create(
            model=get_model(),
            max_tokens=get_max_tokens(),
            system=system or "",
            messages=[{"role": "user", "content": prompt}],
        )
    except APIError as exc:
        raise LLMRequestError(f"Anthropic API request failed: {exc}") from exc

    text_parts = [block.text for block in response.content if block.type == "text"]
    return "".join(text_parts).strip()
