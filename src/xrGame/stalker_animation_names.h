////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_animation_names.h
//	Created 	: 25.02.2003
//  Modified 	: 19.11.2004
//	Author		: Dmitriy Iassenev
//	Description : Stalker animation names
////////////////////////////////////////////////////////////////////////////

#pragma once

extern LPCSTR state_names[];
extern LPCSTR weapon_names[];
extern LPCSTR weapon_action_names[];
extern LPCSTR food_names[];
extern LPCSTR food_action_names[];
extern LPCSTR movement_names[];
extern LPCSTR movement_action_names[];
extern LPCSTR in_place_names[];
extern LPCSTR global_names[];
extern LPCSTR head_names[];

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
