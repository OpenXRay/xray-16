////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_alife_actions.cpp
//	Created 	: 25.03.2004
//  Modified 	: 26.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Stalker alife action classes
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "stalker_alife_actions.h"
#include "ai/stalker/ai_stalker.h"
#include "inventory_item.h"
#include "script_game_object.h"
#include "script_game_object_impl.h"
#include "Inventory.h"
#include "WeaponMagazined.h"
#include "movement_manager_space.h"
#include "detail_path_manager_space.h"
#include "memory_manager.h"
#include "item_manager.h"
#include "sight_manager.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "stalker_movement_manager_smart_cover.h"
#include "patrol_path_manager.h"
#include "sound_player.h"
#include "ai/stalker/ai_stalker_space.h"
#include "restricted_object.h"

using namespace StalkerSpace;

#ifdef _DEBUG
//#	define STALKER_DEBUG_MODE
#endif

#ifdef STALKER_DEBUG_MODE
#include "attachable_item.h"
#endif

//////////////////////////////////////////////////////////////////////////
// CStalkerActionNoALife
//////////////////////////////////////////////////////////////////////////

CStalkerActionNoALife::CStalkerActionNoALife(CAI_Stalker* object, LPCSTR action_name) : inherited(object, action_name)
{
}

void CStalkerActionNoALife::initialize()
{
    inherited::initialize();
#ifndef STALKER_DEBUG_MODE
    object().movement().set_desired_position(0);
    object().movement().set_desired_direction(0);
    object().movement().set_path_type(MovementManager::ePathTypeGamePath);
    object().movement().set_detail_path_type(DetailPathManager::eDetailPathTypeSmooth);
    object().movement().set_body_state(eBodyStateStand);
    object().movement().set_movement_type(eMovementTypeWalk);
    object().movement().set_mental_state(eMentalStateFree);
    object().sight().setup(CSightAction(SightManager::eSightTypeCover, false, true));

    m_stop_weapon_handling_time = Device.dwTimeGlobal;
    if (object().inventory().ActiveItem() && object().best_weapon() &&
        (object().inventory().ActiveItem()->object().ID() == object().best_weapon()->object().ID()))
        m_stop_weapon_handling_time += ::Random32.random(30000) + 30000;

#else
    object().movement().set_mental_state(eMentalStateDanger);
    object().movement().set_movement_type(eMovementTypeStand);
    object().movement().set_body_state(eBodyStateStand);
    object().movement().set_desired_direction(0);
    object().movement().set_path_type(MovementManager::ePathTypeLevelPath);
    object().movement().set_detail_path_type(DetailPathManager::eDetailPathTypeSmooth);
    object().movement().set_nearest_accessible_position();
    object().sight().setup(CSightAction(SightManager::eSightTypeCurrentDirection));
    object().CObjectHandler::set_goal(
        eObjectActionFire1, object().inventory().ItemFromSlot(INV_SLOT_2), 0, 1, 2500, 3000);
//	object().movement().patrol().set_path		("way_0000",ePatrolStartTypeNearest);
#endif
}

void CStalkerActionNoALife::finalize()
{
    inherited::finalize();

    object().movement().set_desired_position(0);

    if (!object().g_Alive())
        return;

    object().sound().remove_active_sounds(u32(eStalkerSoundMaskNoHumming));
}

void CStalkerActionNoALife::execute()
{
    inherited::execute();
#ifndef STALKER_DEBUG_MODE
    object().sound().play(eStalkerSoundHumming, 60000, 10000);
    if (Device.dwTimeGlobal >= m_stop_weapon_handling_time)
        if (!object().best_weapon())
            object().CObjectHandler::set_goal(eObjectActionIdle);
        else
            object().CObjectHandler::set_goal(eObjectActionStrapped, object().best_weapon());
    else
        object().CObjectHandler::set_goal(eObjectActionIdle, object().best_weapon());
#else
//	object().movement().set_movement_type		(eMovementTypeRun);
#endif
}

//////////////////////////////////////////////////////////////////////////
// CStalkerActionGatherItems
//////////////////////////////////////////////////////////////////////////

CStalkerActionGatherItems::CStalkerActionGatherItems(CAI_Stalker* object, LPCSTR action_name)
    : inherited(object, action_name)
{
}

void CStalkerActionGatherItems::initialize()
{
    inherited::initialize();

    object().movement().set_desired_direction(0);
    object().movement().set_path_type(MovementManager::ePathTypeLevelPath);
    object().movement().set_detail_path_type(DetailPathManager::eDetailPathTypeSmooth);
    object().movement().set_body_state(eBodyStateStand);
    object().movement().set_movement_type(eMovementTypeWalk);
    object().movement().set_mental_state(eMentalStateDanger);
    object().sound().remove_active_sounds(u32(eStalkerSoundMaskNoHumming));
    if (!object().inventory().ActiveItem())
        object().CObjectHandler::set_goal(eObjectActionIdle);
    else
        object().CObjectHandler::set_goal(eObjectActionIdle, object().inventory().ActiveItem());

    IGameObject const* const selected = object().memory().item().selected();

    typedef CAI_Stalker::ignored_touched_objects_type ignored_touched_objects_type;
    ignored_touched_objects_type& ignored_touched_objects = m_object->ignored_touched_objects();
    ignored_touched_objects_type::iterator i =
        std::find(ignored_touched_objects.begin(), ignored_touched_objects.end(), selected);
    if (i == ignored_touched_objects.end())
        return;

    ignored_touched_objects.erase(i);

    m_object->generate_take_event(selected);
}

void CStalkerActionGatherItems::finalize()
{
    inherited::finalize();

    object().sight().setup(SightManager::eSightTypePathDirection);

    object().movement().set_desired_position(0);

    if (!object().g_Alive())
        return;

    object().sound().set_sound_mask(0);
}

void CStalkerActionGatherItems::execute()
{
    inherited::execute();

    if (!object().memory().item().selected())
        return;

    u32 level_vertex_id = object().memory().item().selected()->ai_location().level_vertex_id();
    //	if (object().movement().restrictions().accessible(level_vertex_id)) {
    object().movement().set_level_dest_vertex(level_vertex_id);
    object().movement().set_desired_position(&object().memory().item().selected()->Position());
    //	}
    //	else {
    //		object().movement().set_nearest_accessible_position	(
    //			object().memory().item().selected()->Position(),
    //			level_vertex_id
    //		);
    //	}

    object().sight().setup(SightManager::eSightTypePosition, &object().memory().item().selected()->Position());
}
