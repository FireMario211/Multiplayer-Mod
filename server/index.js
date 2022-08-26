/**
 * GD Multiplayer Mod Socket Server
 * Version 1.0
 * (this isn't secure at all by the way, has no anti-cheat or checks)
**/

const http = require('http')
const server = http.createServer();
const sio = require('socket.io')(server);

let playerTemplate = {
    id: Number,
    x: Number,
    y: Number,
    rotation: Number,
    scale: Number,
    tag: Number,
    gamemode: Number,
    icons: {
        cube: Number,
        ship: Number,
        ball: Number,
        ufo: Number,
        wave: Number,
        robot: Number,
        spider: Number
    },
    level: Number,
}

// since im not using typescript
let players = [playerTemplate]
players = [];
function customTag() {
    // Random Integer which defines the players ID
    return Math.floor(Math.random() * 9999);
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
        console.log("Socket joined")
        let playerObj = {
            id: socket.id,
            x: 0,
            y: 0,
            rotation: 0,
            scale: 1,
            tag: customTag(),
            gamemode: 0,
            icons: {
                cube: 1,
                ship: 1,
                ball: 1,
                ufo: 1,
                wave: 1,
                robot: 1,
                spider: 1
            },
            level
        }
        players.push(playerObj)
        players.forEach(function (player) {
            if (player.id == socket.id) {
                socket.to(level).emit('player', player)
            }/* else {
                sio.to(level).emit('update', player)
            }*/
        })
        
    })
    socket.on('update', async (pos) => {
        const player = players.find(p => p.id == socket.id)
        if (!player) return;
        player.x = pos.x
        player.y = pos.y
        player.rotation = pos.rotation
        player.scale = pos.scale
        player.gamemode = pos.gamemode
        socket.broadcast.to(player.level).emit('update', player)
        console.log(player)
    })
    socket.on('disconnect', async () => {
        console.log("Socket disconnected")
        const player = players.find(p => p.id === socket.id)
        if (!player) return;
        player.y = -100;
        socket.broadcast.to(player.level).emit('update', player)
        players.splice(players.indexOf(player), 1)
    })
})
server.listen(8000, async () => {
    console.log(`Server Listening on port @8000`)
})