////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_animation_names.h
//	Created 	: 25.02.2003
//  Modified 	: 19.11.2004
//	Author		: Dmitriy Iassenev
//	Description : Stalker animation names
////////////////////////////////////////////////////////////////////////////

#pragma once

extern const char* state_names[];
extern const char* weapon_names[];
extern const char* weapon_action_names[];
extern const char* food_names[];
extern const char* food_action_names[];
extern const char* movement_names[];
extern const char* movement_action_names[];
extern const char* in_place_names[];
extern const char* global_names[];
extern const char* head_names[];

enum ECriticalWoundType : u32
{
    critical_wound_type_head = u32(4),
    critical_wound_type_torso,
    critical_wound_type_hand_left,
    critical_wound_type_hand_right,
    critical_wound_type_leg_left,
    critical_wound_type_leg_right,
    critical_wound_type_dummy = u32(-1),
};
