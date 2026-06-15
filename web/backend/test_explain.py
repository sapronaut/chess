"""
Tests for the /api/explain endpoint.

The real UCIEngine spawns a subprocess for the compiled SapChess
binary, which won't exist in CI/test environments. These tests
monkeypatch UCIEngine with a stub that returns a fixed SearchResult,
and monkeypatch the LLM call, so we can verify request/response
handling, SAN conversion, caching, and fallback behavior without
needing the actual engine binary or a real API key.
"""

import pytest
from fastapi.testclient import TestClient

from backend import cache
from backend.app import app
from backend.llm_client import LLMConfigError, LLMRequestError
from backend.uci_engine import SearchResult


class StubEngine:
    """Drop-in replacement for UCIEngine that returns a canned result."""

    def __init__(self, *args, **kwargs):
        pass

    def start(self):
        pass

    def set_position(self, fen=None, moves=None):
        pass

    def go(self, depth=12, movetime_ms=None):
        # Pretend the engine likes 1. e4 and sees ...e5 2. Nf3 as the PV.
        return SearchResult(
            best_move="e2e4",
            score_cp=35,
            score_mate=None,
            depth=12,
            pv=["e7e5", "g1f3"],
        )

    def quit(self):
        pass


@pytest.fixture(autouse=True)
def clear_cache():
    """Ensure the explanation cache doesn't leak state between tests."""
    cache.clear()
    yield
    cache.clear()


@pytest.fixture
def client(monkeypatch):
    monkeypatch.setattr("backend.explain.UCIEngine", StubEngine)
    monkeypatch.setattr(
        "backend.explain.generate_text",
        lambda prompt, system=None: "Opening the center and developing toward king safety.",
    )
    return TestClient(app)


def test_explain_valid_opening_move(client):
    response = client.post("/api/explain", json={"fen": None, "move": "e2e4"})
    assert response.status_code == 200

    data = response.json()
    assert data["move_san"] == "e4"
    assert data["score_cp"] == 35
    assert data["score_mate"] is None
    # PV is from the position after e2e4 (Black to move): e7e5, then Nf3
    assert data["principal_variation"] == ["e5", "Nf3"]
    assert data["explanation"] == "Opening the center and developing toward king safety."


def test_explain_illegal_move_returns_400(client):
    response = client.post("/api/explain", json={"fen": None, "move": "e2e5"})
    assert response.status_code == 400


def test_explain_evaluation_is_human_readable(client):
    response = client.post("/api/explain", json={"fen": None, "move": "e2e4"})
    data = response.json()
    # After 1.e4 it's Black to move; +35cp reported for Black means
    # it's -0.35 from White's perspective.
    assert data["evaluation"] == "-0.35"


def test_explain_uses_cache_on_second_call(client, monkeypatch):
    calls = []

    def tracking_generate_text(prompt, system=None):
        calls.append(prompt)
        return "Cached-path explanation."

    monkeypatch.setattr("backend.explain.generate_text", tracking_generate_text)

    first = client.post("/api/explain", json={"fen": None, "move": "e2e4"})
    second = client.post("/api/explain", json={"fen": None, "move": "e2e4"})

    assert first.json()["explanation"] == "Cached-path explanation."
    assert second.json()["explanation"] == "Cached-path explanation."
    # The LLM should only be called once; the second request hits the cache.
    assert len(calls) == 1


def test_explain_falls_back_when_llm_unconfigured(client, monkeypatch):
    def raise_config_error(prompt, system=None):
        raise LLMConfigError("ANTHROPIC_API_KEY not set")

    monkeypatch.setattr("backend.explain.generate_text", raise_config_error)

    response = client.post("/api/explain", json={"fen": None, "move": "e2e4"})
    assert response.status_code == 200
    data = response.json()
    assert "unavailable" in data["explanation"].lower()
    # Evaluation/PV should still be present and correct.
    assert data["evaluation"] == "-0.35"
    assert data["principal_variation"] == ["e5", "Nf3"]


def test_explain_falls_back_when_llm_request_fails(client, monkeypatch):
    def raise_request_error(prompt, system=None):
        raise LLMRequestError("API timeout")

    monkeypatch.setattr("backend.explain.generate_text", raise_request_error)

    response = client.post("/api/explain", json={"fen": None, "move": "e2e4"})
    assert response.status_code == 200
    data = response.json()
    assert "unavailable" in data["explanation"].lower()


def test_health_endpoint(client):
    response = client.get("/health")
    assert response.status_code == 200
    assert response.json() == {"status": "ok"}
