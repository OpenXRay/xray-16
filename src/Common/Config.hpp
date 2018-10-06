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

/* Visual */
#define DETAIL_RADIUS // detail draw radius (by K.D.)

/* Tweaks: */
//#define FP_DEATH // first person death view
#define DEAD_BODY_COLLISION // restore collision with dead bodies (thanks malandrinus)
#define NEW_ANIMS // use new animations. Please enclose any new animation additions with this define

//#define CONFIG_SUN_MOVEMENT // With this defined sun will move as configured in weather ltx files

/* Sound: */
#define NEW_SOUNDS // use new sounds. Please enclose any new sound additions with this define
#define LAYERED_SND_SHOOT// see comment down below

/* Debug: */
//#define USE_LOG_TIMING


/* LAYERED_SND_SHOOT by Alundaio
When defined, it will allow you to play a group of sounds from a specified section for snd_shoot.
You can have as many layers as you want, but you must follow naming convention,
snd_1_layer
snd_2_layer
snd_3_layer
...
You can also have different variants for each layer defined,
snd_1_layer
snd_1_layer1
snd_1_layer2
...
The correct line settings are standard, (ie. snd_1_layer = sound_path, volume, delay)
ex.
Here is an example usage:
snd_shoot = new_snd_section
[new_snd_section]
snd_1_layer = weapons\new_sound_shoot1
snd_1_layer1 = weapons\new_sound_shoot2
snd_1_layer2 = weapons\new_sound_shoot3
snd_1_layer3 = weapons\new_sound_shoot4
snd_2_layer = weapons\mechanical_noise, 1.0, 0.1
snd_3_layer = weapons\gunshot_echo, 1.0, 0.8
*/
//-TWEAKS
