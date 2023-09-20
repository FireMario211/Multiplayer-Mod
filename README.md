# GD Multiplayer Mod
A mod that allows you to play with other users on the same Geometry Dash level.
<p align="center"><img src="./screenshots/Screenshot_1.png?raw=true" alt="Mod Example"/></p> 

## NOTICE
There are **most likely** some bugs in this mod that I have yet to fix. You can report them in issues if you find any.
Also another note, there isn't a "DLL" provided as I do not have a socket server to run off of. This may change in the future.

## Installation
To download this mod, you can download it from the Releases page, do note that it'll automatically try connecting to `localhost`, meaning it will only work on your machine, and no one else can connect to it (Unless you setup a bridge/proxy).
## Building

To build this project, you must have the following installed:
- [CMake](https://cmake.org/) [Version 3.0.0+]
- [Visual Studio Build Tools (MSVC)](https://visualstudio.microsoft.com/downloads/)
- [Node.JS](https://nodejs.org) - Optional but required for running the socket server
Also be sure to have the libraries installed under this section.

After all of that, you can simply build the project using CMake as x86 Release, then you should see `Multiplayer.dll` in the `builds/Release` folder (or a different build folder depending on where you set the build directory to)
## Mod Libraries
- [gd.h](https://github.com/HJfod/gd.h/tree/90f21108faea2b6f3d9756f458a5f8a5a421ab6d) - This was modified manually to add new fields.
- [cocos-headers](https://github.com/HJfod/cocos-headers/tree/01436c6fec5bc0a42a2d75b188c40895eee8b60a)
- [MinHook](https://github.com/TsudaKageyu/minhook/tree/4a455528f61b5a375b1f9d44e7d296d47f18bb18)
- [Socket.IO Client CPP](https://github.com/socketio/socket.io-client-cpp)

## Server Libraries
- [Socket.IO](https://www.npmjs.com/package/socket.io)

## Running the Server
To run the server, you first will need to install the modules it requires, though you should be able to execute these commands to install it, assuming you have Node.JS installed already
```
cd server
npm i
```
After installing the packages, you can run the server using `node .` or `node index.js`.

## TODO (For me and contributers)
- Implement Usernames instead of tags
- ~~Stop people from crashing other players~~
- Migrate to Geode
- Add smooth interpolation using SimplePlayer
- (possibly?) Add a menu for connection settings and other settings
    - ^^ This for Geode since it allows you to add settings
- Add optional anticheat for speedhacks/noclip

## License
This project is created by [Firee](https://github.com/FireMario211) under the [MIT](https://choosealicense.com/licenses/mit/) license, read more by clicking on the highlighted name.
