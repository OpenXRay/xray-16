#pragma once

// CONFIG_PROFILE_LOCKS
// XXX: convert to command line params
// CONFIG_SCRIPT_ENGINE_LOG_EXPORTS
// CONFIG_SCRIPT_ENGINE_LOG_SKIPPED_EXPORTS

/* Callbacks */
//#define ACTOR_BEFORE_DEATH_CALLBACK // For extending the life of the actor to fake death or do other tasks that need to happen before actor is dead

/* Scripts */
#define MORE_INVENTORY_SLOTS // Adds 5 more slots CUSTOM_SLOT_1..5
#define GAME_OBJECT_EXTENDED_EXPORTS // see: script_game_object*.cpp/h
#define GAME_OBJECT_CASTING_EXPORTS // see: script_game_object4.cpp  functions for object casting (ie. cast_Car(), cast_Heli())

/* Tweaks: */
#define DEAD_BODY_COLLISION // restore collision with dead bodies (thanks malandrinus)

/* Debug: */
//#define USE_LOG_TIMING

#include "submodule_check.hpp"
