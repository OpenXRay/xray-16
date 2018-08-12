////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_animation_names.cpp
//	Created 	: 25.02.2003
//  Modified 	: 19.11.2004
//	Author		: Dmitriy Iassenev
//	Description : Stalker animation names
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "stalker_animation_names.h"

LPCSTR state_names[] = {"cr_", "norm_", "dmg_norm_", 0};

LPCSTR weapon_names[] = {"0_", "1_", "2_", "3_", "4_", "5_", "6_", "7_", "8_", "9_", "10_", 0};

LPCSTR weapon_action_names[] = {"draw_", // 0
    "attack_", // 1
    "drop_", // 2
    "holster_", // 3
    "reload_", // 4
    "pick_", // 5
    "aim_", // 6
    "walk_", // 7
    "run_", // 8
    "idle_", // 9
    "prepare_", // 10
    "strap_", // 11
    "unstrap_", // 12
    "look_beack_ls_", // 13
    "look_beack_rs_", // 14
    0};

LPCSTR movement_names[] = {"walk_", "run_", 0};

LPCSTR movement_action_names[] = {"fwd_", "back_", "ls_", "rs_", 0};

LPCSTR in_place_names[] = {"idle_0", "idle_1", "turn_right_0", "turn_left_0", "turn_right_1", "turn_left_1",
    "jump_begin", "jump_idle", "jump_end", "jump_end_1", 0};

LPCSTR global_names[] = {"damage_", // 0
    "escape_", // 1
    "dead_stop_", // 2
    "hello_", // 3

    "1_critical_hit_head_", // 4
    "1_critical_hit_torso_", // 5
    "1_critical_hit_hend_left_", // 6
    "1_critical_hit_hend_right_", // 7
    "1_critical_hit_legs_left_", // 8
    "1_critical_hit_legs_right_", // 9

    "2_critical_hit_head_", // 10
    "2_critical_hit_torso_", // 11
    "2_critical_hit_hend_left_", // 12
    "2_critical_hit_hend_right_", // 13
    "2_critical_hit_legs_left_", // 14
    "2_critical_hit_legs_right_", // 15

    "3_critical_hit_head_", // 16
    "3_critical_hit_torso_", // 17
    "3_critical_hit_hend_left_", // 18
    "3_critical_hit_hend_right_", // 19
    "3_critical_hit_legs_left_", // 20
    "3_critical_hit_legs_right_", // 21

    "panic_stand_", 0};

LPCSTR head_names[] = {"head_idle_0", "head_talk_0", 0};
