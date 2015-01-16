#pragma once

/* Scripts */
//#define MOUSE_MOVE_CALLBACK // expose mouse move callback to scripts (configure in bind_stalker)
//#define KEY_RELEASE_CALLBACK // expose key release callback to scripts (configure in bind_stalker)
//#define KEY_HOLD_CALLBACK // expose key hold callback to scripts (configure in bind_stalker)
#define FP_DEATH // first person death view
#define LUA_DEBUG_PRINT // allow LUA debug prints (i.e.: ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError, "CWeapon : cannot access class member Weapon_IsScopeAttached!");)
//#define SPAWN_ANTIFREEZE // spread spawn of game objects thoughout multiple frames to prevent lags (by alpet)
#define NON_FATAL_VERIFY // don't crash game when VERIFY fails
#define DETAIL_RADIUS // detail draw radius (by KD)
#define ECO_RENDER // limit FPS in menu to prevent video card overheat (by alpet)
#define NEW_ANIMS // use new animations. Please enclose any new animation addions with this define
#define NEW_SOUNDS // use new sounds. Please enclose any new sound addions with this define