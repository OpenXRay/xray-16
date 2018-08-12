////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_animation_manager.cpp
//	Created 	: 25.02.2003
//  Modified 	: 19.11.2004
//	Author		: Dmitriy Iassenev
//	Description : Stalker animation manager
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "stalker_animation_manager.h"
#include "ai/stalker/ai_stalker.h"
#include "stalker_animation_data_storage.h"
#include "stalker_animation_data.h"
#include "stalker_movement_manager_smart_cover.h"

// TODO:
// stalker animation manager consists of 5 independent managers,
// they should be represented with the different classes:
//    * head
//    * torso
//    * legs
//    * globals
//    * script

CStalkerAnimationManager::CStalkerAnimationManager(CAI_Stalker* object)
    : m_object(object), m_global(object), m_head(object), m_torso(object), m_legs(object), m_script(object),
      m_start_new_script_animation(false)
{
}

void CStalkerAnimationManager::reinit()
{
    m_direction_start = 0;
    m_current_direction = eMovementDirectionForward;
    m_target_direction = eMovementDirectionForward;

    m_change_direction_time = 0;
    m_looking_back = 0;

    m_no_move_actual = false;

    m_script_animations.clear();

    m_global.reset();
    m_head.reset();
    m_torso.reset();
    m_legs.reset();
    m_script.reset();

    m_legs.step_dependence(true);
    m_global.step_dependence(true);
    m_script.step_dependence(true);

    m_global.global_animation(true);
    m_script.global_animation(true);

    m_call_script_callback = false;

    m_previous_speed = 0.f;
    m_target_speed = 0.f;
    m_last_non_zero_speed = m_target_speed;

    m_special_danger_move = false;
}

void CStalkerAnimationManager::reload()
{
    m_visual = object().Visual();

    m_crouch_state_config = object().SpecificCharacter().crouch_type();
    VERIFY((m_crouch_state_config == 0) || (m_crouch_state_config == 1) || (m_crouch_state_config == -1));
    m_crouch_state = m_crouch_state_config;

    if (object().already_dead())
        return;

    m_skeleton_animated = smart_cast<IKinematicsAnimated*>(m_visual);
    VERIFY(m_skeleton_animated);

    m_data_storage = stalker_animation_data_storage().object(m_skeleton_animated);
    VERIFY(m_data_storage);

    if (!object().g_Alive())
        return;

#ifdef USE_HEAD_BONE_PART_FAKE
    VERIFY(!m_data_storage->m_head_animations.A.empty());
    u16 bone_part = m_skeleton_animated->LL_GetMotionDef(m_data_storage->m_head_animations.A.front())->bone_or_part;
    VERIFY(bone_part != BI_NONE);
    m_script_bone_part_mask = CStalkerAnimationPair::all_bone_parts ^ (1 << bone_part);
#endif

    assign_bone_callbacks();

#ifdef DEBUG
    global().set_dbg_info(*object().cName(), "Global");
    head().set_dbg_info(*object().cName(), "Head  ");
    torso().set_dbg_info(*object().cName(), "Torso ");
    legs().set_dbg_info(*object().cName(), "Legs  ");
    script().set_dbg_info(*object().cName(), "Script");
#endif
};

void CStalkerAnimationManager::play_fx(float power_factor, int fx_index)
{
    VERIFY(fx_index >= 0);
    VERIFY(
        fx_index < (int)m_data_storage->m_part_animations.A[object().movement().body_state()].m_global.A[0].A.size());
#ifdef DEBUG
    if (psAI_Flags.is(aiAnimation))
    {
        LPCSTR name =
            m_skeleton_animated
                ->LL_MotionDefName_dbg(
                    m_data_storage->m_part_animations.A[object().movement().body_state()].m_global.A[0].A[fx_index])
                .first;
        Msg("%6d [%s][%s][%s][%f]", Device.dwTimeGlobal, *object().cName(), "FX", name, power_factor);
    }
#endif
    m_skeleton_animated->PlayFX(
        m_data_storage->m_part_animations.A[object().movement().body_state()].m_global.A[0].A[fx_index], power_factor);
}
