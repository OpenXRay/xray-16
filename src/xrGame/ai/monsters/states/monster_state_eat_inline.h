#pragma once

#include "state_data.h"
#include "state_move_to_point.h"
#include "state_hide_from_point.h"
#include "state_custom_action.h"
#include "monster_state_eat_eat.h"
#include "monster_state_eat_drag.h"
#include "xrPhysics/PhysicsShell.h"
#include "PHMovementControl.h"
#include "CharacterPhysicsSupport.h"
#ifdef _DEBUG
#include "Level.h"
#include "level_debug.h"
#endif

#define TEMPLATE_SPECIALIZATION \
    template <typename _Object\
>

#define CStateMonsterEatAbstract CStateMonsterEat<_Object>

TEMPLATE_SPECIALIZATION
CStateMonsterEatAbstract::CStateMonsterEat(_Object* obj) : inherited(obj)
{
    this->add_state(eStateEat_CorpseApproachRun, xr_new<CStateMonsterMoveToPoint<_Object>>(obj));
    this->add_state(eStateEat_CorpseApproachWalk, xr_new<CStateMonsterMoveToPoint<_Object>>(obj));
    this->add_state(eStateEat_CheckCorpse, xr_new<CStateMonsterCustomAction<_Object>>(obj));
    this->add_state(eStateEat_Eat, xr_new<CStateMonsterEating<_Object>>(obj));
    this->add_state(eStateEat_WalkAway, xr_new<CStateMonsterHideFromPoint<_Object>>(obj));
    this->add_state(eStateEat_Rest, xr_new<CStateMonsterCustomAction<_Object>>(obj));
    this->add_state(eStateEat_Drag, xr_new<CStateMonsterDrag<_Object>>(obj));
}

TEMPLATE_SPECIALIZATION
CStateMonsterEatAbstract::~CStateMonsterEat() {}
TEMPLATE_SPECIALIZATION
void CStateMonsterEatAbstract::reinit()
{
    inherited::reinit();

    m_time_last_eat = 0;
}

TEMPLATE_SPECIALIZATION
void CStateMonsterEatAbstract::initialize()
{
    inherited::initialize();
    corpse = this->object->CorpseMan.get_corpse();

    monster_squad().get_squad(this->object)->lock_corpse(this->object->CorpseMan.get_corpse());
}

TEMPLATE_SPECIALIZATION
void CStateMonsterEatAbstract::finalize()
{
    inherited::finalize();

    monster_squad().get_squad(this->object)->unlock_corpse(this->object->CorpseMan.get_corpse());
}

TEMPLATE_SPECIALIZATION
void CStateMonsterEatAbstract::critical_finalize()
{
    inherited::critical_finalize();

    monster_squad().get_squad(this->object)->unlock_corpse(this->object->CorpseMan.get_corpse());
}

TEMPLATE_SPECIALIZATION
void CStateMonsterEatAbstract::reselect_state()
{
    if (this->prev_substate == u32(-1))
    {
        this->select_state(eStateEat_CorpseApproachRun);
        return;
    }
    if (this->prev_substate == eStateEat_CorpseApproachRun)
    {
        this->select_state(eStateEat_CheckCorpse);
        return;
    }

    if (this->prev_substate == eStateEat_CheckCorpse)
    {
        if (this->object->ability_can_drag())
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
        if (this->get_state(eStateEat_Eat)->check_start_conditions())
            this->select_state(eStateEat_Eat);
        else
            this->select_state(eStateEat_CorpseApproachWalk);
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

    if (this->prev_substate == eStateEat_CorpseApproachWalk)
    {
        if (this->get_state(eStateEat_Eat)->check_start_conditions())
            this->select_state(eStateEat_Eat);
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
}

TEMPLATE_SPECIALIZATION
void CStateMonsterEatAbstract::setup_substates()
{
    state_ptr state = this->get_state_current();

    if (this->current_substate == eStateEat_CorpseApproachRun)
    {
        // Определить позицию ближайшей боны у трупа
        Fvector nearest_bone_pos;
        const CEntityAlive* corpse = this->object->CorpseMan.get_corpse();
        if (corpse->m_pPhysicsShell == nullptr || !corpse->m_pPhysicsShell->isActive())
        {
            nearest_bone_pos = corpse->Position();
        }
        else
            nearest_bone_pos = this->object->character_physics_support()->movement()->PHCaptureGetNearestElemPos(corpse);

#ifdef _DEBUG // XXX: Not sure, but maybe this should be under DEBUG, not _DEBUG
        DBG().level_info(this).clear();
        Fvector pos1;
        pos1.set(nearest_bone_pos);
        pos1.y += 20.f;

        DBG().level_info(this).add_item(nearest_bone_pos, pos1, COLOR_GREEN);
#endif
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
        data.time_out = 1500;
        data.sound_type = MonsterSound::eMonsterSoundEat;
        data.sound_delay = this->object->db().m_dwEatSndDelay;

        state->fill_data_with(&data, sizeof(SStateDataAction));

        return;
    }

    if (this->current_substate == eStateEat_WalkAway)
    {
        SStateHideFromPoint data;

        data.point = this->object->CorpseMan.get_corpse_position();
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
        data.action = ACT_REST;
        data.spec_params = 0;
        data.time_out = 8500;
        data.sound_type = MonsterSound::eMonsterSoundIdle;
        data.sound_delay = this->object->db().m_dwIdleSndDelay;

        state->fill_data_with(&data, sizeof(SStateDataAction));
        return;
    }

    if (this->current_substate == eStateEat_CorpseApproachWalk)
    {
        // Определить позицию ближайшей боны у трупа
        Fvector nearest_bone_pos;
        const CEntityAlive* corpse = this->object->CorpseMan.get_corpse();
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
bool CStateMonsterEatAbstract::check_completion()
{
    if (corpse != this->object->CorpseMan.get_corpse())
        return true;
    if (!hungry())
        return true;

    return false;
}

TEMPLATE_SPECIALIZATION
bool CStateMonsterEatAbstract::check_start_conditions()
{
    return (this->object->CorpseMan.get_corpse() && this->object->Home->at_home(this->object->CorpseMan.get_corpse()->Position()) &&
        hungry() && !monster_squad().get_squad(this->object)->is_locked_corpse(this->object->CorpseMan.get_corpse()));
}

TEMPLATE_SPECIALIZATION
bool CStateMonsterEatAbstract::hungry()
{
    return ((m_time_last_eat == 0) || (m_time_last_eat + TIME_NOT_HUNGRY < time()));
}

TEMPLATE_SPECIALIZATION
void CStateMonsterEatAbstract::remove_links(IGameObject* object)
{
    if (corpse == object)
        corpse = nullptr;
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateMonsterEatAbstract
