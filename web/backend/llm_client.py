"""
LLM client setup for move explanations.

Uses the Groq API (free tier) for fast inference.

Environment variables:
    GROQ_API_KEY       - required, your Groq API key (free at console.groq.com)
    EXPLAIN_MODEL      - optional, defaults to "llama-3.1-8b-instant"
    EXPLAIN_MAX_TOKENS - optional, defaults to 200
"""

from __future__ import annotations

import os

from groq import Groq, APIError

DEFAULT_MODEL = "llama-3.1-8b-instant"
DEFAULT_MAX_TOKENS = 200


class LLMConfigError(RuntimeError):
    """Raised when required LLM configuration is missing."""


class LLMRequestError(RuntimeError):
    """Raised when a call to the LLM API fails."""


def get_client() -> Groq:
    api_key = os.environ.get("GROQ_API_KEY")
    if not api_key:
        raise LLMConfigError(
            "GROQ_API_KEY environment variable is not set. "
            "Get a free key at https://console.groq.com and set it in your .env."
        )
    return Groq(api_key=api_key)


def get_model() -> str:
    return os.environ.get("EXPLAIN_MODEL", DEFAULT_MODEL)


def get_max_tokens() -> int:
    raw = os.environ.get("EXPLAIN_MAX_TOKENS")
    if raw is None:
        return DEFAULT_MAX_TOKENS
    try:
        return int(raw)
    except ValueError:
        return DEFAULT_MAX_TOKENS


def generate_text(prompt: str, system: str | None = None) -> str:
    """
    Send `prompt` to Groq and return the response text.

    Raises:
        LLMConfigError: if GROQ_API_KEY is not set.
        LLMRequestError: if the API call fails.
    """
    client = get_client()

    try:
        response = client.chat.completions.create(
            model=get_model(),
            max_tokens=get_max_tokens(),
            messages=[
                {"role": "system", "content": system or ""},
                {"role": "user", "content": prompt},
            ],
        )
    except APIError as exc:
        raise LLMRequestError(f"Groq API request failed: {exc}") from exc

    return response.choices[0].message.content.strip()
