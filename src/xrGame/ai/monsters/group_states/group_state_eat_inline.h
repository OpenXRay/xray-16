#pragma once

#include "ai/monsters/states/state_data.h"
#include "ai/monsters/states/state_move_to_point.h"
#include "ai/monsters/states/state_hide_from_point.h"
#include "ai/monsters/states/state_custom_action.h"
#include "xrPhysics/PhysicsShell.h"
#include "PHMovementControl.h"
#include "CharacterPhysicsSupport.h"
#include "group_state_eat_drag.h"
#include "group_state_custom.h"
#include "group_state_eat_eat.h"

#define TEMPLATE_SPECIALIZATION \
    template <typename _Object\
>

#define CStateGroupEatAbstract CStateGroupEat<_Object>

#define TIME_NOT_HUNGRY 20000

TEMPLATE_SPECIALIZATION
CStateGroupEatAbstract::CStateGroupEat(_Object* obj) : inherited(obj)
{
    this->add_state(eStateEat_CorpseApproachRun, new CStateMonsterMoveToPoint<_Object>(obj));
    this->add_state(eStateEat_CorpseApproachWalk, new CStateMonsterMoveToPoint<_Object>(obj));
    this->add_state(eStateEat_CheckCorpse, new CStateMonsterCustomAction<_Object>(obj));
    this->add_state(eStateEat_Eat, new CStateGroupEating<_Object>(obj));
    this->add_state(eStateEat_WalkAway, new CStateMonsterHideFromPoint<_Object>(obj));
    this->add_state(eStateEat_Rest, new CStateMonsterCustomAction<_Object>(obj));
    this->add_state(eStateEat_Drag, new CStateGroupDrag<_Object>(obj));
    this->add_state(eStateCustom, new CStateCustomGroup<_Object>(obj));
}

TEMPLATE_SPECIALIZATION
CStateGroupEatAbstract::~CStateGroupEat() {}
TEMPLATE_SPECIALIZATION
void CStateGroupEatAbstract::reinit()
{
    inherited::reinit();

    m_time_last_eat = 0;
}

TEMPLATE_SPECIALIZATION
void CStateGroupEatAbstract::initialize()
{
    inherited::initialize();
    corpse = this->object->EatedCorpse;
}

TEMPLATE_SPECIALIZATION
void CStateGroupEatAbstract::finalize()
{
    inherited::finalize();

    if ((corpse == this->object->EatedCorpse) && this->object->EatedCorpse)
    {
        const_cast<CEntityAlive*>(this->object->EatedCorpse)->m_use_timeout = this->object->m_corpse_use_timeout;
        const_cast<CEntityAlive*>(this->object->EatedCorpse)->set_lock_corpse(false);
    }
    if (this->object->character_physics_support()->movement()->PHCapture())
        this->object->character_physics_support()->movement()->PHReleaseObject();
    this->object->EatedCorpse = NULL;
    this->object->b_end_state_eat = true;
}

TEMPLATE_SPECIALIZATION
void CStateGroupEatAbstract::critical_finalize()
{
    inherited::critical_finalize();
    if ((corpse == this->object->EatedCorpse) && this->object->EatedCorpse && check_completion())
    {
        if (this->object->character_physics_support()->movement()->PHCapture())
            this->object->character_physics_support()->movement()->PHReleaseObject();
        const_cast<CEntityAlive*>(this->object->EatedCorpse)->m_use_timeout = this->object->m_corpse_use_timeout;
        const_cast<CEntityAlive*>(this->object->EatedCorpse)->set_lock_corpse(false);
        this->object->EatedCorpse = NULL;
        this->object->b_end_state_eat = true;
    }
    if (this->object->EnemyMan.get_enemy())
        if (this->object->character_physics_support()->movement()->PHCapture())
            this->object->character_physics_support()->movement()->PHReleaseObject();
    this->object->EatedCorpse = NULL;
    this->object->b_end_state_eat = true;
}

TEMPLATE_SPECIALIZATION
void CStateGroupEatAbstract::reselect_state()
{
    if (this->object->b_state_check)
    {
        this->select_state(eStateCustom);
        this->object->b_state_check = false;
        m_time_last_eat = time() + TIME_NOT_HUNGRY;
        return;
    }

    if (this->object->saved_state == eStateEat_Eat)
    {
        this->object->saved_state = u32(-1);
        if (this->object->character_physics_support()->movement()->PHCapture())
            this->object->character_physics_support()->movement()->PHReleaseObject();
        this->select_state(eStateEat_Eat);
        return;
    }

    /*
        if (prev_substate == eStateEat_CorpseApproachRun) { select_state(eStateEat_CheckCorpse); return; }

        if (prev_substate == eStateEat_CheckCorpse) {
            if (object->ability_can_drag()) select_state(eStateEat_Drag);
            else {
                if (get_state(eStateEat_Eat)->check_start_conditions())
                    select_state(eStateEat_Eat);
                else
                    select_state(eStateEat_CorpseApproachWalk);
            }
            return;
        }*/

    if (this->prev_substate == u32(-1))
    {
        this->select_state(eStateEat_CorpseApproachWalk);
        return;
    }

    if (this->prev_substate == eStateEat_CorpseApproachWalk)
    {
        if (!this->get_state(eStateEat_CorpseApproachWalk)->check_completion())
        {
            this->select_state(eStateEat_CorpseApproachWalk);
            return;
        }
        // Lain: added
        if (this->object->ability_can_drag() && this->object->check_eated_corpse_draggable())
        {
            this->select_state(eStateEat_Drag);
        }
        else
        {
            if (this->get_state(eStateEat_Eat)->check_start_conditions())
                this->select_state(eStateEat_Eat);
            else
                this->select_state(eStateEat_CorpseApproachWalk);
        }
        return;
    }

    if (this->prev_substate == eStateEat_Drag)
    {
        if (!this->get_state(eStateEat_Drag)->check_completion())
        {
            this->select_state(eStateEat_Drag);
            return;
        }

        if (this->get_state(eStateEat_Eat)->check_start_conditions())
        {
            this->object->set_current_animation(15);
            this->object->saved_state = eStateEat_Eat;
            this->select_state(eStateCustom);
            this->object->b_state_check = false;
        }
        else
        {
            this->select_state(eStateEat_CorpseApproachWalk);
        }
        return;
    }

    if (this->prev_substate == eStateEat_Eat)
    {
        m_time_last_eat = time();

        if (!hungry())
            this->select_state(eStateEat_WalkAway);
        else
            this->select_state(eStateEat_CorpseApproachWalk);
        return;
    }

    if (this->prev_substate == eStateEat_WalkAway)
    {
        this->select_state(eStateEat_Rest);
        return;
    }

    if (this->prev_substate == eStateEat_Rest)
    {
        this->select_state(eStateEat_Rest);
        return;
    }

    this->select_state(eStateEat_Rest);
}

