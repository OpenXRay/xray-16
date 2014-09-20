////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_get_distance_actions.cpp
//	Created 	: 25.07.2007
//  Modified 	: 25.07.2007
//	Author		: Dmitriy Iassenev
//	Description : Stalker get distance action classes
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "stalker_get_distance_actions.h"
#include "ai/stalker/ai_stalker.h"
#include "stalker_movement_manager_smart_cover.h"
#include "sight_manager.h"
#include "memory_space.h"
#include "memory_manager.h"
#include "enemy_manager.h"
#include "cover_evaluators.h"
#include "cover_manager.h"
#include "stalker_movement_restriction.h"
#include "cover_point.h"
#include "stalker_decision_space.h"
#include "script_game_object.h"
#include "visual_memory_manager.h"

//////////////////////////////////////////////////////////////////////////
// CStalkerActionRunToCover
//////////////////////////////////////////////////////////////////////////

CStalkerActionRunToCover::CStalkerActionRunToCover	(CAI_Stalker *object, LPCSTR action_name) :
	inherited				(object,action_name)
{
}

void CStalkerActionRunToCover::initialize			()
{
	inherited::initialize							();

	object().movement().set_mental_state			(eMentalStateDanger);
	object().movement().set_movement_type			(eMovementTypeRun);
	object().movement().set_body_state				(eBodyStateStand);

	object().set_goal								(eObjectActionIdle,object().best_weapon());
	object().sight().setup							(CSightAction(SightManager::eSightTypePathDirection));
	
	MemorySpace::CMemoryInfo						mem_object = object().memory().memory(object().memory().enemy().selected());
	Fvector											position = mem_object.m_object_params.m_position;
	object().m_ce_close->setup						(position,0.f,object().Position().distance_to(position),10.f);
	const CCoverPoint								*point = ai().cover_manager().best_cover(object().Position(),10.f,*object().m_ce_close,CStalkerMovementRestrictor(m_object,true));
	if (!point) {
		object().m_ce_close->setup					(position,0.f,object().Position().distance_to(position),10.f);
		point										= ai().cover_manager().best_cover(object().Position(),30.f,*object().m_ce_close,CStalkerMovementRestrictor(m_object,true));
	}

	if (point) {
		object().movement().set_level_dest_vertex	(point->level_vertex_id());
		object().movement().set_desired_position	(&point->position());
		return;
	}

	if (object().movement().restrictions().accessible(mem_object.m_object_params.m_level_vertex_id)) {
		object().movement().set_level_dest_vertex	(mem_object.m_object_params.m_level_vertex_id);
		object().movement().set_desired_position	(&mem_object.m_object_params.m_position);
		return;
	}

	object().movement().set_nearest_accessible_position	(
		mem_object.m_object_params.m_position,
		mem_object.m_object_params.m_level_vertex_id
	);
}

void CStalkerActionRunToCover::execute				()
{
	inherited::execute								();

	if (!object().memory().visual().visible_now(object().memory().enemy().selected())) {
		object().set_goal							(eObjectActionIdle,object().best_weapon());
		object().sight().setup						(CSightAction(SightManager::eSightTypePathDirection));
	}
	else {
		object().sight().setup						(CSightAction(object().memory().enemy().selected(),true,true,false));
		fire										();
	}

	if (!object().movement().path_completed())
		return;

	m_storage->set_property							(StalkerDecisionSpace::eWorldPropertyInCover,	true);
}

void CStalkerActionRunToCover::finalize				()
{
	inherited::finalize		();
}

//////////////////////////////////////////////////////////////////////////
// CStalkerActionWaitInCover
//////////////////////////////////////////////////////////////////////////

CStalkerActionWaitInCover::CStalkerActionWaitInCover(CAI_Stalker *object, LPCSTR action_name) :
	inherited				(object,action_name)
{
}

void CStalkerActionWaitInCover::initialize			()
{
	inherited::initialize							();

	object().movement().set_mental_state			(eMentalStateDanger);
	object().movement().set_movement_type			(eMovementTypeStand);
	
	if (object().movement().body_state() == eBodyStateStand)
		object().movement().set_body_state			(::Random.randI(2) ? eBodyStateCrouch : eBodyStateStand);
	else
		object().movement().set_body_state			(eBodyStateCrouch);

	object().set_goal								(eObjectActionIdle,object().best_weapon());
	object().sight().clear							();
	object().sight().setup							(CSightAction(SightManager::eSightTypePathDirection,false));

	set_inertia_time								(::Random.randI(1000,3000));
}

void CStalkerActionWaitInCover::execute				()
{
	inherited::execute								();

	if (!completed() && !object().memory().visual().visible_now(object().memory().enemy().selected()))
		return;

	m_storage->set_property							(StalkerDecisionSpace::eWorldPropertyInCover,	false);
}

void CStalkerActionWaitInCover::finalize			()
{
	inherited::finalize								();
	m_storage->set_property							(StalkerDecisionSpace::eWorldPropertyInCover,false);
	object().best_cover_invalidate					();
}
