////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_danger_unknown_actions.cpp
//	Created 	: 31.05.2005
//  Modified 	: 31.05.2005
//	Author		: Dmitriy Iassenev
//	Description : Stalker danger unknown actions classes
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "stalker_danger_unknown_actions.h"
#include "ai/stalker/ai_stalker.h"
#include "script_game_object.h"
#include "stalker_movement_manager_smart_cover.h"
#include "sight_manager.h"
#include "object_handler.h"
#include "movement_manager_space.h"
#include "detail_path_manager.h"
#include "stalker_decision_space.h"
#include "memory_manager.h"
#include "danger_manager.h"
#include "agent_manager.h"
#include "agent_member_manager.h"
#include "agent_location_manager.h"
#include "cover_point.h"
#include "danger_cover_location.h"

using namespace StalkerDecisionSpace;

const float DANGER_DISTANCE = 5.f;
const u32	DANGER_INTERVAL = 120000;

//////////////////////////////////////////////////////////////////////////
// CStalkerActionDangerUnknownTakeCover
//////////////////////////////////////////////////////////////////////////

CStalkerActionDangerUnknownTakeCover::CStalkerActionDangerUnknownTakeCover	(CAI_Stalker *object, LPCSTR action_name) :
	inherited				(object,action_name)
{
}

void CStalkerActionDangerUnknownTakeCover::initialize						()
{
	inherited::initialize	();
	
	set_property			(eWorldPropertyCoverReached,false);
	set_property			(eWorldPropertyLookedAround,false);

	object().movement().set_desired_direction	(0);
	object().movement().set_path_type			(MovementManager::ePathTypeLevelPath);
	object().movement().set_detail_path_type	(DetailPathManager::eDetailPathTypeSmooth);
	object().movement().set_mental_state		(eMentalStateDanger);

	m_direction_sight		= !!::Random.randI(2);
}

void CStalkerActionDangerUnknownTakeCover::execute							()
{
	inherited::execute		();

	if (!object().memory().danger().selected())
		return;

	const CCoverPoint		*point = object().agent_manager().member().member(&object()).cover();
	if (point) {
		object().movement().set_level_dest_vertex	(point->level_vertex_id());
		object().movement().set_desired_position	(&point->position());
	}	
	else
		object().movement().set_nearest_accessible_position	();

	object().CObjectHandler::set_goal	(eObjectActionAimReady1,object().best_weapon());

	if (!object().movement().path_completed()) {
		object().movement().set_body_state		(eBodyStateStand);
		object().movement().set_movement_type	(eMovementTypeRun);
		if (!m_direction_sight || !object().movement().distance_to_destination_greater(2.f))
			object().sight().setup				(CSightAction(SightManager::eSightTypeCover,true,true));
		else
			object().sight().setup				(CSightAction(SightManager::eSightTypePathDirection,true,true));
		return;
	}

	set_property								(eWorldPropertyCoverReached,true);
}

void CStalkerActionDangerUnknownTakeCover::finalize							()
{
	inherited::finalize		();
}

//////////////////////////////////////////////////////////////////////////
// CStalkerActionDangerUnknownLookAround
//////////////////////////////////////////////////////////////////////////

CStalkerActionDangerUnknownLookAround::CStalkerActionDangerUnknownLookAround	(CAI_Stalker *object, LPCSTR action_name) :
	inherited				(object,action_name)
{
}

void CStalkerActionDangerUnknownLookAround::initialize						()
{
	set_inertia_time							(15000);

	inherited::initialize						();

	object().movement().set_desired_direction	(0);
	object().movement().set_path_type			(MovementManager::ePathTypeLevelPath);
	object().movement().set_detail_path_type	(DetailPathManager::eDetailPathTypeSmooth);
	object().movement().set_movement_type		(eMovementTypeStand);
	object().movement().set_mental_state		(eMentalStateDanger);
	object().movement().set_body_state			(eBodyStateCrouch);
}

void CStalkerActionDangerUnknownLookAround::execute							()
{
	inherited::execute		();

	if (!object().memory().danger().selected())
		return;

	if (fsimilar(object().movement().body_orientation().target.yaw,object().movement().body_orientation().current.yaw))
		object().sight().setup					(CSightAction(SightManager::eSightTypeCoverLookOver,true));
	else
		object().sight().setup					(CSightAction(SightManager::eSightTypeCover,true));

	if (completed())
		set_property							(eWorldPropertyLookedAround,true);
}

void CStalkerActionDangerUnknownLookAround::finalize							()
{
	inherited::finalize		();
}

//////////////////////////////////////////////////////////////////////////
// CStalkerActionDangerUnknownSearch
//////////////////////////////////////////////////////////////////////////

CStalkerActionDangerUnknownSearch::CStalkerActionDangerUnknownSearch	(CAI_Stalker *object, LPCSTR action_name) :
	inherited				(object,action_name)
{
}

void CStalkerActionDangerUnknownSearch::initialize						()
{
	inherited::initialize	();
}

void CStalkerActionDangerUnknownSearch::execute							()
{
	inherited::execute		();

	if (object().agent_manager().member().member(&object()).cover()) {
		object().agent_manager().location().add		(
			xr_new<CDangerCoverLocation>(
				object().agent_manager().member().member(&object()).cover(),
				Device.dwTimeGlobal,
				DANGER_INTERVAL,
				DANGER_DISTANCE
			)
		);
		return;
	}

	set_property			(eWorldPropertyCoverReached,false);
	set_property			(eWorldPropertyLookedAround,false);
}

void CStalkerActionDangerUnknownSearch::finalize							()
{
	inherited::finalize		();
}
