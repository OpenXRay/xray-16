#pragma once

#include "ai/monsters/states/state_data.h"
#include "ai/monsters/states/state_move_to_point.h"
#include "ai/monsters/states/state_look_unprotected_area.h"
#include "group_state_panic_run.h"
#include "ai/monsters/states/monster_state_home_point_attack.h"

#define TEMPLATE_SPECIALIZATION \
    template <typename _Object\
>

#define CStateGroupPanicAbstract CStateGroupPanic<_Object>

TEMPLATE_SPECIALIZATION
CStateGroupPanicAbstract::CStateGroupPanic(_Object* obj) : inherited(obj)
{
    this->add_state(eStatePanic_Run, new CStateGroupPanicRun<_Object>(obj));
    this->add_state(eStatePanic_FaceUnprotectedArea, new CStateMonsterLookToUnprotectedArea<_Object>(obj));
    this->add_state(eStatePanic_MoveToHomePoint, new CStateMonsterAttackMoveToHomePoint<_Object>(obj));
}

TEMPLATE_SPECIALIZATION
CStateGroupPanicAbstract::~CStateGroupPanic() {}
TEMPLATE_SPECIALIZATION
void CStateGroupPanicAbstract::initialize() { inherited::initialize(); }
TEMPLATE_SPECIALIZATION
void CStateGroupPanicAbstract::reselect_state()
{
    if (this->get_state(eStatePanic_MoveToHomePoint)->check_start_conditions())
    {
        this->select_state(eStatePanic_MoveToHomePoint);
        return;
    }

    if (this->prev_substate == eStatePanic_Run)
        this->select_state(eStatePanic_FaceUnprotectedArea);
    else
        this->select_state(eStatePanic_Run);
}

TEMPLATE_SPECIALIZATION
void CStateGroupPanicAbstract::setup_substates()
{
    state_ptr state = this->get_state_current();

    if (this->current_substate == eStatePanic_FaceUnprotectedArea)
    {
        SStateDataAction data;

        data.action = ACT_STAND_IDLE;
        data.spec_params = ASP_STAND_SCARED;
        data.time_out = 3000;
        data.sound_type = MonsterSound::eMonsterSoundPanic;
        data.sound_delay = this->object->db().m_dwAttackSndDelay;

        state->fill_data_with(&data, sizeof(SStateDataAction));

        return;
    }
}

TEMPLATE_SPECIALIZATION
void CStateGroupPanicAbstract::check_force_state()
{
    if ((this->current_substate == eStatePanic_FaceUnprotectedArea))
    {
        // если видит врага
        if (this->object->EnemyMan.get_enemy_time_last_seen() == Device.dwTimeGlobal)
        {
            this->select_state(eStatePanic_Run);
            return;
        }
        // если получил hit
        if (this->object->HitMemory.get_last_hit_time() + 5000 > Device.dwTimeGlobal)
        {
            this->select_state(eStatePanic_Run);
            return;
        }
    }
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateGroupPanicAbstract
