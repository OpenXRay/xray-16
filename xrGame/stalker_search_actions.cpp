////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_search_actions.cpp
//	Created 	: 25.03.2004
//  Modified 	: 08.10.2007
//	Author		: Dmitriy Iassenev
//	Description : stalker search enemy action classes
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "stalker_search_actions.h"
#include "ai/stalker/ai_stalker.h"
#include "script_game_object.h"
#include "stalker_movement_manager_smart_cover.h"
#include "movement_manager_space.h"
#include "agent_manager.h"
#include "agent_member_manager.h"
#include "memory_manager.h"
#include "enemy_manager.h"
#include "hit_memory_manager.h"
#include "sight_manager.h"
#include "cover_evaluators.h"
#include "cover_manager.h"
#include "stalker_movement_restriction.h"
#include "cover_point.h"
#include "detail_path_manager_space.h"

using namespace StalkerSpace;
using namespace StalkerDecisionSpace;

//////////////////////////////////////////////////////////////////////////
// CStalkerActionReachEnemyLocation
//////////////////////////////////////////////////////////////////////////

CStalkerActionReachEnemyLocation::CStalkerActionReachEnemyLocation(
		CAI_Stalker *object,
		CPropertyStorage *combat_storage,
		LPCSTR action_name
	) :
	inherited			(object,action_name),
	m_combat_storage	(combat_storage)
{
}

void CStalkerActionReachEnemyLocation::initialize		()
{
	inherited::initialize				();
	object().movement().set_desired_direction		(0);
	object().movement().set_path_type				(MovementManager::ePathTypeLevelPath);
	object().movement().set_detail_path_type		(DetailPathManager::eDetailPathTypeSmooth);
	object().movement().set_mental_state			(eMentalStateDanger);
	object().movement().set_body_state				(eBodyStateStand);
	object().movement().set_movement_type			(eMovementTypeWalk);

	aim_ready										();

	object().agent_manager().member().member(m_object).cover(0);

	const MemorySpace::CHitObject		*hit = object().memory().hit().hit(object().memory().enemy().selected());
	if (!hit)
		m_last_hit_time					= 0;
	else
		m_last_hit_time					= hit->m_level_time;

//	object().sniper_update_rate			(true);
}

void CStalkerActionReachEnemyLocation::finalize		()
{
	inherited::finalize					();

//	object().sniper_update_rate			(false);
}

void CStalkerActionReachEnemyLocation::execute			()
{
#ifdef TEST_MENTAL_STATE
	VERIFY								((start_level_time() == Device.dwTimeGlobal) || (object().movement().mental_state() == eMentalStateDanger));
#endif // TEST_MENTAL_STATE

	inherited::execute					();

	MemorySpace::CMemoryInfo			mem_object = object().memory().memory(object().memory().enemy().selected());

	if (!mem_object.m_object)
		return;

	const MemorySpace::CHitObject		*hit = object().memory().hit().hit(object().memory().enemy().selected());
	if (hit && hit->m_level_time > m_last_hit_time) {
		m_combat_storage->set_property	(eWorldPropertyLookedOut,		false);
		m_combat_storage->set_property	(eWorldPropertyPositionHolded,	false);
		m_combat_storage->set_property	(eWorldPropertyEnemyDetoured,	false);
		return;
	}

	if (object().movement().path_completed()) {
#if 0
		object().m_ce_ambush->setup		(mem_object.m_object_params.m_position,mem_object.m_self_params.m_position,10.f);
		const CCoverPoint				*point = ai().cover_manager().best_cover(mem_object.m_object_params.m_position,10.f,*object().m_ce_ambush,CStalkerMovementRestrictor(m_object,true));
		if (!point) {
			object().m_ce_ambush->setup	(mem_object.m_object_params.m_position,mem_object.m_self_params.m_position,10.f);
			point						= ai().cover_manager().best_cover(mem_object.m_object_params.m_position,30.f,*object().m_ce_ambush,CStalkerMovementRestrictor(m_object,true));
		}

		if (point) {
			object().movement().set_level_dest_vertex	(point->level_vertex_id());
			object().movement().set_desired_position	(&point->position());
		}
		else
			object().movement().set_nearest_accessible_position	();
#else
		if (object().movement().accessible(mem_object.m_object_params.m_level_vertex_id)) {
			object().movement().set_level_dest_vertex	(mem_object.m_object_params.m_level_vertex_id);
//			object().movement().set_desired_position	(0);
		}
		else {
			object().movement().set_nearest_accessible_position	(
				mem_object.m_object_params.m_position,
				mem_object.m_object_params.m_level_vertex_id
			);
		}

		object().sight().setup		(
			CSightAction(
				SightManager::eSightTypePosition,
				Fvector(mem_object.m_object_params.m_position).add(Fvector().set(0.f, .5f, 0.f)),
//				mem_object.m_object_params.m_position,
				true
			)
		);
#endif

		if (object().movement().path_completed()) {
			m_storage->set_property		(eWorldPropertyEnemyLocationReached, true);

#ifndef SILENT_COMBAT
			play_start_search_sound		(0,0,10000,10000);
#endif // SILENT_COMBAT
		}
	}
	else {
		object().sight().setup		(
			CSightAction(
				SightManager::eSightTypePosition,
				Fvector(mem_object.m_object_params.m_position).add(Fvector().set(0.f, .5f, 0.f)),
//				mem_object.m_object_params.m_position,
				true
			)
		);
	}
}

