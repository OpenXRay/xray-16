#pragma once

#include "ai/monsters/states/state_move_to_point.h"
#include "ai/monsters/states/state_look_point.h"
#include "cover_point.h"
#include "ai/monsters/monster_cover_manager.h"
#include "ai/monsters/monster_home.h"

#define TEMPLATE_SPECIALIZATION \
    template <typename _Object\
>

#define CStateGroupAttackMoveToHomePointAbstract CStateGroupAttackMoveToHomePoint<_Object>

namespace detail
{
namespace dog
{
const float scare_distance2enemy = 20.f; // distance on which dog can be scared of enemy

} // namespace dog

} // namespace detail

//////////////////////////////////////////////////////////////////////////
// Construct Substates
//////////////////////////////////////////////////////////////////////////

TEMPLATE_SPECIALIZATION
CStateGroupAttackMoveToHomePointAbstract::CStateGroupAttackMoveToHomePoint(_Object* obj)
    : inherited(obj), m_target_node(0), m_skip_camp(false), m_state_started(0)
{
    add_state(eStateAttack_HomePoint_Hide, new CStateMonsterMoveToPointEx<_Object>(obj));
    add_state(eStateAttack_HomePoint_LookOpenPlace, new CStateMonsterLookToPoint<_Object>(obj));

    m_last_tick_enemy_inaccessible = 0;
    m_first_tick_enemy_inaccessible = 0;
}

//////////////////////////////////////////////////////////////////////////
// Initialize/Finalize
//////////////////////////////////////////////////////////////////////////

TEMPLATE_SPECIALIZATION
void CStateGroupAttackMoveToHomePointAbstract::initialize()
{
    inherited::initialize();
    m_target_node = u32(-1);
    m_last_tick_enemy_inaccessible = 0;
    m_first_tick_enemy_inaccessible = 0;
    m_state_started = time();
}

TEMPLATE_SPECIALIZATION
void CStateGroupAttackMoveToHomePointAbstract::finalize()
{
    inherited::finalize();
    CMonsterSquad* squad = monster_squad().get_squad(object);
    squad->unlock_cover(m_target_node);
    m_last_tick_enemy_inaccessible = 0;
    m_first_tick_enemy_inaccessible = 0;
}

TEMPLATE_SPECIALIZATION
void CStateGroupAttackMoveToHomePointAbstract::critical_finalize()
{
    inherited::critical_finalize();

    CMonsterSquad* squad = monster_squad().get_squad(object);
    squad->unlock_cover(m_target_node);
    m_last_tick_enemy_inaccessible = 0;
    m_first_tick_enemy_inaccessible = 0;
}

//////////////////////////////////////////////////////////////////////////
// Check Start Conditions / Completion
//////////////////////////////////////////////////////////////////////////

TEMPLATE_SPECIALIZATION
bool CStateGroupAttackMoveToHomePointAbstract::enemy_inaccessible()
{
    CEntityAlive const* enemy = object->EnemyMan.get_enemy();
    Fvector const enemy_pos = enemy->Position();
    Fvector const enemy_vert_pos = ai().level_graph().vertex_position(enemy->ai_location().level_vertex_id());
    if (enemy_vert_pos.distance_to(enemy_pos) > 1.f)
    {
        return true;
    }
    if (!object->Home->at_home(enemy_pos))
    {
        return true;
    }

    if (!ai().level_graph().valid_vertex_position(enemy_pos))
    {
        return true;
    }

    if (!ai().level_graph().valid_vertex_id(enemy->ai_location().level_vertex_id()))
    {
        return true;
    }

    return false;
}

TEMPLATE_SPECIALIZATION
bool CStateGroupAttackMoveToHomePointAbstract::check_start_conditions()
{
    if (!object->Home->at_home())
    {
        return true;
    }

    if (enemy_inaccessible())
    {
        if (!m_first_tick_enemy_inaccessible)
        {
            m_first_tick_enemy_inaccessible = current_time();
        }

        m_last_tick_enemy_inaccessible = current_time();

        return current_time() - m_first_tick_enemy_inaccessible > 3000;
    }
    else
    {
        if (m_last_tick_enemy_inaccessible && current_time() - m_last_tick_enemy_inaccessible > 3000)
        {
            m_first_tick_enemy_inaccessible = 0;
            m_last_tick_enemy_inaccessible = 0;
        }
    }

    return false;
}

TEMPLATE_SPECIALIZATION
bool CStateGroupAttackMoveToHomePointAbstract::check_completion()
{
    bool const in_camp_state = prev_substate != u32(-1) && prev_substate != eStateAttack_HomePoint_Hide;

    if (m_skip_camp && in_camp_state)
    {
        return true;
    }

    if (!object->Home->at_home() && !in_camp_state)
    {
        return false;
    }

    if (!enemy_inaccessible() && time() > m_state_started + 5000)
    {
        return true;
    }

    if (object->ai_location().level_vertex_id() == m_target_node &&
        !object->control().path_builder().is_moving_on_path())
    {
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
// Select Substate
//////////////////////////////////////////////////////////////////////////

TEMPLATE_SPECIALIZATION
void CStateGroupAttackMoveToHomePointAbstract::reselect_state()
{
    if (prev_substate == eStateAttack_HomePoint_Hide)
    {
        select_state(eStateAttack_HomePoint_LookOpenPlace);
        return;
    }

    select_state(eStateAttack_HomePoint_Hide);
}

//////////////////////////////////////////////////////////////////////////
// Setup Substates
//////////////////////////////////////////////////////////////////////////

TEMPLATE_SPECIALIZATION
void CStateGroupAttackMoveToHomePointAbstract::setup_substates()
{
    state_ptr state = get_state_current();

    if (current_substate == eStateAttack_HomePoint_Hide)
    {
        const CEntityAlive* enemy = object->EnemyMan.get_enemy();

        Fvector enemy2home = object->Home->get_home_point();
        enemy2home.sub(enemy->Position());
        enemy2home.normalize_safe();

        m_target_node = object->Home->get_place_in_max_home_to_direction(enemy2home);

        m_skip_camp = false;

        if (m_target_node == u32(-1))
        {
            m_target_node = object->Home->get_place_in_min_home();
            m_skip_camp = true;
        }

        CMonsterSquad* squad = monster_squad().get_squad(object);
        squad->lock_cover(m_target_node);

        SStateDataMoveToPointEx data;

        data.vertex = m_target_node;
        data.point = ai().level_graph().vertex_position(data.vertex);
        data.action.action = ACT_RUN;
        data.action.time_out = 0; // do not use time out
        data.completion_dist = 1.f;
        data.time_to_rebuild = 0; // do not rebuild
        data.accelerated = true;
        data.braking = true;
        data.accel_type = eAT_Aggressive;
        data.action.sound_type = MonsterSound::eMonsterSoundAggressive;
        data.action.sound_delay = object->db().m_dwAttackSndDelay;

        state->fill_data_with(&data, sizeof(SStateDataMoveToPointEx));
        return;
    }

    if (current_substate == eStateAttack_HomePoint_LookOpenPlace)
    {
        SStateDataLookToPoint data;

        Fvector dir;
        object->CoverMan->less_cover_direction(dir);

        data.point.mad(object->Position(), dir, 10.f);
        data.action.action = ACT_STAND_IDLE;
        data.action.time_out = 2000;
        data.action.sound_type = MonsterSound::eMonsterSoundAggressive;
        data.action.sound_delay = object->db().m_dwIdleSndDelay;
        data.face_delay = 0;

        state->fill_data_with(&data, sizeof(SStateDataLookToPoint));
        return;
    }
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateGroupAttackMoveToHomePointAbstract
