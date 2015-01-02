#pragma once

/* Scripts */
//#define MOUSE_MOVE_CALLBACK // expose mouse move callback to scripts (configure in bind_stalker)
//#define KEY_RELEASE_CALLBACK // expose key release callback to scripts (configure in bind_stalker)
//#define KEY_HOLD_CALLBACK // expose key hold callback to scripts (configure in bind_stalker)
#define FP_DEATH // first person death view
#define LUA_DEBUG_PRINT // allow LUA debug prints (i.e.: ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError, "CWeapon : cannot access class member Weapon_IsScopeAttached!");)
#define SPAWN_ANTIFREEZE // spread spawn of game objects thoughout multiple frames to prevent lags (by alpet)
#define NON_FATAL_VERIFY // don't crash game when VERIFY fails