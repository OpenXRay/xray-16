#pragma once

// CALLBACKS
	#define EXTENDED_ITEM_CALLBACKS							// (eItemToBelt, eItemToSlot, eItemToRuck)
	#define EXTENDED_WEAPON_CALLBACKS						// (eOnWeaponZoomIn, eOnWeaponZoomOut, eOnWeaponJammed)
	#define INVENTORY_UPGRADE_CALLBACKS						// calls lua function inventory_upgrades.effect_upgrade_item
	#define ACTOR_BEFORE_DEATH_CALLBACK						// For extending the life of the actor to fake death or do other tasks that need to happen before actor is dead
	#define INPUT_CALLBACKS									// (eKeyPress, eKeyRelease, eKeyHold)
	//#define MOUSE_INPUT_CALLBACKS							// (eMouseMove, eMouseWheel)
	#define ENGINE_LUA_ALIFE_STORAGE_MANAGER_CALLBACKS		// calls lua functions from engine in a script named alife_storage_manager.script  (alife_storage_manager.CALifeStorageManager_save) and (alife_storage_manager.CALifeStorageManager_load)
	#define ENGINE_LUA_ALIFE_UPDAGE_MANAGER_CALLBACKS		// calls lua function named on_before_change_level and on_after_new_game in _G.script when enabled
//-CALLBACKS

// SCRIPTS:
	//#define MORE_INVENTORY_SLOTS							// Adds 5 more slots CUSTOM_SLOT_1..5
	#define GAME_OBJECT_EXTENDED_EXPORTS					// see: script_game_object*.cpp/h
	#define GAME_OBJECT_TESTING_EXPORTS						// see: script_game_object4.cpp  functions for object testing (ie. is_stalker(), is_heli())
	#define NAMESPACE_LEVEL_EXPORTS							// see: level_script.cpp
	#define INI_FILE_EXTENDED_EXPORTS						// see: script_ini_file_script.cpp
	#define ENABLE_CAR										// reimplements car along with new callbacks (eOnVehicleAttached, eOnVehicleDetached) and new game_object actor methods get_attached_vehicle(), attach_vehicle() and detach_vehicle()
//-SCRIPTS

// CORE:
	#define NO_BUG_TRAP										// dont use bug trap
	//#define SPAWN_ANTIFREEZE								// spread spawn of game objects thoughout multiple frames to prevent lags (by alpet)
	#define NON_FATAL_VERIFY								// don't crash game when VERIFY fails
	#define USE_GSC_MEM_ALLOC								// when this is undefined memory allocation for luajit is handled by luajit allocator
//-CORE

// VISUAL:
	#define DETAIL_RADIUS									// detail draw radius (by KD)
	#define VSYNC_FIX										// functional VSync by avbaula
	#define ECO_RENDER										// limit FPS in menu to prevent video card overheat (by alpet)
	#define TREE_WIND_EFFECT								// configurable tree sway, can be used to have trees sway more during storms or lightly on clear days.
//-VISUAL

// TWEAKS:
	//#define ACTOR_FEEL_GRENADE							// When undefined it disables the grenade HUD indicator for thrown grenades
	//#define FP_DEATH										// first person death view (Note: It's fixed to position and does not follow corpse)
	//#define DEAD_BODY_COLLISION								// restore collision with dead bodies (thanks malandrinus) (Note: Collides with AI and they can get stuck)
	#define NEW_ANIMS										// use new animations. Please enclose any new animation addions with this define
	//#define DYNAMIC_SUN_MOVEMENT							// use dynamic sun movement. If this is not defined sun will move as configured in weather ltx files
//-TWEAKS

// SOUND:
	#define NEW_SOUNDS									// use new sounds. Please enclose any new sound addions with this define
	#define LAYERED_SND_SHOOT								// see comment down below 
//-SOUND




/*LAYERED_SND_SHOOT by Alundaio
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