#pragma once

#include "ai_space.h"
#include "ai/monsters/monster_cover_manager.h"
#include "cover_point.h"
#include "Level.h"
#include "level_debug.h"

#define TEMPLATE_SPECIALIZATION \
    template <typename _Object\
>
#define CStateControllerHideAbstract CStateControlHide<_Object>

TEMPLATE_SPECIALIZATION
void CStateControllerHideAbstract::initialize()
{
    inherited::initialize();

    m_cover_reached = false;
    select_target_point();
    this->object->path().prepare_builder();
}

TEMPLATE_SPECIALIZATION
void CStateControllerHideAbstract::execute()
{
    if (m_state_fast_run)
    {
        if (target.position.distance_to(this->object->Position()) < 5.f)
        {
            m_state_fast_run = false;
            this->object->set_mental_state(CController::eStateDanger);
        }
    }

    this->object->set_action(ACT_RUN);

    this->object->path().set_target_point(target.position, target.node);
    this->object->path().set_rebuild_time(0);
    this->object->path().set_distance_to_end(0.f);
    this->object->path().set_use_covers(false);

    this->object->anim().accel_activate(eAT_Aggressive);
    this->object->anim().accel_set_braking(false);

    this->object->sound().play(MonsterSound::eMonsterSoundAggressive, 0, 0, this->object->db().m_dwAttackSndDelay);

    if (this->object->HitMemory.get_last_hit_time() >this->object->EnemyMan.get_enemy_time_last_seen())
    {
        Fvector pos;
        pos.mad(this->object->Position(), this->object->HitMemory.get_last_hit_dir(), 5.f);
        pos.y += 1.5f;
        this->object->custom_dir().head_look_point(pos);
    }
    else
        this->object->custom_dir().head_look_point(this->object->EnemyMan.get_enemy_position());

    this->object->custom_anim().set_body_state(CControllerAnimation::eTorsoRun, CControllerAnimation::eLegsTypeRun);
}

TEMPLATE_SPECIALIZATION
bool CStateControllerHideAbstract::check_start_conditions() { return true; }
TEMPLATE_SPECIALIZATION
void CStateControllerHideAbstract::finalize()
{
    inherited::finalize();
    this->object->set_mental_state(CController::eStateDanger);
}

TEMPLATE_SPECIALIZATION
void CStateControllerHideAbstract::critical_finalize()
{
    inherited::finalize();
    this->object->set_mental_state(CController::eStateDanger);
}

TEMPLATE_SPECIALIZATION
bool CStateControllerHideAbstract::check_completion()
{
    return ((this->object->ai_location().level_vertex_id() == target.node) &&
        !this->object->control().path_builder().is_moving_on_path());
}

TEMPLATE_SPECIALIZATION
void CStateControllerHideAbstract::select_target_point()
{
#ifdef DEBUG
    DBG().level_info(this).clear();
#endif

    const CCoverPoint* point = this->object->CoverMan->find_cover(this->object->EnemyMan.get_enemy_position(), 10.f, 30.f);
    if (point)
    {
        target.node = point->level_vertex_id();
        target.position = point->position();
    }
    else
    {
        target.node = 0;
        target.position = ai().level_graph().vertex_position(target.node);
    }

    m_state_fast_run = (target.position.distance_to(this->object->Position()) > 20.f);
    if (m_state_fast_run && (Random.randI(100) < 50))
        this->object->set_mental_state(CController::eStateIdle);
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateControllerHideAbstract
