"""
Simple in-memory cache for move explanations.

Keyed on (fen_before, move) so repeated requests for the same
position/move pair don't re-call the LLM. This is intentionally a
basic process-local cache — good enough for a single backend
instance and demo/portfolio use. Swap for Redis or similar if you
need it to survive restarts or scale across instances.
"""

from __future__ import annotations

import threading

_cache: dict[tuple[str, str], str] = {}
_lock = threading.Lock()

MAX_CACHE_SIZE = 1000


def make_key(fen_before: str, move: str) -> tuple[str, str]:
    """Build a cache key from the position FEN and the move played."""
    return (fen_before, move)


def get(fen_before: str, move: str) -> str | None:
    """Return a cached explanation, or None if not present."""
    key = make_key(fen_before, move)
    with _lock:
        return _cache.get(key)


def set(fen_before: str, move: str, explanation: str) -> None:
    """
    Store an explanation in the cache.

    If the cache has grown beyond MAX_CACHE_SIZE, the oldest entry
    (insertion order) is evicted first.
    """
    key = make_key(fen_before, move)
    with _lock:
        if key not in _cache and len(_cache) >= MAX_CACHE_SIZE:
            oldest_key = next(iter(_cache))
            del _cache[oldest_key]
        _cache[key] = explanation


def clear() -> None:
    """Remove all cached explanations. Mainly useful for tests."""
    with _lock:
        _cache.clear()


def size() -> int:
    """Return the number of cached entries."""
    with _lock:
        return len(_cache)
