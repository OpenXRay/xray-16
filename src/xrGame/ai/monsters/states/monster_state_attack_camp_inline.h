#pragma once

#include "state_move_to_point.h"
#include "state_look_point.h"
#include "monster_state_attack_camp_stealout.h"
#include "cover_point.h"
#include "ai/monsters/monster_cover_manager.h"

#define TEMPLATE_SPECIALIZATION \
    template <typename _Object\
>

#define CStateMonsterAttackCampAbstract CStateMonsterAttackCamp<_Object>

TEMPLATE_SPECIALIZATION
CStateMonsterAttackCampAbstract::CStateMonsterAttackCamp(_Object* obj) : inherited(obj)
{
    this->add_state(eStateAttackCamp_Hide, xr_new<CStateMonsterMoveToPointEx<_Object>>(obj));
    this->add_state(eStateAttackCamp_Camp, xr_new<CStateMonsterLookToPoint<_Object>>(obj));
    this->add_state(eStateAttackCamp_StealOut, xr_new<CStateMonsterAttackCampStealOut<_Object>>(obj));
}

TEMPLATE_SPECIALIZATION
void CStateMonsterAttackCampAbstract::initialize()
{
    inherited::initialize();

    CMonsterSquad* squad = monster_squad().get_squad(this->object);
    squad->lock_cover(m_target_node);
}

TEMPLATE_SPECIALIZATION
void CStateMonsterAttackCampAbstract::finalize()
{
    inherited::finalize();
    CMonsterSquad* squad = monster_squad().get_squad(this->object);
    squad->unlock_cover(m_target_node);
}

TEMPLATE_SPECIALIZATION
void CStateMonsterAttackCampAbstract::critical_finalize()
{
    inherited::critical_finalize();

    CMonsterSquad* squad = monster_squad().get_squad(this->object);
    squad->unlock_cover(m_target_node);
}

TEMPLATE_SPECIALIZATION
bool CStateMonsterAttackCampAbstract::check_completion()
{
    if (this->current_substate == eStateAttackCamp_StealOut)
    {
        return this->get_state_current()->check_completion();
    }

    if (this->current_substate == eStateAttackCamp_Camp)
    {
        if (this->object->EnemyMan.see_enemy_now())
            return true;
        if (this->object->HitMemory.get_last_hit_time() >this->get_state_current()->time_started())
            return true;
    }

    if (this->object->EnemyMan.get_enemy()->Position().distance_to(this->object->Position()) < 5.f)
        return true;

    return false;
}

#define MIN_DISTANCE_TO_ENEMY 20.f

TEMPLATE_SPECIALIZATION
bool CStateMonsterAttackCampAbstract::check_start_conditions()
{
    if (!this->object->ability_distant_feel())
        return false;

    // check enemy
    if (!this->object->EnemyMan.get_enemy())
        return false;

    // check distance to enemy
    if (this->object->Position().distance_to(this->object->EnemyMan.get_enemy_position()) < MIN_DISTANCE_TO_ENEMY)
        return false;

    // check if enemy see me
    // if (EnemyMan.get_flags().is(FLAG_ENEMY_DOESNT_SEE_ME)) return false;

    // try to get cover
    const CCoverPoint* point = this->object->CoverMan->find_cover(this->object->EnemyMan.get_enemy_position(), 10.f, 30.f);
    if (!point)
        return false;

    m_target_node = point->level_vertex_id();

    return true;
}

TEMPLATE_SPECIALIZATION
void CStateMonsterAttackCampAbstract::reselect_state()
{
    if (this->prev_substate == u32(-1))
    {
        this->select_state(eStateAttackCamp_Hide);
        return;
    }

    if (this->prev_substate == eStateAttackCamp_Hide)
    {
        this->select_state(eStateAttackCamp_Camp);
        return;
    }

    if (this->prev_substate == eStateAttackCamp_Camp)
    {
        if (this->get_state(eStateAttackCamp_StealOut)->check_start_conditions())
            this->select_state(eStateAttackCamp_StealOut);
        else
            this->select_state(eStateAttackCamp_Hide);
        return;
    }

    if (this->prev_substate == eStateAttackCamp_StealOut)
    {
        this->select_state(eStateAttackCamp_Camp);
        return;
    }
}

TEMPLATE_SPECIALIZATION
void CStateMonsterAttackCampAbstract::setup_substates()
{
    state_ptr state = this->get_state_current();

    if (this->current_substate == eStateAttackCamp_Hide)
    {
        SStateDataMoveToPointEx data;

        data.vertex = m_target_node;
        data.point = ai().level_graph().vertex_position(data.vertex);
        data.action.action = ACT_RUN;
        data.action.time_out = 0; // do not use time out
        data.completion_dist = 1.f; // get exactly to the point
        data.time_to_rebuild = 0; // do not rebuild
        data.accelerated = true;
        data.braking = false;
        data.accel_type = eAT_Aggressive;
        data.action.sound_type = MonsterSound::eMonsterSoundIdle;
        data.action.sound_delay = this->object->db().m_dwIdleSndDelay;

        state->fill_data_with(&data, sizeof(SStateDataMoveToPointEx));
        return;
    }

    if (this->current_substate == eStateAttackCamp_Camp)
    {
        SStateDataLookToPoint data;

        Fvector dir;
        this->object->CoverMan->less_cover_direction(dir);

        data.point.mad(this->object->Position(), dir, 10.f);
        data.action.action = ACT_STAND_IDLE;
        data.action.time_out = 10000; // do not use time out
        data.action.sound_type = MonsterSound::eMonsterSoundIdle;
        data.action.sound_delay = this->object->db().m_dwIdleSndDelay;
        data.face_delay = 0;

        state->fill_data_with(&data, sizeof(SStateDataLookToPoint));
        return;
    }
}

TEMPLATE_SPECIALIZATION
void CStateMonsterAttackCampAbstract::check_force_state() {}
#undef TEMPLATE_SPECIALIZATION
#undef CStateMonsterAttackCampAbstract
#undef MIN_DISTANCE_TO_ENEMY
