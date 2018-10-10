#pragma once

#include "ai/monsters/states/state_move_to_point.h"

#define TEMPLATE_SPECIALIZATION \
    template <typename _Object\
>

#define CBloodsuckerStateAttackAbstract CBloodsuckerStateAttack<_Object>

TEMPLATE_SPECIALIZATION
CBloodsuckerStateAttackAbstract::CBloodsuckerStateAttack(_Object* obj) : inherited_attack(obj)
{
    add_state(eStateAttack_Hide, new CStateMonsterBackstubEnemy<_Object>(obj));
    add_state(eStateVampire_Execute, new CStateBloodsuckerVampireExecute<_Object>(obj));
}

TEMPLATE_SPECIALIZATION
CBloodsuckerStateAttackAbstract::~CBloodsuckerStateAttack() {}
TEMPLATE_SPECIALIZATION
void CBloodsuckerStateAttackAbstract::initialize()
{
    inherited::initialize();
    m_time_stop_invis = 0;
    m_last_health = object->conditions().GetHealth();
}

TEMPLATE_SPECIALIZATION
void CBloodsuckerStateAttackAbstract::finalize()
{
    inherited::finalize();
    object->start_invisible_predator();
}

TEMPLATE_SPECIALIZATION
void CBloodsuckerStateAttackAbstract::critical_finalize()
{
    inherited::critical_finalize();
    object->start_invisible_predator();
}

namespace detail
{
namespace bloodsucker
{
const u32 encircle_time = 3000;
const float loose_health_diff = 0.15f;
const u32 change_behaviour_time = 1000;

} // namespace bloodsucker

} // namespace detail

TEMPLATE_SPECIALIZATION
void CBloodsuckerStateAttackAbstract::execute()
{
    if (check_home_point())
        select_state(eStateAttack_MoveToHomePoint);
    else if (check_vampire())
        select_state(eStateVampire_Execute);
    else if (check_steal_state())
        select_state(eStateAttack_Steal);
    else if (check_camp_state())
        select_state(eStateAttackCamp);
    else if (check_find_enemy_state())
        select_state(eStateAttack_FindEnemy);
    else if (check_hiding())
        select_state(eStateAttack_Hide);
    else if (check_run_attack_state())
        select_state(eStateAttack_RunAttack);
    else
    {
        // определить тип атаки
        bool b_melee = false;

        if (prev_substate == eStateAttack_Melee)
        {
            if (!get_state_current()->check_completion())
            {
                b_melee = true;
            }
        }
        else if (get_state(eStateAttack_Melee)->check_start_conditions())
        {
            b_melee = true;
        }

        if (!b_melee && (prev_substate == eStateAttack_Melee))
        {
            select_state(eStateAttack_Hide);
        }
        else
            // установить целевое состояние
            if (b_melee)
        {
            // check if enemy is behind me for a long time
            // [TODO] make specific state and replace run_away state (to avoid ratation jumps)
            // if (check_behinder())
            //	select_state(eStateAttack_RunAway);
            // else
            select_state(eStateAttack_Melee);
        }
        else
        {
            select_state(eStateAttack_Run);
        }
    }

    // clear behinder var if not melee state selected
    if (current_substate != eStateAttack_Melee)
    {
        m_time_start_check_behinder = 0;
    }
    else
    {
        object->clear_runaway_invisible();
    }

    get_state_current()->execute();
    prev_substate = current_substate;

    // Notify squad
    CMonsterSquad* squad = monster_squad().get_squad(object);
    if (squad)
    {
        SMemberGoal goal;

        goal.type = MG_AttackEnemy;
        goal.entity = const_cast<CEntityAlive*>(object->EnemyMan.get_enemy());

        squad->UpdateGoal(object, goal);
    }
}

TEMPLATE_SPECIALIZATION
bool CBloodsuckerStateAttackAbstract::check_vampire()
{
    if (prev_substate != eStateVampire_Execute)
    {
        if (get_state(eStateVampire_Execute)->check_start_conditions())
            return true;
    }
    else
    {
        if (!get_state(eStateVampire_Execute)->check_completion())
            return true;
    }
    return false;
}

TEMPLATE_SPECIALIZATION
bool CBloodsuckerStateAttackAbstract::check_hiding()
{
    const bool health_step_lost =
        object->conditions().GetHealth() < m_last_health - detail::bloodsucker::loose_health_diff;

    if (health_step_lost)
    {
        object->start_runaway_invisible();
        m_last_health = object->conditions().GetHealth();
        m_start_with_encircle = true;
        return true;
    }

    // if we get here before 1 sec after last critical hit:
    u32 last_critical_hit_tick = object->get_last_critical_hit_tick();
    if (last_critical_hit_tick && time() < last_critical_hit_tick + 1000)
    {
        object->clear_last_critical_hit_tick();
        m_start_with_encircle = true;
        return true;
    }

    if (current_substate == eStateAttack_Hide)
    {
        return !get_state_current()->check_completion();
    }

    m_start_with_encircle = false;
    return get_state(eStateAttack_Hide)->check_start_conditions();
}

TEMPLATE_SPECIALIZATION
void CBloodsuckerStateAttackAbstract::setup_substates()
{
    typename inherited::state_ptr state = get_state_current();

    if (current_substate == eStateAttack_Hide)
    {
        typename CStateMonsterBackstubEnemy<_Object>::StateParams data;

        data.action.action = ACT_RUN;
        data.action.time_out = 0;
        data.completion_dist = 1.f; // get exactly to the point
        data.time_to_rebuild = 200;
        data.accelerated = true;
        data.braking = false;
        data.accel_type = eAT_Aggressive;
        data.action.sound_type = MonsterSound::eMonsterSoundIdle;
        data.action.sound_delay = object->db().m_dwIdleSndDelay;
        data.start_with_encircle = m_start_with_encircle;

        state->fill_data_with(&data, sizeof(CStateMonsterBackstubEnemy<_Object>::StateParams));
        return;
    }
}

#undef TEMPLATE_SPECIALIZATION
#undef CBloodsuckerStateAttackAbstract

//////////////////////////////////////////////////////////////////////////
// CStateMonsterMoveToPointEx with path rebuild options
//////////////////////////////////////////////////////////////////////////

template <class _Object>
void CStateMonsterBackstubEnemy<_Object>::initialize()
{
    inherited::initialize();
    object->path().prepare_builder();
    m_last_health = object->conditions().GetHealth();
    m_encircle = data.start_with_encircle;
    m_encircle_end_tick = Device.dwTimeGlobal + detail::bloodsucker::encircle_time;
    m_next_change_behaviour_tick = 0;
}

template <class _Object>
void CStateMonsterBackstubEnemy<_Object>::execute()
{
    // on hit, change behaviour
    if (object->conditions().GetHealth() < m_last_health - detail::bloodsucker::loose_health_diff &&
        Device.dwTimeGlobal > m_next_change_behaviour_tick)
    {
        m_next_change_behaviour_tick = Device.dwTimeGlobal + detail::bloodsucker::change_behaviour_time;
        m_last_health = object->conditions().GetHealth();
        m_encircle = !m_encircle;
        if (m_encircle)
        {
            m_encircle_end_tick = Device.dwTimeGlobal + detail::bloodsucker::encircle_time;
        }
    }

    if (Device.dwTimeGlobal > m_encircle_end_tick)
    {
        if (object->EnemyMan.enemy_see_me_now())
        {
            m_encircle = false;
        }
    }

    object->set_action(data.action.action);
    object->anim().SetSpecParams(data.action.spec_params);

    data.point = object->EnemyMan.get_enemy_position();
    data.vertex = 0;

    data.target_direction = Fvector().set(0.f, 0.f, 0.f);
    const CEntityAlive* enemy = object->EnemyMan.get_enemy();
    VERIFY(enemy);

    const SRotation rot = enemy->Orientation();
    data.target_direction.setHP(rot.yaw, rot.pitch);

    object->path().set_target_point(data.point, data.vertex);
    object->path().set_rebuild_time(data.time_to_rebuild);
    object->path().set_distance_to_end(data.completion_dist);
    object->path().set_use_covers();
    object->path().set_cover_params(5.f, 30.f, 1.f, 30.f);

    if (m_encircle)
    {
        object->path().set_use_dest_orient(true);
        object->path().set_dest_direction(data.target_direction);
        object->path().set_try_min_time(false);
    }
    else
    {
        object->path().set_try_min_time(true);
        object->path().set_use_dest_orient(false);
    }

    if (data.accelerated)
    {
        object->anim().accel_activate(EAccelType(data.accel_type));
        object->anim().accel_set_braking(data.braking);
    }

    if (data.action.sound_type != u32(-1))
    {
        object->set_state_sound(data.action.sound_type, data.action.sound_delay == u32(-1));
    }
}

template <class _Object>
bool CStateMonsterBackstubEnemy<_Object>::check_start_conditions()
{
    if (!object->Home->at_home(object->EnemyMan.get_enemy_position()))
    {
        return false;
    }

    float dist = object->MeleeChecker.distance_to_enemy(object->EnemyMan.get_enemy());

    return dist > object->MeleeChecker.get_min_distance();
}

template <class _Object>
bool CStateMonsterBackstubEnemy<_Object>::check_completion()
{
    if (!object->Home->at_home(object->EnemyMan.get_enemy_position()))
    {
        return true;
    }

    const bool real_path_end = fis_zero(data.completion_dist) ?
        (data.point.distance_to_xz(object->Position()) < ai().level_graph().header().cell_size()) :
        true;

    if (object->control().path_builder().is_path_end(data.completion_dist) && real_path_end)
    {
        // in straight-mode we're done
        if (!m_encircle)
        {
            return true;
        }

        if (object->EnemyMan.see_enemy_now() && !object->EnemyMan.enemy_see_me_now())
        {
            // object->sound().play(MonsterSound::eMonsterSoundSteal);
            return true;
        }
    }

    return false;
}
