// ============================================================
//  SAPCHESS FRONTEND
//  Handles both vs-bot and multiplayer modes.
// ============================================================

let board = null;       // chessboard.js instance
let game  = null;       // chess.js instance (move validation)
let ws    = null;       // WebSocket for multiplayer
let playerColor = 'w'; // our color in current game
let gameMode    = null; // 'bot' or 'multiplayer'
let moveHistory = [];   // all moves played
let currentRoomId = null;

// ---- Screen navigation ----
function showHome() {
    show('home-screen');
    hide('game-screen');
    hide('multiplayer-screen');
    if (ws) { ws.close(); ws = null; }
}

function showMultiplayer() {
    show('multiplayer-screen');
    hide('home-screen');
}

function show(id) { document.getElementById(id).classList.remove('hidden'); }
function hide(id) { document.getElementById(id).classList.add('hidden'); }

// ---- Board initialization ----
function initBoard(orientation = 'white') {
    game = new Chess();
    moveHistory = [];
    document.getElementById('moves-list').innerHTML = '';

    const config = {
        draggable: true,
        position: 'start',
        orientation: orientation,
        onDragStart: onDragStart,
        onDrop: onDrop,
        onSnapEnd: () => board.position(game.fen()),
        pieceTheme: 'https://chessboardjs.com/img/chesspieces/wikipedia/{piece}.png'
    };

    if (board) board.destroy();
    board = Chessboard('board', config);

    show('game-screen');
    hide('home-screen');
    hide('multiplayer-screen');
}

// ---- Drag validation ----
function onDragStart(source, piece) {
    // Don't pick up pieces if game over
    if (game.game_over()) return false;

    // Only move your own pieces
    if (gameMode === 'bot') {
        if (playerColor === 'w' && piece.search(/^b/) !== -1) return false;
        if (playerColor === 'b' && piece.search(/^w/) !== -1) return false;
    }

    if (gameMode === 'multiplayer') {
        const myTurn = (playerColor === 'w' && game.turn() === 'w') ||
                       (playerColor === 'b' && game.turn() === 'b');
        if (!myTurn) return false;
        if (playerColor === 'w' && piece.search(/^b/) !== -1) return false;
        if (playerColor === 'b' && piece.search(/^w/) !== -1) return false;
    }

    return true;
}

// ---- Move handling ----
function onDrop(source, target) {
    // Try the move with chess.js
    const move = game.move({
        from: source,
        to: target,
        promotion: 'q'  // auto-promote to queen for now
    });

    // Illegal move - snap back
    if (move === null) return 'snapback';

    const uciMove = move.from + move.to + (move.promotion || '');
    moveHistory.push(uciMove);
    updateMoveHistory(uciMove);
    updateStatus();

    if (game.game_over()) {
        handleGameOver();
        return;
    }

    if (gameMode === 'bot') {
        // Disable board while engine thinks
        setStatus('Engine thinking...');
        setTimeout(() => askEngine(), 100);
    }

    if (gameMode === 'multiplayer') {
        // Send move to server
        ws.send(JSON.stringify({ type: 'move', move: uciMove }));
    }
}

// ---- Bot mode ----
async function startBotGame() {
    gameMode = 'bot';
    playerColor = 'w';

    document.getElementById('white-name').textContent = 'You';
    document.getElementById('black-name').textContent = 'SapChess';
    hide('room-info');

    await fetch('/api/bot/new', { method: 'POST' });
    initBoard('white');
    setStatus('Your turn');
}

async function askEngine() {
    try {
        const res = await fetch('/api/bot/move', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ moves: moveHistory, depth: 6 })
        });
        const data = await res.json();

        if (data.status === 'gameover') {
            handleGameOver();
            return;
        }

        if (data.bestmove) {
            const from  = data.bestmove.substring(0, 2);
            const to    = data.bestmove.substring(2, 4);
            const promo = data.bestmove.length === 5 ? data.bestmove[4] : undefined;

            game.move({ from, to, promotion: promo || 'q' });
            moveHistory.push(data.bestmove);
            board.position(game.fen());
            updateMoveHistory(data.bestmove);
            updateStatus();

            if (game.game_over()) handleGameOver();
        }
    } catch (e) {
        setStatus('Engine error - is the server running?');
    }
}

// ---- Multiplayer mode ----
async function createRoom() {
    const res = await fetch('/api/room/create', { method: 'POST' });
    const data = await res.json();
    currentRoomId = data.room_id;
    connectToRoom(currentRoomId, 'white');
}

function joinRoom() {
    const code = document.getElementById('room-input').value.trim();
    if (!code) return;
    currentRoomId = code;
    connectToRoom(code, 'black');
}

function connectToRoom(roomId, expectedColor) {
    gameMode = 'multiplayer';

    const protocol = location.protocol === 'https:' ? 'wss' : 'ws';
    ws = new WebSocket(`${protocol}://${location.host}/ws/${roomId}`);

    ws.onmessage = (event) => {
        const data = JSON.parse(event.data);

        if (data.type === 'init') {
            playerColor = data.color === 'white' ? 'w' : 'b';

            document.getElementById('white-name').textContent =
                playerColor === 'w' ? 'You' : 'Opponent';
            document.getElementById('black-name').textContent =
                playerColor === 'b' ? 'You' : 'Opponent';

            initBoard(data.color);

            // Show room code
            show('room-info');
            document.getElementById('room-code-display').textContent =
                roomId.toUpperCase();

            if (data.color === 'white') {
                setStatus('Waiting for opponent...');
            } else {
                setStatus('Connected! Your turn as Black');
            }
        }

        if (data.type === 'opponent_joined') {
            setStatus(playerColor === 'w' ? 'Your turn' : "Opponent's turn");
        }

        if (data.type === 'move') {
            // Apply opponent's move to our board
            const from  = data.move.substring(0, 2);
            const to    = data.move.substring(2, 4);
            const promo = data.move.length === 5 ? data.move[4] : 'q';

            // Only apply if it wasn't our own move
            if (game.fen() !== data.fen) {
                game.move({ from, to, promotion: promo });
                board.position(game.fen());
                moveHistory.push(data.move);
                updateMoveHistory(data.move);
            }
            updateStatus();
        }

        if (data.type === 'gameover') {
            handleGameOver(data.result);
        }

        if (data.type === 'opponent_left') {
            setStatus('Opponent disconnected');
        }

        if (data.type === 'error') {
            console.error('Server error:', data.msg);
        }
    };

    ws.onerror = () => setStatus('Connection error');
    ws.onclose = () => console.log('WebSocket closed');
}

// ---- UI helpers ----
function setStatus(msg) {
    document.getElementById('status').textContent = msg;
}

function updateStatus() {
    if (game.game_over()) return;
    const myTurn = game.turn() === playerColor;
    setStatus(myTurn ? 'Your turn' : "Opponent's turn");
}

function updateMoveHistory(uciMove) {
    const list = document.getElementById('moves-list');
    const span = document.createElement('span');
    span.textContent = uciMove;
    list.appendChild(span);
    list.scrollTop = list.scrollHeight;
}

function handleGameOver(result) {
    let msg = 'Game over';
    if (game.in_checkmate()) {
        msg = game.turn() === 'w' ? 'Black wins!' : 'White wins!';
    } else if (game.in_draw()) {
        msg = 'Draw!';
    } else if (result) {
        msg = result === '1-0' ? 'White wins!' :
              result === '0-1' ? 'Black wins!' : 'Draw!';
    }
    setStatus(msg);
}

function flipBoard() {
    if (board) board.flip();
}

function copyRoomCode() {
    const code = currentRoomId;
    const url  = `${location.origin}/?room=${code}`;
    navigator.clipboard.writeText(url);

    const btn = event.target;
    btn.textContent = 'Copied!';
    setTimeout(() => btn.textContent = 'Copy Link', 2000);
}