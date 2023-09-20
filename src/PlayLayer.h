#pragma once
#include "pch.h"

// ITS ALL STATIC
class PlayLayer : public gd::PlayLayer {
	public:
        static inline bool(__thiscall* init)(gd::PlayLayer* self, gd::GJGameLevel* level);
        static inline bool(__thiscall* update)(gd::PlayLayer* self, float dt);
	    static inline bool(__thiscall* exit)(gd::PlayLayer* self);
        static bool __fastcall hook(gd::PlayLayer* self, int edx, gd::GJGameLevel* level);
        static bool __fastcall hookUpdate(gd::PlayLayer* self, float dt);
        static bool __fastcall hookExit(gd::PlayLayer* self);
        // is it really necessary to have these static
        static bool join();
        static bool setSocket(gd::PlayLayer* self, sio::socket::ptr sock);
};

// no need for inheritance
class PauseLayer : public CCLayer {
    public:
        static inline bool(__thiscall* init)(CCLayer* self);
        static bool __fastcall hook(CCLayer* self);
};
