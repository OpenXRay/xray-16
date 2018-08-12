#pragma once

#include "burer_state_attack_tele.h"
#include "burer_state_attack_gravi.h"
#include "burer_state_attack_shield.h"
#include "burer_state_attack_melee.h"
#include "ai/monsters/states/state_look_point.h"
#include "ai/monsters/states/state_move_to_restrictor.h"
#include "burer_state_attack_run_around.h"
#include "burer_state_attack_antiaim.h"

namespace burer
{
float const health_delta = 0.01f;

} // namespace detail

template <typename Object>
CStateBurerAttack<Object>::CStateBurerAttack(Object* obj) : inherited(obj)
{
    this->add_state(eStateBurerAttack_Tele, new CStateBurerAttackTele<Object>(obj));
    this->add_state(eStateBurerAttack_Gravi, new CStateBurerAttackGravi<Object>(obj));
    this->add_state(eStateBurerAttack_Melee, new CStateBurerAttackMelee<Object>(obj));
    this->add_state(eStateBurerAttack_FaceEnemy, new CStateMonsterLookToPoint<Object>(obj));
    this->add_state(eStateBurerAttack_RunAround, new CStateBurerAttackRunAround<Object>(obj));
    this->add_state(eStateBurerAttack_Shield, new CStateBurerShield<Object>(obj));
    this->add_state(eStateBurerAttack_AntiAim, new CStateBurerAntiAim<Object>(obj));
    this->add_state(eStateAttack_Run, new CStateMonsterAttackRun<Object>(obj));
    this->add_state(eStateCustomMoveToRestrictor, new CStateMonsterMoveToRestrictor<Object>(obj));

    m_allow_anti_aim = false;
    m_wait_state_end = false;
    m_lost_delta_health = false;
}

template <typename Object>
void CStateBurerAttack<Object>::initialize()
{
    inherited::initialize();

    m_last_health = this->object->conditions().GetHealth();
    m_lost_delta_health = false;
    m_next_runaway_allowed_tick = 0;
    m_allow_anti_aim = false;
    m_wait_state_end = false;

    // 	CMonsterSquad *squad					=	monster_squad().get_squad(object);
    // 	if ( squad )
    // 	{
    // 		squad->InformSquadAboutEnemy			(object->EnemyMan.get_enemy());
    // 	}
}

template <typename Object>
void CStateBurerAttack<Object>::execute()
{
    CEntityAlive* enemy = const_cast<CEntityAlive*>(this->object->EnemyMan.get_enemy());

    // Notify squad
    CMonsterSquad* squad = monster_squad().get_squad(this->object);
    if (squad)
    {
        SMemberGoal goal;
        goal.type = MG_AttackEnemy;
        goal.entity = enemy;
        squad->UpdateGoal(this->object, goal);
    }

    if (this->object->anim().has_override_animation())
    {
        this->object->anim().clear_override_animation();
    }

    if (this->object->conditions().GetHealth() <= m_last_health - burer::health_delta)
    {
        m_last_health = this->object->conditions().GetHealth();
        m_lost_delta_health = true;
    }

    if (m_wait_state_end)
    {
        if (this->get_state_current()->check_completion())
        {
            m_wait_state_end = false;
        }
        else
        {
            this->get_state_current()->execute();
            this->prev_substate = this->current_substate;
            return;
        }
    }

    m_allow_anti_aim = true;
    bool const anti_aim_ready = this->get_state(eStateBurerAttack_AntiAim)->check_start_conditions();
    m_allow_anti_aim = false;

    bool const gravi_ready = this->get_state(eStateBurerAttack_Gravi)->check_start_conditions();
    bool const shield_ready = this->get_state(eStateBurerAttack_Shield)->check_start_conditions();
    bool const tele_ready = this->get_state(eStateBurerAttack_Tele)->check_start_conditions();

    bool selected_state = true;

    if (gravi_ready)
    {
        this->select_state(eStateBurerAttack_Gravi);
    }
    else if (m_lost_delta_health && shield_ready)
    {
        m_lost_delta_health = false;
        this->select_state(eStateBurerAttack_Shield);
    }
    else if (anti_aim_ready)
    {
        this->select_state(eStateBurerAttack_AntiAim);
    }
    else if (tele_ready && this->current_substate != eStateBurerAttack_RunAround)
    {
        this->select_state(eStateBurerAttack_Tele);
    }
    else
    {
        selected_state = false;
    }

    if (selected_state)
    {
        this->get_state_current()->execute();
        m_wait_state_end = true;
        this->prev_substate = this->current_substate;
        return;
    }

    Fvector const enemy_pos = enemy->Position();
    Fvector const self_pos = this->object->Position();
    Fvector const self2enemy = enemy_pos - self_pos;
    float const self2enemy_dist = magnitude(self2enemy);

    bool const in_runaway_range = self2enemy_dist < this->object->m_runaway_distance;
    bool const in_normal_range = self2enemy_dist < this->object->m_normal_distance;

    if (this->current_substate == eStateCustomMoveToRestrictor)
    {
        if (!this->get_state_current()->check_completion())
        {
            this->get_state_current()->execute();
            this->prev_substate = this->current_substate;
            return;
        }
    }

    if (this->get_state(eStateCustomMoveToRestrictor)->check_start_conditions())
    {
        this->select_state(eStateCustomMoveToRestrictor);
        this->get_state_current()->execute();
        this->prev_substate = this->current_substate;
        return;
    }

    if (this->current_substate == eStateBurerAttack_RunAround)
    {
        if (this->get_state_current()->check_completion())
        {
            if (in_runaway_range)
            {
                m_next_runaway_allowed_tick = current_time() + 5000;
            }
        }
        else
        {
            this->get_state_current()->execute();
            this->prev_substate = this->current_substate;
            return;
        }
    }

    if (m_lost_delta_health || (in_runaway_range && current_time() > m_next_runaway_allowed_tick))
    {
        m_lost_delta_health = false;
        this->select_state(eStateBurerAttack_RunAround);
    }
    else if (!in_normal_range)
    {
        this->select_state(eStateAttack_Run);
    }
    else
    {
        Fvector const self2enemy = enemy_pos - self_pos;
        bool const good_aiming = angle_between_vectors(self2enemy, this->object->Direction()) < deg2rad(20.f);

        this->select_state(eStateBurerAttack_FaceEnemy);

        if (!good_aiming)
        {
            bool const rotate_right = this->object->control().direction().is_from_right(enemy_pos);
            this->object->anim().set_override_animation(rotate_right ? eAnimStandTurnRight : eAnimStandTurnLeft, 0);
            this->object->dir().face_target(enemy_pos);
        }

        this->object->set_action(ACT_STAND_IDLE);
        return;
    }

    this->get_state_current()->execute();
    this->prev_substate = this->current_substate;
}

template <typename Object>
void CStateBurerAttack<Object>::finalize()
{
    if (this->object->anim().has_override_animation())
    {
        this->object->anim().clear_override_animation();
    }

    inherited::finalize();
}

template <typename Object>
void CStateBurerAttack<Object>::critical_finalize()
{
    if (this->object->anim().has_override_animation())
    {
        this->object->anim().clear_override_animation();
    }

    inherited::critical_finalize();
}

template <typename Object>
bool CStateBurerAttack<Object>::check_control_start_conditions(ControlCom::EControlType type)
{
    if (type == ControlCom::eAntiAim)
    {
        return m_allow_anti_aim;
    }

    return true;
}
