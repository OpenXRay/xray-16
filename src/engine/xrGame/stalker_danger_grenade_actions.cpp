////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_danger_grenade_actions.cpp
//	Created 	: 31.05.2005
//  Modified 	: 31.05.2005
//	Author		: Dmitriy Iassenev
//	Description : Stalker danger grenade actions classes
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "stalker_danger_grenade_actions.h"
#include "ai/stalker/ai_stalker.h"
#include "script_game_object.h"
#include "stalker_movement_manager_smart_cover.h"
#include "sight_manager.h"
#include "object_handler.h"
#include "movement_manager_space.h"
#include "detail_path_manager_space.h"
#include "stalker_decision_space.h"
#include "memory_manager.h"
#include "danger_manager.h"
#include "cover_point.h"
#include "agent_manager.h"
#include "agent_member_manager.h"
#include "inventory.h"
#include "weapon.h"

using namespace StalkerDecisionSpace;

//////////////////////////////////////////////////////////////////////////
// CStalkerActionDangerGrenadeTakeCover
//////////////////////////////////////////////////////////////////////////

CStalkerActionDangerGrenadeTakeCover::CStalkerActionDangerGrenadeTakeCover	(CAI_Stalker *object, LPCSTR action_name) :
	inherited				(object,action_name)
{
}

void CStalkerActionDangerGrenadeTakeCover::initialize						()
{
	inherited::initialize	();

	set_property			(eWorldPropertyCoverReached,false);
	set_property			(eWorldPropertyLookedAround,false);

	object().movement().set_desired_direction	(0);
	object().movement().set_path_type			(MovementManager::ePathTypeLevelPath);
	object().movement().set_detail_path_type	(DetailPathManager::eDetailPathTypeSmooth);
	object().movement().set_movement_type		(eMovementTypeRun);
}

void CStalkerActionDangerGrenadeTakeCover::execute							()
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

	EMentalState				temp;
	{
		if (!object().inventory().ActiveItem()) {
			object().CObjectHandler::set_goal			(eObjectActionIdle);
			temp										= eMentalStatePanic;
		}
		else {
			CWeapon					*weapon = smart_cast<CWeapon*>(&object().inventory().ActiveItem()->object());
			if (weapon && weapon->can_be_strapped() && object().best_weapon() && (object().best_weapon()->object().ID() == weapon->ID())) {
				object().CObjectHandler::set_goal			(eObjectActionStrapped,object().inventory().ActiveItem());
				if (weapon->strapped_mode())
					temp									= eMentalStatePanic;
				else
					temp									= eMentalStateDanger;
			}
			else {
				object().CObjectHandler::set_goal			(eObjectActionIdle);
				temp										= eMentalStateDanger;
			}
		}
	}

	if (!object().movement().path_completed()) {
		object().movement().set_body_state			(eBodyStateStand);
		if (object().movement().distance_to_destination_greater(2.f)) {
			object().movement().set_mental_state	(temp);
			object().sight().setup					(CSightAction(SightManager::eSightTypePathDirection,true,true));
		}
		else {
			object().movement().set_mental_state	(eMentalStateDanger);
			object().sight().setup					(CSightAction(SightManager::eSightTypeCover,true,true));
		}
		return;
	}

	set_property								(eWorldPropertyCoverReached,true);
}

void CStalkerActionDangerGrenadeTakeCover::finalize							()
{
	inherited::finalize		();
}

//////////////////////////////////////////////////////////////////////////
// CStalkerActionDangerGrenadeWaitForExplosion
//////////////////////////////////////////////////////////////////////////

CStalkerActionDangerGrenadeWaitForExplosion::CStalkerActionDangerGrenadeWaitForExplosion	(CAI_Stalker *object, LPCSTR action_name) :
	inherited				(object,action_name)
{
}

void CStalkerActionDangerGrenadeWaitForExplosion::initialize							()
{
	inherited::initialize						();

	object().movement().set_desired_direction	(0);
	object().movement().set_path_type			(MovementManager::ePathTypeLevelPath);
	object().movement().set_detail_path_type	(DetailPathManager::eDetailPathTypeSmooth);
	object().movement().set_movement_type		(eMovementTypeStand);
	object().movement().set_mental_state		(eMentalStateDanger);
	object().movement().set_body_state			(eBodyStateCrouch);
	object().CObjectHandler::set_goal			(eObjectActionIdle,object().best_weapon());
}

void CStalkerActionDangerGrenadeWaitForExplosion::execute							()
{
	inherited::execute		();
	if (!object().memory().danger().selected())
		return;

	if (fsimilar(object().movement().body_orientation().target.yaw,object().movement().body_orientation().current.yaw))
		object().sight().setup					(CSightAction(SightManager::eSightTypeCoverLookOver,true));
	else
		object().sight().setup					(CSightAction(SightManager::eSightTypeCover,true));
}

void CStalkerActionDangerGrenadeWaitForExplosion::finalize							()
{
	inherited::finalize		();
}

//////////////////////////////////////////////////////////////////////////
// CStalkerActionDangerGrenadeTakeCoverAfterExplosion
//////////////////////////////////////////////////////////////////////////

CStalkerActionDangerGrenadeTakeCoverAfterExplosion::CStalkerActionDangerGrenadeTakeCoverAfterExplosion	(CAI_Stalker *object, LPCSTR action_name) :
	inherited				(object,action_name)
{
}

void CStalkerActionDangerGrenadeTakeCoverAfterExplosion::initialize						()
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

void CStalkerActionDangerGrenadeTakeCoverAfterExplosion::execute							()
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

void CStalkerActionDangerGrenadeTakeCoverAfterExplosion::finalize							()
{
	inherited::finalize		();
}

//////////////////////////////////////////////////////////////////////////
// CStalkerActionDangerGrenadeLookAround
//////////////////////////////////////////////////////////////////////////

CStalkerActionDangerGrenadeLookAround::CStalkerActionDangerGrenadeLookAround	(CAI_Stalker *object, LPCSTR action_name) :
	inherited				(object,action_name)
{
}

void CStalkerActionDangerGrenadeLookAround::initialize						()
{
	set_inertia_time							(15000);

	inherited::initialize						();

	object().movement().set_desired_direction	(0);
	object().movement().set_path_type			(MovementManager::ePathTypeLevelPath);
	object().movement().set_detail_path_type	(DetailPathManager::eDetailPathTypeSmooth);
	object().movement().set_movement_type		(eMovementTypeStand);
	object().movement().set_mental_state		(eMentalStateDanger);
	object().movement().set_body_state			(eBodyStateCrouch);
	object().CObjectHandler::set_goal			(eObjectActionAimReady1,object().best_weapon());
}

void CStalkerActionDangerGrenadeLookAround::execute							()
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

void CStalkerActionDangerGrenadeLookAround::finalize							()
{
	inherited::finalize		();
}

//////////////////////////////////////////////////////////////////////////
// CStalkerActionDangerGrenadeSearch
//////////////////////////////////////////////////////////////////////////

CStalkerActionDangerGrenadeSearch::CStalkerActionDangerGrenadeSearch	(CAI_Stalker *object, LPCSTR action_name) :
	inherited				(object,action_name)
{
}

void CStalkerActionDangerGrenadeSearch::initialize						()
{
	inherited::initialize	();
	object().movement().set_desired_direction	(0);
	object().movement().set_path_type			(MovementManager::ePathTypeLevelPath);
	object().movement().set_detail_path_type	(DetailPathManager::eDetailPathTypeSmooth);
	object().movement().set_nearest_accessible_position();
	object().movement().set_body_state			(eBodyStateStand);
	object().movement().set_movement_type		(eMovementTypeStand);
	object().movement().set_mental_state		(eMentalStateDanger);
	object().sight().setup						(SightManager::eSightTypeCurrentDirection);
	object().CObjectHandler::set_goal			(eObjectActionIdle);
}

void CStalkerActionDangerGrenadeSearch::execute							()
{
	inherited::execute		();
}

void CStalkerActionDangerGrenadeSearch::finalize							()
{
	inherited::finalize		();
}
