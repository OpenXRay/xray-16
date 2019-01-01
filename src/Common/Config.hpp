#pragma once

// CONFIG_PROFILE_LOCKS
// XXX: convert to command line params
// CONFIG_SCRIPT_ENGINE_LOG_EXPORTS
// CONFIG_SCRIPT_ENGINE_LOG_SKIPPED_EXPORTS

/* Callbacks */
#define EXTENDED_ITEM_CALLBACKS // (eItemToBelt, eItemToSlot, eItemToRuck)
#define EXTENDED_WEAPON_CALLBACKS // (eOnWeaponZoomIn, eOnWeaponZoomOut, eOnWeaponJammed)
//#define ACTOR_BEFORE_DEATH_CALLBACK // For extending the life of the actor to fake death or do other tasks that need to happen before actor is dead
#define INPUT_CALLBACKS // (eKeyPress, eKeyRelease, eKeyHold, eMouseMove, eMouseWheel)
#define ENGINE_LUA_ALIFE_STORAGE_MANAGER_CALLBACKS // calls lua functions from engine in a script named alife_storage_manager.script  (alife_storage_manager.CALifeStorageManager_save) and (alife_storage_manager.CALifeStorageManager_load)
#define ENGINE_LUA_ALIFE_UPDAGE_MANAGER_CALLBACKS // calls lua function named on_before_change_level and on_after_new_game in _G.script when enabled

/* Scripts */
#define MORE_INVENTORY_SLOTS // Adds 5 more slots CUSTOM_SLOT_1..5
#define GAME_OBJECT_EXTENDED_EXPORTS // see: script_game_object*.cpp/h
#define GAME_OBJECT_CASTING_EXPORTS // see: script_game_object4.cpp  functions for object casting (ie. cast_Car(), cast_Heli())
#define NAMESPACE_LEVEL_EXPORTS // see: level_script.cpp

/* Tweaks: */
#define DEAD_BODY_COLLISION // restore collision with dead bodies (thanks malandrinus)

/* Debug: */
//#define USE_LOG_TIMING
