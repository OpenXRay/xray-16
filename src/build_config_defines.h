#pragma once

// SCRIPTS:
//#define MOUSE_MOVE_CALLBACK // expose mouse move callback to scripts (configure in bind_stalker)
#define KEY_RELEASE_CALLBACK // expose key release callback to scripts (configure in bind_stalker)
//#define KEY_HOLD_CALLBACK // expose key hold callback to scripts (configure in bind_stalker)
/*** DEPRECATED as redundant, all engine lua output is being saved into main log now. Do not use as it will be removed
#define LUA_DEBUG_PRINT // allow output of lua logs (*_lua.log)
***/
//-SCRIPTS

// CORE:
#define NO_BUG_TRAP // dont use bug trap
#define SPAWN_ANTIFREEZE // spread spawn of game objects thoughout multiple frames to prevent lags (by alpet)
#define NON_FATAL_VERIFY // don't crash game when VERIFY fails
#define USE_GSC_MEM_ALLOC // when this is undefined memory allocation for luajit is handled by luajit allocator
//-CORE

// VISUAL:
#define DETAIL_RADIUS // detail draw radius (by KD)
#define VSYNC_FIX // functional VSync by avbaula
#define ECO_RENDER // limit FPS in menu to prevent video card overheat (by alpet)
#define TREE_WIND_EFFECT // configurable tree sway, can be used to have trees sway more during storms or lightly on clear days.
//-VISUAL

// TWEAKS:
//#define FP_DEATH // first person death view
#define DEAD_BODY_COLLISION // restore collision with dead bodies (thanks malandrinus)
#define NEW_ANIMS // use new animations. Please enclose any new animation addions with this define
#define NEW_SOUNDS // use new sounds. Please enclose any new sound addions with this define
//#define DYNAMIC_SUN_MOVEMENT // use dynamic sun movement. If this is not defined sun will move as configured in weather ltx files
//-TWEAKS