TEMPLATE_SPECIALIZATION
void CStateGroupEatAbstract::setup_substates()
{
    state_ptr state = this->get_state_current();

    if (this->current_substate == eStateEat_CorpseApproachRun)
    {
        // Определить позицию ближайшей кости у трупа
        Fvector nearest_bone_pos;
        const CEntityAlive* corpse = this->object->EatedCorpse;
        if (corpse->m_pPhysicsShell == nullptr || !corpse->m_pPhysicsShell->isActive())
        {
            nearest_bone_pos = corpse->Position();
        }
        else
            nearest_bone_pos = this->object->character_physics_support()->movement()->PHCaptureGetNearestElemPos(corpse);

        SStateDataMoveToPoint data;
        data.point = nearest_bone_pos;
        data.vertex = u32(-1);
        data.action.action = ACT_RUN;
        data.accelerated = true;
        data.braking = true;
        data.accel_type = eAT_Calm;
        data.completion_dist = this->object->db().m_fDistToCorpse;
        data.action.sound_type = MonsterSound::eMonsterSoundIdle;
        data.action.sound_delay = this->object->db().m_dwIdleSndDelay;

        state->fill_data_with(&data, sizeof(SStateDataMoveToPoint));
        return;
    }

    if (this->current_substate == eStateEat_CheckCorpse)
    {
        SStateDataAction data;
        data.action = ACT_STAND_IDLE;
        data.spec_params = 0;
        data.time_out = 500;
        data.sound_type = MonsterSound::eMonsterSoundEat;
        data.sound_delay = this->object->db().m_dwEatSndDelay;

        state->fill_data_with(&data, sizeof(SStateDataAction));

        return;
    }

    if (this->current_substate == eStateEat_WalkAway)
    {
        SStateHideFromPoint data;

        data.point = this->object->EatedCorpse->Position();
        data.action.action = ACT_WALK_FWD;
        data.distance = 15.f;
        data.accelerated = true;
        data.braking = true;
        data.accel_type = eAT_Calm;
        data.cover_min_dist = 20.f;
        data.cover_max_dist = 30.f;
        data.cover_search_radius = 25.f;
        data.action.sound_type = MonsterSound::eMonsterSoundIdle;
        data.action.sound_delay = this->object->db().m_dwIdleSndDelay;

        state->fill_data_with(&data, sizeof(SStateHideFromPoint));

        return;
    }

    if (this->current_substate == eStateEat_Rest)
    {
        SStateDataAction data;
        data.action = ACT_STAND_IDLE;
        data.spec_params = 0;
        data.time_out = 500;
        data.sound_type = MonsterSound::eMonsterSoundIdle;
        data.sound_delay = this->object->db().m_dwIdleSndDelay;

        state->fill_data_with(&data, sizeof(SStateDataAction));

        return;
    }

    if (this->current_substate == eStateEat_CorpseApproachWalk)
    {
        // Определить позицию ближайшей кости у трупа
        Fvector nearest_bone_pos;
        const CEntityAlive* corpse = this->object->EatedCorpse;

#ifdef DEBUG
        if (!corpse)
        {
            debug::text_tree tree;
            this->object->add_debug_info(tree);
            debug::log_text_tree(tree);
            FATAL("Debug info has been added, plz save log");
        }
#endif //#ifdef DEBUG

        if (corpse->m_pPhysicsShell == nullptr || !corpse->m_pPhysicsShell->isActive())
        {
            nearest_bone_pos = corpse->Position();
        }
        else
            nearest_bone_pos = this->object->character_physics_support()->movement()->PHCaptureGetNearestElemPos(corpse);

        SStateDataMoveToPoint data;
        data.point = nearest_bone_pos;
        data.vertex = u32(-1);
        data.action.action = ACT_WALK_FWD;
        data.accelerated = true;
        data.braking = true;
        data.accel_type = eAT_Calm;
        data.completion_dist = this->object->db().m_fDistToCorpse;
        data.action.sound_type = MonsterSound::eMonsterSoundIdle;
        data.action.sound_delay = this->object->db().m_dwIdleSndDelay;

        state->fill_data_with(&data, sizeof(SStateDataMoveToPoint));
        return;
    }
}

TEMPLATE_SPECIALIZATION
bool CStateGroupEatAbstract::check_completion()
{
    if (corpse != this->object->EatedCorpse)
        return true;
    if (!hungry())
        return true;

    return false;
}

TEMPLATE_SPECIALIZATION
bool CStateGroupEatAbstract::check_start_conditions()
{
    if (this->object->EatedCorpse)
        return true;
    return (this->object->CorpseMan.get_corpse() && this->object->Home->at_home(this->object->CorpseMan.get_corpse()->Position()) &&
        hungry() && !const_cast<CEntityAlive*>(this->object->CorpseMan.get_corpse())->is_locked_corpse());
}

TEMPLATE_SPECIALIZATION
bool CStateGroupEatAbstract::hungry()
{
    return ((m_time_last_eat == 0) || (m_time_last_eat + TIME_NOT_HUNGRY < time()));
}

TEMPLATE_SPECIALIZATION
void CStateGroupEatAbstract::remove_links(IGameObject* object)
{
    if (corpse == object)
        corpse = 0;
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateGroupEatAbstract
