#pragma once

#include "../../../ai_space.h"
#include "../monster_cover_manager.h"
#include "../../../cover_point.h"
#include "../../../level.h"
#include "../../../level_debug.h"


#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>
#define CStateControllerHideAbstract CStateControlHide<_Object>

TEMPLATE_SPECIALIZATION
void CStateControllerHideAbstract::initialize()
{
	inherited::initialize();
	
	m_cover_reached		= false;
	select_target_point();
	object->path().prepare_builder();
	
}

TEMPLATE_SPECIALIZATION
void CStateControllerHideAbstract::execute()
{
	if (m_state_fast_run) {
		if (target.position.distance_to(object->Position()) < 5.f) {
			m_state_fast_run			= false;
			object->set_mental_state	(CController::eStateDanger);
		}
	}
	
	object->set_action					(ACT_RUN);
	
	object->path().set_target_point		(target.position, target.node);
	object->path().set_rebuild_time		(0);
	object->path().set_distance_to_end	(0.f);
	object->path().set_use_covers		(false);

	object->anim().accel_activate		(eAT_Aggressive);
	object->anim().accel_set_braking	(false);
	
	object->sound().play				(MonsterSound::eMonsterSoundAggressive, 0,0,object->db().m_dwAttackSndDelay);
	
	if (object->HitMemory.get_last_hit_time() > object->EnemyMan.get_enemy_time_last_seen()) {
		Fvector pos;
		pos.mad(object->Position(),object->HitMemory.get_last_hit_dir(),5.f);
		pos.y += 1.5f;
		object->custom_dir().head_look_point(pos);
	} else 
		object->custom_dir().head_look_point(object->EnemyMan.get_enemy_position());

	object->custom_anim().set_body_state(CControllerAnimation::eTorsoRun,CControllerAnimation::eLegsTypeRun);
}

TEMPLATE_SPECIALIZATION
bool CStateControllerHideAbstract::check_start_conditions()
{
	return true;
}

TEMPLATE_SPECIALIZATION
void CStateControllerHideAbstract::finalize()
{
	inherited::finalize();
	object->set_mental_state	(CController::eStateDanger);
}

TEMPLATE_SPECIALIZATION
void CStateControllerHideAbstract::critical_finalize()
{
	inherited::finalize();
	object->set_mental_state	(CController::eStateDanger);
}

TEMPLATE_SPECIALIZATION
bool CStateControllerHideAbstract::check_completion()
{
	return ((object->ai_location().level_vertex_id() == target.node) && !object->control().path_builder().is_moving_on_path());
}

TEMPLATE_SPECIALIZATION
void CStateControllerHideAbstract::select_target_point()
{
#ifdef DEBUG
	DBG().level_info(this).clear();
#endif

	const CCoverPoint	*point = object->CoverMan->find_cover(object->EnemyMan.get_enemy_position(),10.f,30.f);
	if (point) {
		target.node					= point->level_vertex_id	();
		target.position				= point->position			();
	} else {
		target.node					= 0;
		target.position				= ai().level_graph().vertex_position(target.node);			
	}

	m_state_fast_run = (target.position.distance_to(object->Position()) > 20.f);
	if (m_state_fast_run && (Random.randI(100) < 50)) 
		object->set_mental_state	(CController::eStateIdle);
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateControllerHideAbstract
