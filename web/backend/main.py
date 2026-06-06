from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.staticfiles import StaticFiles
from fastapi.responses import FileResponse
from pydantic import BaseModel
from engine import UCIEngine
import chess
import uuid
import os

app = FastAPI()

# ---- Path to your compiled engine ----
ENGINE_PATH = os.path.join(
    os.path.dirname(__file__),
    "../../SapChess.exe"
)

# ---- Engine instance (one shared engine for vs-bot games) ----
engine = UCIEngine(ENGINE_PATH)

# ---- Active multiplayer rooms ----
# room_id -> {"fen": str, "players": [ws1, ws2], "moves": []}
rooms: dict = {}

# ============================================================
#  REST API - Play vs Bot
# ============================================================

class MoveRequest(BaseModel):
    moves: list[str]  # all moves played so far
    depth: int = 6

@app.post("/api/bot/move")
async def bot_move(req: MoveRequest):
    """Get engine's best move given move history"""
    # Validate position with python-chess
    board = chess.Board()
    for move in req.moves:
        board.push_uci(move)

    if board.is_game_over():
        return {"status": "gameover", "result": board.result()}

    best_move = engine.get_best_move(req.moves, req.depth)
    return {
        "bestmove": best_move,
        "fen": board.fen()
    }

@app.post("/api/bot/new")
async def new_bot_game():
    """Start a new game vs bot"""
    engine.new_game()
    return {"status": "ok"}

# ============================================================
#  WEBSOCKET - Multiplayer
# ============================================================

@app.post("/api/room/create")
async def create_room():
    """Create a new multiplayer room, return room ID"""
    room_id = str(uuid.uuid4())[:8]
    rooms[room_id] = {
        "fen": chess.STARTING_FEN,
        "moves": [],
        "players": []
    }
    return {"room_id": room_id}

@app.websocket("/ws/{room_id}")
async def websocket_room(websocket: WebSocket, room_id: str):
    """WebSocket endpoint for multiplayer rooms"""
    await websocket.accept()

    # Create room if doesn't exist
    if room_id not in rooms:
        rooms[room_id] = {
            "fen": chess.STARTING_FEN,
            "moves": [],
            "players": []
        }

    room = rooms[room_id]

    # Assign color
    if len(room["players"]) == 0:
        color = "white"
    elif len(room["players"]) == 1:
        color = "black"
    else:
        await websocket.send_json({"type": "error", "msg": "Room full"})
        await websocket.close()
        return

    room["players"].append(websocket)

    # Send current board state to new player
    await websocket.send_json({
        "type": "init",
        "color": color,
        "fen": room["fen"],
        "moves": room["moves"]
    })

    # Notify other player if both connected
    if len(room["players"]) == 2:
        await room["players"][0].send_json({
            "type": "opponent_joined"
        })

    try:
        while True:
            data = await websocket.receive_json()

            if data["type"] == "move":
                move_uci = data["move"]

                # Validate move with python-chess
                board = chess.Board()
                for m in room["moves"]:
                    board.push_uci(m)

                try:
                    board.push_uci(move_uci)
                except Exception:
                    await websocket.send_json({
                        "type": "error",
                        "msg": "Illegal move"
                    })
                    continue

                # Update room state
                room["moves"].append(move_uci)
                room["fen"] = board.fen()

                # Broadcast move to both players
                for player in room["players"]:
                    await player.send_json({
                        "type": "move",
                        "move": move_uci,
                        "fen": room["fen"]
                    })

                # Check game over
                if board.is_game_over():
                    for player in room["players"]:
                        await player.send_json({
                            "type": "gameover",
                            "result": board.result()
                        })

    except WebSocketDisconnect:
        room["players"].remove(websocket)
        # Notify other player
        for player in room["players"]:
            await player.send_json({"type": "opponent_left"})

# ============================================================
#  SERVE FRONTEND
# ============================================================
app.mount(
    "/static",
    StaticFiles(directory="../frontend/static"),
    name="static"
)

@app.get("/")
async def serve_index():
    return FileResponse("../frontend/index.html")

@app.get("/play")
async def serve_play():
    return FileResponse("../frontend/index.html")