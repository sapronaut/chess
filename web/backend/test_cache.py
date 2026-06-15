"""Unit tests for backend.cache"""

from backend import cache


def setup_function():
    cache.clear()


def test_get_returns_none_when_empty():
    assert cache.get("some-fen", "e2e4") is None


def test_set_and_get_roundtrip():
    cache.set("some-fen", "e2e4", "Develops the knight.")
    assert cache.get("some-fen", "e2e4") == "Develops the knight."


def test_different_moves_are_different_keys():
    cache.set("some-fen", "e2e4", "explanation A")
    cache.set("some-fen", "d2d4", "explanation B")
    assert cache.get("some-fen", "e2e4") == "explanation A"
    assert cache.get("some-fen", "d2d4") == "explanation B"


def test_different_positions_are_different_keys():
    cache.set("fen-1", "e2e4", "explanation A")
    cache.set("fen-2", "e2e4", "explanation B")
    assert cache.get("fen-1", "e2e4") == "explanation A"
    assert cache.get("fen-2", "e2e4") == "explanation B"


def test_clear_empties_cache():
    cache.set("some-fen", "e2e4", "explanation")
    cache.clear()
    assert cache.get("some-fen", "e2e4") is None
    assert cache.size() == 0


def test_size_reflects_entry_count():
    assert cache.size() == 0
    cache.set("fen-1", "e2e4", "a")
    cache.set("fen-2", "d2d4", "b")
    assert cache.size() == 2


def test_eviction_when_over_capacity(monkeypatch):
    monkeypatch.setattr(cache, "MAX_CACHE_SIZE", 2)

    cache.set("fen-1", "e2e4", "first")
    cache.set("fen-2", "d2d4", "second")
    cache.set("fen-3", "g1f3", "third")

    # Oldest entry (fen-1, e2e4) should have been evicted.
    assert cache.get("fen-1", "e2e4") is None
    assert cache.get("fen-2", "d2d4") == "second"
    assert cache.get("fen-3", "g1f3") == "third"
    assert cache.size() == 2
