/**
 * GD Multiplayer Mod Client
 * Version 1.0
 * Will code in Geode soon
**/

#include "pch.h"
#include "PlayLayer.h"

// For debugging
void ShowConsole() {
    AllocConsole();
    SetConsoleTitleA("Mod");
    freopen_s(reinterpret_cast<FILE**>(stdout), "CONOUT$", "w", stdout);
}

DWORD WINAPI my_thread(void* hModule) {
    if (MH_Initialize() != MH_OK) {
        FreeLibraryAndExitThread(reinterpret_cast<HMODULE>(hModule), 0);
    }
    // PlayLayer::hook (PlayLayer::init as trampoline) -> Initializes connection with the socket server.
    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x01FB780), PlayLayer::hook, reinterpret_cast<void**>(&PlayLayer::init));

    // PlayLayer::hookUpdate (PlayLayer::update as trampoline) -> If there is a connection, send requests to the socket server of the players data.
    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x2029C0), PlayLayer::hookUpdate, reinterpret_cast<void**>(&PlayLayer::update));

    // PlayLayer::hookExit (PlayLayer::exit as trampoline) -> Terminates the connection to the socket server, and stops the thread.
    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x20D810), PlayLayer::hookExit, reinterpret_cast<void**>(&PlayLayer::exit));

    // PauseLayer::hook (PauseLayer::init as trampoline, AKA customSetup) -> Detects whether or not the player has paused. If they have, pause any socket events.
    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x1e4620), PauseLayer::hook, reinterpret_cast<void**>(&PauseLayer::init));
    MH_EnableHook(MH_ALL_HOOKS);
    return true;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        CreateThread(0, 0x1000, my_thread, hModule, 0, 0);
    }
    return TRUE;
}