//////////////////////////////////////////////////////////////////////////
// CStalkerActionReachAmbushLocation
//////////////////////////////////////////////////////////////////////////

CStalkerActionReachAmbushLocation::CStalkerActionReachAmbushLocation(CAI_Stalker *object, CPropertyStorage *combat_storage, LPCSTR action_name) :
	inherited			(object,action_name),
	m_combat_storage	(combat_storage)
{
}

void CStalkerActionReachAmbushLocation::initialize					()
{
	inherited::initialize				();

	const MemorySpace::CHitObject		*hit = object().memory().hit().hit(object().memory().enemy().selected());
	if (!hit)
		m_last_hit_time					= 0;
	else
		m_last_hit_time					= hit->m_level_time;
}

void CStalkerActionReachAmbushLocation::finalize					()
{
	inherited::finalize					();
}

void CStalkerActionReachAmbushLocation::execute						()
{
	inherited::execute					();

	MemorySpace::CMemoryInfo			mem_object = object().memory().memory(object().memory().enemy().selected());

	if (!mem_object.m_object)
		return;

	const MemorySpace::CHitObject		*hit = object().memory().hit().hit(object().memory().enemy().selected());
	if (hit && hit->m_level_time > m_last_hit_time) {
		m_combat_storage->set_property	(eWorldPropertyLookedOut,		false);
		m_combat_storage->set_property	(eWorldPropertyPositionHolded,	false);
		m_combat_storage->set_property	(eWorldPropertyEnemyDetoured,	false);
		return;
	}

	object().m_ce_ambush->setup			(mem_object.m_object_params.m_position,mem_object.m_self_params.m_position,10.f);
	const CCoverPoint					*point = ai().cover_manager().best_cover(mem_object.m_object_params.m_position,10.f,*object().m_ce_ambush,CStalkerMovementRestrictor(m_object,true));
	if (!point) {
		object().m_ce_ambush->setup		(mem_object.m_object_params.m_position,mem_object.m_self_params.m_position,10.f);
		point							= ai().cover_manager().best_cover(mem_object.m_object_params.m_position,30.f,*object().m_ce_ambush,CStalkerMovementRestrictor(m_object,true));
	}

	if (point) {
		object().movement().set_level_dest_vertex	(point->level_vertex_id());
		object().movement().set_desired_position	(&point->position());
	}
	else
		object().movement().set_nearest_accessible_position	();

	if (!object().movement().path_completed()) {
#ifndef SILENT_COMBAT
		play_enemy_lost_sound			(0,0,10000,10000);
#endif // SILENT_COMBAT
		return;
	}

	m_storage->set_property				(eWorldPropertyAmbushLocationReached, true);
}

//////////////////////////////////////////////////////////////////////////
// CStalkerActionHoldAmbushLocation
//////////////////////////////////////////////////////////////////////////

CStalkerActionHoldAmbushLocation::CStalkerActionHoldAmbushLocation	(CAI_Stalker *object, CPropertyStorage *combat_storage, LPCSTR action_name) :
	inherited			(object,action_name),
	m_combat_storage	(combat_storage)
{
}

void CStalkerActionHoldAmbushLocation::initialize					()
{
	inherited::initialize				();

	const MemorySpace::CHitObject		*hit = object().memory().hit().hit(object().memory().enemy().selected());
	if (!hit)
		m_last_hit_time					= 0;
	else
		m_last_hit_time					= hit->m_level_time;

	object().movement().set_body_state	(eBodyStateCrouch);
	object().sight().setup				(CSightAction(SightManager::eSightTypeCoverLookOver,true));
}

void CStalkerActionHoldAmbushLocation::finalize						()
{
	inherited::finalize					();
}

void CStalkerActionHoldAmbushLocation::execute						()
{
	inherited::execute					();


	MemorySpace::CMemoryInfo			mem_object = object().memory().memory(object().memory().enemy().selected());

	if (!mem_object.m_object)
		return;

	const MemorySpace::CHitObject		*hit = object().memory().hit().hit(object().memory().enemy().selected());
	if (hit && hit->m_level_time > m_last_hit_time) {
		m_combat_storage->set_property	(eWorldPropertyLookedOut,		false);
		m_combat_storage->set_property	(eWorldPropertyPositionHolded,	false);
		m_combat_storage->set_property	(eWorldPropertyEnemyDetoured,	false);
		return;
	}

	if (!completed())
		return;

	if (mem_object.m_last_level_time + 60000 < Device.dwTimeGlobal)
		return;

	object().memory().enable			( object().memory().enemy().selected(), false);
}
