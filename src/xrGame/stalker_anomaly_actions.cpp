////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_anomaly_actions.cpp
//	Created 	: 25.03.2004
//  Modified 	: 26.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Stalker anomaly action classes
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "stalker_anomaly_actions.h"
#include "ai/stalker/ai_stalker.h"
#include "script_game_object.h"
#include "script_game_object_impl.h"
#include "stalker_decision_space.h"
#include "CustomZone.h"
#include "space_restriction_manager.h"
#include "space_restriction_bridge.h"
#include "space_restriction_base.h"
#include "Inventory.h"
#include "movement_manager_space.h"
#include "detail_path_manager_space.h"
#include "memory_manager.h"
#include "enemy_manager.h"
#include "sight_manager.h"
#include "restricted_object.h"
#include "stalker_movement_manager_smart_cover.h"
#include "sound_player.h"
#include "ai/stalker/ai_stalker_space.h"
#include "RadioactiveZone.h"
#include "alife_simulator.h"
#include "alife_object_registry.h"
#include "xrServerEntities/xrServer_Objects_ALife_Monsters.h"

using namespace StalkerSpace;
using namespace StalkerDecisionSpace;

//////////////////////////////////////////////////////////////////////////
// CStalkerActionGetOutOfAnomaly
//////////////////////////////////////////////////////////////////////////

CStalkerActionGetOutOfAnomaly::CStalkerActionGetOutOfAnomaly(CAI_Stalker* object, LPCSTR action_name)
    : inherited(object, action_name)
{
}

void CStalkerActionGetOutOfAnomaly::initialize()
{
    inherited::initialize();

    object().sound().remove_active_sounds(u32(eStalkerSoundMaskNoHumming));

    object().movement().set_desired_direction(0);
    object().movement().set_path_type(MovementManager::ePathTypeLevelPath);
    object().movement().set_detail_path_type(DetailPathManager::eDetailPathTypeSmooth);
    object().movement().set_body_state(eBodyStateStand);
    object().movement().set_movement_type(eMovementTypeWalk);
    object().movement().set_mental_state(eMentalStateDanger);
    object().sight().setup(SightManager::eSightTypeCurrentDirection);
    if (object().memory().enemy().selected() && object().inventory().ActiveItem() && object().best_weapon() &&
        (object().inventory().ActiveItem()->object().ID() == object().best_weapon()->object().ID()))
        object().CObjectHandler::set_goal(eObjectActionIdle, object().best_weapon());
    else
        object().CObjectHandler::set_goal(eObjectActionIdle);
    set_property(eWorldPropertyAnomaly, true);
}

void CStalkerActionGetOutOfAnomaly::finalize()
{
    inherited::finalize();

    if (!object().g_Alive())
        return;

    object().sound().set_sound_mask(0);
}

void CStalkerActionGetOutOfAnomaly::execute()
{
    inherited::execute();
    //
    object().movement().set_path_type(MovementManager::ePathTypeLevelPath);
    object().movement().set_detail_path_type(DetailPathManager::eDetailPathTypeSmooth);
    object().movement().set_body_state(eBodyStateStand);
    object().movement().set_movement_type(eMovementTypeWalk);
    object().movement().set_mental_state(eMentalStateDanger);
    //

    m_temp0.clear();
    m_temp1.clear();

    CSE_ALifeDynamicObject const* const base_alife_object = ai().alife().objects().object(object().ID(), true);
    if (!base_alife_object)
        return;

    CSE_ALifeHumanAbstract const* const alife_object = smart_cast<CSE_ALifeHumanAbstract const*>(base_alife_object);
    if (!alife_object)
        return;

    typedef xr_vector<ALife::_OBJECT_ID> ids_type;
    ids_type const& restrictions = alife_object->m_dynamic_in_restrictions;

    xr_vector<IGameObject*>::const_iterator I = object().feel_touch.begin();
    xr_vector<IGameObject*>::const_iterator E = object().feel_touch.end();
    for (; I != E; ++I)
    {
        CCustomZone* zone = smart_cast<CCustomZone*>(*I);
        if (zone && (zone->restrictor_type() != RestrictionSpace::eRestrictorTypeNone))
        {
            if (smart_cast<CRadioactiveZone*>(zone))
                continue;

            if (std::find(restrictions.begin(), restrictions.end(), zone->ID()) != restrictions.end())
                continue;

            m_temp0.push_back(zone->ID());
        }
    }

    object().movement().restrictions().add_restrictions(m_temp1, m_temp0);
    object().movement().set_nearest_accessible_position();
}

//////////////////////////////////////////////////////////////////////////
// CStalkerActionDetectAnomaly
//////////////////////////////////////////////////////////////////////////

CStalkerActionDetectAnomaly::CStalkerActionDetectAnomaly(CAI_Stalker* object, LPCSTR action_name)
    : inherited(object, action_name)
{
}

void CStalkerActionDetectAnomaly::initialize()
{
    inherited::initialize();
    object().sound().remove_active_sounds(u32(eStalkerSoundMaskNoHumming));
    m_inertia_time = 15000 + ::Random32.random(5000);

    Fvector result;
    object().eye_matrix.transform_tiny(result, Fvector().set(0.f, 0.f, 10.f));
    object().throw_target(result, 0);
}

void CStalkerActionDetectAnomaly::finalize()
{
    inherited::finalize();

    if (!object().g_Alive())
        return;

    object().CObjectHandler::set_goal(eObjectActionIdle);
    object().sound().set_sound_mask(0);
}

void CStalkerActionDetectAnomaly::execute()
{
    inherited::execute();

    if (completed() || object().memory().enemy().selected())
    {
        set_property(eWorldPropertyAnomaly, false);
        return;
    }

    object().CObjectHandler::set_goal(eObjectActionFire1, object().inventory().ItemFromSlot(BOLT_SLOT));
}
