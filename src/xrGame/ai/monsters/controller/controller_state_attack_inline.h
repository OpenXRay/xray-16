#pragma once

#include "controller_state_attack_hide.h"
#include "controller_state_attack_hide_lite.h"
#include "controller_state_attack_moveout.h"
#include "controller_state_attack_camp.h"
#include "controller_state_attack_fire.h"
#include "controller_tube.h"

#define CONTROL_FIRE_PERC 80
#define CONTROL_TUBE_PERC 20

#define TEMPLATE_SPECIALIZATION \
    template <typename _Object\
>

#define CStateControllerAttackAbstract CStateControllerAttack<_Object>

TEMPLATE_SPECIALIZATION
CStateControllerAttackAbstract::CStateControllerAttack(_Object* obj) : inherited(obj)
{
    this->add_state(eStateAttack_MoveToHomePoint, xr_new<CStateMonsterAttackMoveToHomePoint<CController>>(obj));
    this->add_state(eStateAttack_Run, xr_new<CStateMonsterAttackRun<CController>>(obj));
    this->add_state(eStateAttack_Melee, xr_new<CStateMonsterAttackMelee<CController>>(obj));
}

TEMPLATE_SPECIALIZATION
void CStateControllerAttackAbstract::initialize() { inherited::initialize(); }
TEMPLATE_SPECIALIZATION
bool CStateControllerAttackAbstract::check_home_point()
{
    if (this->prev_substate != eStateAttack_MoveToHomePoint)
    {
        if (this->get_state(eStateAttack_MoveToHomePoint)->check_start_conditions())
            return true;
    }
    else
    {
        if (!this->get_state(eStateAttack_MoveToHomePoint)->check_completion())
            return true;
    }

    return false;
}

TEMPLATE_SPECIALIZATION
void CStateControllerAttackAbstract::execute()
{
    this->object->anim().clear_override_animation();

    if (check_home_point())
    {
        this->select_state(eStateAttack_MoveToHomePoint);
        this->get_state_current()->execute();
        this->prev_substate = this->current_substate;
        return;
    }

    EMonsterState state_id = eStateUnknown;
    const CEntityAlive* enemy = this->object->EnemyMan.get_enemy();

    if (this->current_substate == eStateAttack_Melee)
    {
        if (this->get_state(eStateAttack_Melee)->check_completion())
            state_id = eStateAttack_Run;
        else
            state_id = eStateAttack_Melee;
    }
    else
    {
        if (this->get_state(eStateAttack_Melee)->check_start_conditions())
            state_id = eStateAttack_Melee;
        else
            state_id = eStateAttack_Run;
    }

    if (!this->object->enemy_accessible() && state_id == eStateAttack_Run)
    {
        this->current_substate = (u32)eStateUnknown;
        this->prev_substate = this->current_substate;

        Fvector dir_xz = this->object->Direction();
        dir_xz.y = 0;
        Fvector self_to_enemy_xz = enemy->Position() - this->object->Position();
        self_to_enemy_xz.y = 0;

        float const angle = angle_between_vectors(dir_xz, self_to_enemy_xz);

        if (_abs(angle) > deg2rad(30.f))
        {
            bool const rotate_right = this->object->control().direction().is_from_right(enemy->Position());
            this->object->anim().set_override_animation(rotate_right ? eAnimStandTurnRight : eAnimStandTurnLeft, 0);
            this->object->dir().face_target(enemy);
        }

        this->object->set_action(ACT_STAND_IDLE);
        return;
    }

    this->select_state(state_id);
    this->get_state_current()->execute();
    this->prev_substate = this->current_substate;
}

TEMPLATE_SPECIALIZATION
void CStateControllerAttackAbstract::setup_substates() {}
TEMPLATE_SPECIALIZATION
void CStateControllerAttackAbstract::check_force_state() {}
TEMPLATE_SPECIALIZATION
void CStateControllerAttackAbstract::finalize()
{
    inherited::finalize();
    // object->set_mental_state(CController::eStateIdle);
}

TEMPLATE_SPECIALIZATION
void CStateControllerAttackAbstract::critical_finalize()
{
    inherited::critical_finalize();
    // object->set_mental_state(CController::eStateIdle);
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateControllerAttackAbstract
