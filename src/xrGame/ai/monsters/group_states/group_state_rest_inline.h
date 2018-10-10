#pragma once
#include "ai/monsters/states/monster_state_rest_sleep.h"
#include "ai/monsters/states/state_move_to_restrictor.h"
#include "ai/monsters/ai_monster_squad.h"
#include "ai/monsters/ai_monster_squad_manager.h"
#include "ai/monsters/monster_home.h"
#include "ai/monsters/anomaly_detector.h"
#include "ai/monsters/states/monster_state_home_point_rest.h"
#include "ai/monsters/states/monster_state_smart_terrain_task.h"
#include "group_state_rest_idle.h"
#include "group_state_custom.h"

#define TEMPLATE_SPECIALIZATION \
    template <typename _Object\
>

#define CStateGroupRestAbstract CStateGroupRest<_Object>

TEMPLATE_SPECIALIZATION
CStateGroupRestAbstract::CStateGroupRest(_Object* obj) : inherited(obj)
{
    this->add_state(eStateRest_Sleep, new CStateMonsterRestSleep<_Object>(obj));
    this->add_state(eStateCustomMoveToRestrictor, new CStateMonsterMoveToRestrictor<_Object>(obj));
    this->add_state(eStateRest_MoveToHomePoint, new CStateMonsterRestMoveToHomePoint<_Object>(obj));
    this->add_state(eStateSmartTerrainTask, new CStateMonsterSmartTerrainTask<_Object>(obj));
    this->add_state(eStateRest_Idle, new CStateGroupRestIdle<_Object>(obj));
    this->add_state(eStateCustom, new CStateCustomGroup<_Object>(obj));
}

TEMPLATE_SPECIALIZATION
CStateGroupRestAbstract::~CStateGroupRest() {}
TEMPLATE_SPECIALIZATION
void CStateGroupRestAbstract::initialize()
{
    inherited::initialize();
    time_for_sleep = 0;
    time_for_life = time() + this->object->m_min_life_time + Random.randI(10) * this->object->m_min_life_time;
    this->object->anomaly_detector().activate();
}

TEMPLATE_SPECIALIZATION
void CStateGroupRestAbstract::finalize()
{
    inherited::finalize();

    this->object->anomaly_detector().deactivate();
}

TEMPLATE_SPECIALIZATION
void CStateGroupRestAbstract::critical_finalize()
{
    inherited::critical_finalize();

    this->object->anomaly_detector().deactivate();
}

TEMPLATE_SPECIALIZATION
void CStateGroupRestAbstract::execute()
{
    // check alife control

    bool captured_by_smart_terrain = false;

    if (this->prev_substate == eStateSmartTerrainTask)
    {
        if (!this->get_state(eStateSmartTerrainTask)->check_completion())
            captured_by_smart_terrain = true;
    }
    else if (this->get_state(eStateSmartTerrainTask)->check_start_conditions())
        captured_by_smart_terrain = true;

    if (captured_by_smart_terrain)
        this->select_state(eStateSmartTerrainTask);
    else
    {
        // check restrictions
        bool move_to_restrictor = false;

        if (this->prev_substate == eStateCustomMoveToRestrictor)
        {
            if (!this->get_state(eStateCustomMoveToRestrictor)->check_completion())
                move_to_restrictor = true;
        }
        else if (this->get_state(eStateCustomMoveToRestrictor)->check_start_conditions())
            move_to_restrictor = true;

        if (move_to_restrictor)
            this->select_state(eStateCustomMoveToRestrictor);
        else
        {
            // check home point
            bool move_to_home_point = false;

            if (this->prev_substate == eStateRest_MoveToHomePoint)
            {
                if (!this->get_state(eStateRest_MoveToHomePoint)->check_completion())
                    move_to_home_point = true;
            }
            else if (this->get_state(eStateRest_MoveToHomePoint)->check_start_conditions())
                move_to_home_point = true;

            if (move_to_home_point)
                this->select_state(eStateRest_MoveToHomePoint);
            else
            {
                // check squad behaviour
                if (this->object->saved_state == eStateRest_Sleep)
                {
                    switch (this->object->get_number_animation())
                    {
                    case u32(8):this->object->set_current_animation(13); break;
                    case u32(14): this->object->set_current_animation(12); break;
                    case u32(12):
                        this->object->set_current_animation(7);
                        this->object->saved_state = u32(-1);
                        break;
                    default: break;
                    }
                    if (this->object->b_state_check)
                    {
                        this->object->b_state_check = false;
                        this->select_state(eStateCustom);
                        this->get_state_current()->execute();
                        this->prev_substate = this->current_substate;
                        return;
                    }
                }
                if (time() < time_for_sleep && this->object->saved_state == eStateRest_Sleep &&
                    this->object->get_number_animation() == u32(13))
                {
                    this->select_state(eStateRest_Sleep);
                    this->get_state_current()->execute();
                    this->prev_substate = this->current_substate;
                    return;
                }
                bool use_to_do = false;
                if (this->prev_substate == eStateRest_Sleep)
                {
                    if (time() <= time_for_sleep)
                    {
                        use_to_do = true;
                    }
                    else
                    {
                        time_for_life = time() + this->object->m_min_life_time + Random.randI(10) * this->object->m_min_life_time;
                        this->object->set_current_animation(14);
                        this->select_state(eStateCustom);
                        this->object->b_state_check = false;
                        this->get_state_current()->execute();
                        this->prev_substate = this->current_substate;
                        return;
                    }
                }
                if (!use_to_do)
                {
                    if (time() > time_for_life && this->object->Home->at_min_home(this->object->Position()))
                    {
                        this->object->set_current_animation(8);
                        this->select_state(eStateCustom);
                        this->object->saved_state = eStateRest_Sleep;
                        time_for_sleep = time() + this->object->m_min_sleep_time + Random.randI(5) * this->object->m_min_sleep_time;
                        use_to_do = true;
                        this->object->b_state_check = false;
                        this->get_state_current()->execute();
                        this->prev_substate = this->current_substate;
                        return;
                    }
                    else
                    {
                        if (this->object->saved_state != eStateRest_Sleep && this->prev_substate == eStateCustom &&
                            this->object->get_number_animation() >= u32(8) && this->object->get_number_animation() < u32(12))
                        {
                            this->object->set_current_animation(this->object->get_number_animation() + u32(1));
                            this->select_state(eStateCustom);
                            this->object->b_state_check = false;
                            this->get_state_current()->execute();
                            this->prev_substate = this->current_substate;
                            return;
                        }
                        if (this->object->b_state_check)
                        {
                            this->select_state(eStateCustom);
                            this->object->b_state_check = false;
                        }
                        else
                        {
                            this->select_state(eStateRest_Idle);
                        }
                    }
                }
            }
        }
    }

    this->get_state_current()->execute();
    this->prev_substate = this->current_substate;
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateGroupRestAbstract
