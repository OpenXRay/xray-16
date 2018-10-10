////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_danger_by_sound_actions.cpp
//	Created 	: 31.05.2005
//  Modified 	: 31.05.2005
//	Author		: Dmitriy Iassenev
//	Description : Stalker danger by sound actions classes
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "stalker_danger_by_sound_actions.h"
#include "ai/stalker/ai_stalker.h"
#include "script_game_object.h"
#include "script_game_object_impl.h"
#include "stalker_movement_manager_smart_cover.h"
#include "sight_manager.h"
#include "object_handler.h"
#include "movement_manager_space.h"
#include "detail_path_manager_space.h"

//////////////////////////////////////////////////////////////////////////
// CStalkerActionDangerBySoundListenTo
//////////////////////////////////////////////////////////////////////////

CStalkerActionDangerBySoundListenTo::CStalkerActionDangerBySoundListenTo(CAI_Stalker* object, LPCSTR action_name)
    : inherited(object, action_name)
{
}

void CStalkerActionDangerBySoundListenTo::initialize()
{
    inherited::initialize();
    object().movement().set_desired_direction(0);
    object().movement().set_path_type(MovementManager::ePathTypeLevelPath);
    object().movement().set_detail_path_type(DetailPathManager::eDetailPathTypeSmooth);
    object().movement().set_nearest_accessible_position();
    object().movement().set_body_state(eBodyStateStand);
    object().movement().set_movement_type(eMovementTypeStand);
    object().movement().set_mental_state(eMentalStateDanger);
    object().sight().setup(SightManager::eSightTypeCurrentDirection);
    object().CObjectHandler::set_goal(eObjectActionIdle);
}

void CStalkerActionDangerBySoundListenTo::execute() { inherited::execute(); }
void CStalkerActionDangerBySoundListenTo::finalize() { inherited::finalize(); }
//////////////////////////////////////////////////////////////////////////
// CStalkerActionDangerBySoundCheck
//////////////////////////////////////////////////////////////////////////

CStalkerActionDangerBySoundCheck::CStalkerActionDangerBySoundCheck(CAI_Stalker* object, LPCSTR action_name)
    : inherited(object, action_name)
{
}

void CStalkerActionDangerBySoundCheck::initialize()
{
    inherited::initialize();
    object().movement().set_desired_direction(0);
    object().movement().set_path_type(MovementManager::ePathTypeLevelPath);
    object().movement().set_detail_path_type(DetailPathManager::eDetailPathTypeSmooth);
    object().movement().set_nearest_accessible_position();
    object().movement().set_body_state(eBodyStateStand);
    object().movement().set_movement_type(eMovementTypeStand);
    object().movement().set_mental_state(eMentalStateDanger);
    object().sight().setup(SightManager::eSightTypeCurrentDirection);
    object().CObjectHandler::set_goal(eObjectActionIdle);
}

void CStalkerActionDangerBySoundCheck::execute() { inherited::execute(); }
void CStalkerActionDangerBySoundCheck::finalize() { inherited::finalize(); }
//////////////////////////////////////////////////////////////////////////
// CStalkerActionDangerBySoundTakeCover
//////////////////////////////////////////////////////////////////////////

CStalkerActionDangerBySoundTakeCover::CStalkerActionDangerBySoundTakeCover(CAI_Stalker* object, LPCSTR action_name)
    : inherited(object, action_name)
{
}

void CStalkerActionDangerBySoundTakeCover::initialize()
{
    inherited::initialize();
    object().movement().set_desired_direction(0);
    object().movement().set_path_type(MovementManager::ePathTypeLevelPath);
    object().movement().set_detail_path_type(DetailPathManager::eDetailPathTypeSmooth);
    object().movement().set_nearest_accessible_position();
    object().movement().set_body_state(eBodyStateStand);
    object().movement().set_movement_type(eMovementTypeStand);
    object().movement().set_mental_state(eMentalStateDanger);
    object().sight().setup(SightManager::eSightTypeCurrentDirection);
    object().CObjectHandler::set_goal(eObjectActionIdle);
}

void CStalkerActionDangerBySoundTakeCover::execute() { inherited::execute(); }
void CStalkerActionDangerBySoundTakeCover::finalize() { inherited::finalize(); }
//////////////////////////////////////////////////////////////////////////
// CStalkerActionDangerBySoundLookOut
//////////////////////////////////////////////////////////////////////////

CStalkerActionDangerBySoundLookOut::CStalkerActionDangerBySoundLookOut(CAI_Stalker* object, LPCSTR action_name)
    : inherited(object, action_name)
{
}

void CStalkerActionDangerBySoundLookOut::initialize()
{
    inherited::initialize();
    object().movement().set_desired_direction(0);
    object().movement().set_path_type(MovementManager::ePathTypeLevelPath);
    object().movement().set_detail_path_type(DetailPathManager::eDetailPathTypeSmooth);
    object().movement().set_nearest_accessible_position();
    object().movement().set_body_state(eBodyStateStand);
    object().movement().set_movement_type(eMovementTypeStand);
    object().movement().set_mental_state(eMentalStateDanger);
    object().sight().setup(SightManager::eSightTypeCurrentDirection);
    object().CObjectHandler::set_goal(eObjectActionIdle);
}

void CStalkerActionDangerBySoundLookOut::execute() { inherited::execute(); }
void CStalkerActionDangerBySoundLookOut::finalize() { inherited::finalize(); }
//////////////////////////////////////////////////////////////////////////
// CStalkerActionDangerBySoundLookAround
//////////////////////////////////////////////////////////////////////////

CStalkerActionDangerBySoundLookAround::CStalkerActionDangerBySoundLookAround(CAI_Stalker* object, LPCSTR action_name)
    : inherited(object, action_name)
{
}

void CStalkerActionDangerBySoundLookAround::initialize()
{
    inherited::initialize();
    object().movement().set_desired_direction(0);
    object().movement().set_path_type(MovementManager::ePathTypeLevelPath);
    object().movement().set_detail_path_type(DetailPathManager::eDetailPathTypeSmooth);
    object().movement().set_nearest_accessible_position();
    object().movement().set_body_state(eBodyStateStand);
    object().movement().set_movement_type(eMovementTypeStand);
    object().movement().set_mental_state(eMentalStateDanger);
    object().sight().setup(SightManager::eSightTypeCurrentDirection);
    object().CObjectHandler::set_goal(eObjectActionIdle);
}

void CStalkerActionDangerBySoundLookAround::execute() { inherited::execute(); }
void CStalkerActionDangerBySoundLookAround::finalize() { inherited::finalize(); }
