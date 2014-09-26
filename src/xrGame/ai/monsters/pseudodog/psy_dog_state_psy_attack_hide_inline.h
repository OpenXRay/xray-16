#pragma once

#include "../../../ai_space.h"
#include "../monster_cover_manager.h"
#include "../../../cover_point.h"

#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>
#define CStatePsyDogHideAbstract CStatePsyDogHide<_Object>

TEMPLATE_SPECIALIZATION
void CStatePsyDogHideAbstract::initialize()
{
	inherited::initialize();

	select_target_point();
	object->path().prepare_builder();

}

TEMPLATE_SPECIALIZATION
void CStatePsyDogHideAbstract::execute()
{
	object->set_action					(ACT_RUN);
	object->path().set_target_point		(target.position, target.node);
	object->path().set_rebuild_time		(0);
	object->path().set_distance_to_end	(0);
	object->path().set_use_covers		(false);

	object->anim().accel_activate		(eAT_Aggressive);
	object->anim().accel_set_braking	(false);

	object->sound().play				(MonsterSound::eMonsterSoundAggressive, 0,0,object->db().m_dwAttackSndDelay);
}

TEMPLATE_SPECIALIZATION
bool CStatePsyDogHideAbstract::check_start_conditions()
{
	return true;
}

TEMPLATE_SPECIALIZATION
bool CStatePsyDogHideAbstract::check_completion()
{
	return ((object->ai_location().level_vertex_id() == target.node) && !object->control().path_builder().is_moving_on_path());
}

TEMPLATE_SPECIALIZATION
void CStatePsyDogHideAbstract::select_target_point()
{
	const CCoverPoint	*point = object->CoverMan->find_cover(object->EnemyMan.get_enemy_position(),10.f,30.f);
	if (point && (object->Position().distance_to(point->position()) > 2.f)) {
		target.node					= point->level_vertex_id	();
		target.position				= point->position			();
	} else {
		const CCoverPoint	*point = object->CoverMan->find_cover(object->Position(),10.f,30.f);
		if (point && (object->Position().distance_to(point->position()) > 2.f)) {
			target.node					= point->level_vertex_id	();
			target.position				= point->position			();
		} else {
			target.node					= 0;
			target.position				= ai().level_graph().vertex_position(target.node);
		}
	}
}

#undef TEMPLATE_SPECIALIZATION
#undef CStatePsyDogHideAbstract
