/**
 * GD Multiplayer Mod Socket Server
 * Version 1.0
 * (this isn't secure at all by the way, has no anti-cheat or checks)
 *
 * Also I know there are probably other better libraries than socket.io like ENet or other networking libs but socket.io is easier, and crossplatform.
 * so this could even run on android apparently
**/
const http = require('http')
const server = http.createServer();
const sio = require('socket.io')(server);

let playerTemplate = {
    id: Number, // Socket ID
    x: Number, // Player X pos
    y: Number, // Player Y pos
    rotation: Number, // Player rotation
    scale: Number, // Player scale (For mini portals)
    flip: Boolean, // If the player is upside down or not (dependant on the scale)
    tag: Number, // Randomly generated number (ID)
    gamemode: Number, // Gamemode ID
    cube: Number,
    ship: Number,
    ball: Number,
    ufo: Number,
    wave: Number,
    robot: Number,
    spider: Number,
    col1: [Number, Number, Number], // RGB
    col2: [Number, Number, Number], // RGB
    glow: Boolean, // Level ID
    level: Number,
    lastFrame: Number
}

let customTagCount = 10001;
// since im not using typescript
let players = [playerTemplate]
players = [];

function minMaxRandom(min, max) {
    return Math.floor(Math.random() * (max - min + 1) + min);
}

function customTag() {
    // Random Integer which defines the players ID
    //return minMaxRandom(10000, 99999);
    // forget randomness, duplicates can happen.
    customTagCount++;
    return customTagCount;
}

const maxFPS = 240; // Frames per interval. Determines how "smooth" the movements should be. Also for limiting packets sent to every player.
const render_distance = 1000;

function calculateDistance(x1, y1, x2, y2) {
    return Math.sqrt(Math.pow(x1 - x2, 2) + Math.pow(y1 - y2, 2));
}

// Gamemode IDs:
// 0 = Cube
// 1 = Ship
// 2 = Ball
// 3 = UFO
// 4 = Wave
// 5 = Robot
// 6 = Spider

sio.on('connection', socket => {
    console.log("Socket connected")
    socket.on('join', async (level) => {
        level = parseInt(level);
        if (isNaN(level)) return;
        socket.join(level)
        const tag = customTag();
        let playerObj = {
            id: socket.id,
            x: 0,
            y: 0,
            rotation: 0,
            scale: 1,
            flip: false,
            tag,
            gamemode: 0,
            cube: 1,
            ship: 1,
            ball: 1,
            ufo: 1,
            wave: 1,
            robot: 1,
            spider: 1,
            col1: [255, 255, 255],
            col2: [255, 255, 255],
            glow: 0,
            level,
            lastFrame: 0
        }
        // for some reason this wont emit unless i do this, i dont know why
        setTimeout(() => {
            socket.emit('tag', tag);
        }, 1000)
        players.push(playerObj)
        players.forEach(function (player) {
            if (player.id == socket.id) {
                socket.to(level).emit('player', player)
            }
        })
        console.log(`Socket joined (${tag}), current players: ${players.length}`)
    })
    socket.on('receive', async () => {
        const player = players.find(p => p.id == socket.id)
        if (!player) return;
        const curr_players = players.filter(p => p.level == player.level);
        if (!curr_players.length) return;
        curr_players.filter(p => p.id != socket.id).forEach(function (other_player) {
            socket.emit('update', other_player);
        })
    })
    socket.on('update', async (pos) => { // when a player updates their position
        const player = players.find(p => p.id == socket.id)
        if (!player) return;
        const curr_players = players.filter(p => p.level == player.level);
        if (!curr_players.length) return;
        // some normal checks
        if (player.scale > 2) return;
        if (player.y < -2) return;
        if (player.gamemode < 0 || player.gamemode > 6) return;
        const currentTime = Date.now();
        const elapsedTime = currentTime - player.lastFrame;
        if (elapsedTime >= (1000 / maxFPS)) {
            player.x = pos.x
            player.y = pos.y
            player.rotation = pos.rotation
            player.scale = pos.scale
            player.flip = pos.flip
            player.gamemode = pos.gamemode

            // icon
            player.cube = pos.cube
            player.ship = pos.ship
            player.ball = pos.ball
            player.ufo = pos.ufo
            player.wave = pos.wave
            player.robot = pos.robot
            player.spider = pos.spider
            player.col1 = pos.col1;
            player.col2 = pos.col2;
            player.glow = pos.glow;
            // icon
            curr_players.filter(p => p.id != socket.id).forEach(function (other_player) {
                //if (calculateDistance(other_player.x, other_player.y, player.x, player.y) <= render_distance) {
                socket.to(other_player.id).emit('update', player);
                //}
            })
            player.lastFrame = currentTime;
            //socket.broadcast.to(player.level).emit('update', player)
        }
    })
    socket.on('disconnect', async () => {
        console.log("Socket disconnected")
        const player = players.find(p => p.id === socket.id)
        if (!player) return;
        player.y = -100;
        socket.broadcast.to(player.level).emit('left', player)
        players.splice(players.indexOf(player), 1)
    })
})
server.listen(8000, async () => {
    console.log(`Server Listening on port @8000`)
})
