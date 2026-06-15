"""
Tests for the /api/explain endpoint.

The real UCIEngine spawns a subprocess for the compiled SapChess
binary, which won't exist in CI/test environments. These tests
monkeypatch UCIEngine with a stub that returns a fixed SearchResult,
so we can verify request/response handling and SAN conversion
without needing the actual engine binary.
"""

import pytest
from fastapi.testclient import TestClient

from backend.app import app
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


@pytest.fixture
def client(monkeypatch):
    monkeypatch.setattr("backend.explain.UCIEngine", StubEngine)
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
    assert "explanation" in data


def test_explain_illegal_move_returns_400(client):
    response = client.post("/api/explain", json={"fen": None, "move": "e2e5"})
    assert response.status_code == 400


def test_explain_evaluation_is_human_readable(client):
    response = client.post("/api/explain", json={"fen": None, "move": "e2e4"})
    data = response.json()
    # After 1.e4 it's Black to move; +35cp reported for Black means
    # it's -0.35 from White's perspective.
    assert data["evaluation"] == "-0.35"


def test_health_endpoint(client):
    response = client.get("/health")
    assert response.status_code == 200
    assert response.json() == {"status": "ok"}
