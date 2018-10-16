////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_kill_wounded_actions.cpp
//	Created 	: 25.05.2006
//  Modified 	: 25.05.2006
//	Author		: Dmitriy Iassenev
//	Description : Stalker kill wounded action classes
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "stalker_kill_wounded_actions.h"
#include "ai/stalker/ai_stalker.h"
#include "ai/stalker/ai_stalker_impl.h"
#include "Inventory.h"
#include "WeaponMagazined.h"
#include "stalker_movement_manager_smart_cover.h"
#include "movement_manager_space.h"
#include "detail_path_manager_space.h"
#include "memory_manager.h"
#include "enemy_manager.h"
#include "sight_manager.h"
#include "memory_space.h"
#include "xrAICore/Navigation/level_graph.h"
#include "visual_memory_manager.h"
#include "script_game_object.h"
#include "script_game_object_impl.h"
#include "restricted_object.h"
#include "sound_player.h"
#include "ai/stalker/ai_stalker_space.h"
#include "stalker_decision_space.h"
#include "agent_manager.h"
#include "agent_enemy_manager.h"

const u32 MIN_QUEUE = 0;
const u32 MAX_QUEUE = 1;
const u32 MIN_INTERVAL = 1000;
const u32 MAX_INTERVAL = 1500;

using namespace StalkerSpace;
using namespace StalkerDecisionSpace;

CInventoryItem* weapon_to_kill(const CAI_Stalker* object)
{
    if (!object->inventory().ItemFromSlot(INV_SLOT_2))
        return (object->best_weapon());

    CWeaponMagazined* temp = smart_cast<CWeaponMagazined*>(object->inventory().ItemFromSlot(INV_SLOT_2));
    if (!temp)
        return (object->best_weapon());

    if (!temp->can_kill())
        return (object->best_weapon());

    return (temp);
}

bool should_process(CAI_Stalker& object, const CEntityAlive* enemy)
{
    if (object.agent_manager().enemy().wounded_processed(enemy))
        return (false);

    ALife::_OBJECT_ID processor_id = object.agent_manager().enemy().wounded_processor(enemy);
    if ((processor_id != ALife::_OBJECT_ID(-1)) && (processor_id != object.ID()))
        return (false);

    return (true);
}

//////////////////////////////////////////////////////////////////////////
// CStalkerActionReachWounded
//////////////////////////////////////////////////////////////////////////

CStalkerActionReachWounded::CStalkerActionReachWounded(CAI_Stalker* object, LPCSTR action_name)
    : inherited(object, action_name)
{
}

void CStalkerActionReachWounded::initialize()
{
    inherited::initialize();

    m_storage->set_property(eWorldPropertyWoundedEnemyPrepared, false);
    m_storage->set_property(eWorldPropertyWoundedEnemyAimed, false);
    object().movement().set_desired_direction(0);
    object().movement().set_path_type(MovementManager::ePathTypeLevelPath);
    object().movement().set_detail_path_type(DetailPathManager::eDetailPathTypeSmooth);
    object().movement().set_body_state(eBodyStateStand);
    object().movement().set_mental_state(eMentalStateFree);
    object().sight().setup(CSightAction(SightManager::eSightTypePathDirection, false));
    object().movement().set_movement_type(eMovementTypeWalk);
    object().set_goal(eObjectActionIdle, weapon_to_kill(&object()), MIN_QUEUE, MAX_QUEUE, MIN_INTERVAL, MAX_INTERVAL);
}

void CStalkerActionReachWounded::finalize()
{
    inherited::finalize();
    //	object().movement().set_desired_position	(0);
}

void CStalkerActionReachWounded::execute()
{
    inherited::execute();

    if (!object().memory().enemy().selected())
        return;

    const CEntityAlive* enemy = object().memory().enemy().selected();

    if (object().agent_manager().enemy().wounded_processed(enemy))
    {
        object().movement().set_movement_type(eMovementTypeStand);
        return;
    }

    CMemoryInfo mem_object = object().memory().memory(enemy);

    if (!mem_object.m_object)
    {
        object().movement().set_movement_type(eMovementTypeStand);
        return;
    }

    if (object().movement().accessible(mem_object.m_object_params.m_level_vertex_id))
        object().movement().set_level_dest_vertex(mem_object.m_object_params.m_level_vertex_id);
    else
        object().movement().set_nearest_accessible_position(
            ai().level_graph().vertex_position(mem_object.m_object_params.m_level_vertex_id),
            mem_object.m_object_params.m_level_vertex_id);

    if (should_process(object(), enemy))
    {
        object().movement().set_movement_type(eMovementTypeWalk);
        return;
    }

    ALife::_OBJECT_ID processor_id = object().agent_manager().enemy().wounded_processor(enemy);
    if (processor_id == ALife::_OBJECT_ID(-1))
    {
        object().movement().set_movement_type(eMovementTypeStand);
        return;
    }

    //	IGameObject									*processor = Level().Objects.net_Find(processor_id);
    //	if (processor && processor->Position().distance_to_sqr(object().Position()) < _sqr(3.f)) {
    //		object().movement().set_movement_type	(eMovementTypeStand);
    //		return;
    //	}

    if (object().Position().distance_to_sqr(mem_object.m_object_params.m_position) < _sqr(3.f))
    {
        object().movement().set_movement_type(eMovementTypeStand);
        return;
    }

    object().movement().set_movement_type(eMovementTypeWalk);
}

//////////////////////////////////////////////////////////////////////////
// CStalkerActionAimWounded
//////////////////////////////////////////////////////////////////////////

CStalkerActionAimWounded::CStalkerActionAimWounded(CAI_Stalker* object, LPCSTR action_name)
    : inherited(object, action_name)
{
}

void CStalkerActionAimWounded::initialize()
{
    inherited::initialize();

    object().movement().set_desired_direction(0);
    object().movement().set_path_type(MovementManager::ePathTypeLevelPath);
    object().movement().set_detail_path_type(DetailPathManager::eDetailPathTypeSmooth);
    object().movement().set_mental_state(eMentalStateDanger);
    object().movement().set_body_state(eBodyStateStand);
    object().movement().set_movement_type(eMovementTypeStand);
    object().CObjectHandler::set_goal(
        eObjectActionAimReady1, weapon_to_kill(&object()), MIN_QUEUE, MAX_QUEUE, MIN_INTERVAL, MAX_INTERVAL);

    const CEntityAlive* enemy = object().memory().enemy().selected();
    object().sight().setup(CSightAction(enemy, true));
    object().agent_manager().enemy().wounded_processed(enemy, true);

    if (!object().memory().visual().visible_now(enemy))
        object().movement().set_movement_type(eMovementTypeWalk);

    //	m_speed									= object().movement().m_head.speed;
    //	object().movement().danger_head_speed	(PI_DIV_4);
}

void CStalkerActionAimWounded::execute()
{
    inherited::execute();

    if (first_time())
        return;

    if (!completed())
        return;

    if (!should_process(object(), object().memory().enemy().selected()))
        return;

    const SBoneRotation& head = object().movement().m_head;
    if (!fsimilar(head.current.yaw, head.target.yaw))
        return;

    if (!fsimilar(head.current.pitch, head.target.pitch))
        return;

    if (!object().memory().visual().visible_now(object().memory().enemy().selected()))
        return;

    m_storage->set_property(eWorldPropertyWoundedEnemyAimed, true);
}

void CStalkerActionAimWounded::finalize()
{
    inherited::finalize();

    //	object().movement().danger_head_speed	(m_speed);
}

//////////////////////////////////////////////////////////////////////////
// CStalkerActionPrepareWounded
//////////////////////////////////////////////////////////////////////////

CStalkerActionPrepareWounded::CStalkerActionPrepareWounded(CAI_Stalker* object, LPCSTR action_name)
    : inherited(object, action_name)
{
}

void CStalkerActionPrepareWounded::initialize()
{
    inherited::initialize();

    object().movement().set_desired_direction(0);
    object().movement().set_path_type(MovementManager::ePathTypeLevelPath);
    object().movement().set_detail_path_type(DetailPathManager::eDetailPathTypeSmooth);
    object().movement().set_mental_state(eMentalStateDanger);
    object().movement().set_body_state(eBodyStateStand);
    object().movement().set_movement_type(eMovementTypeStand);
    object().sound().play(eStalkerSoundKillWounded);
    object().CObjectHandler::set_goal(
        eObjectActionAimReady1, weapon_to_kill(&object()), MIN_QUEUE, MAX_QUEUE, MIN_INTERVAL, MAX_INTERVAL);
}

void CStalkerActionPrepareWounded::finalize()
{
    inherited::finalize();

    object().sound().set_sound_mask(0);
}

void CStalkerActionPrepareWounded::execute()
{
    inherited::execute();

    if (!object().memory().enemy().selected())
        return;

    if (!should_process(object(), object().memory().enemy().selected()))
    {
        object().sound().set_sound_mask((u32)eStalkerSoundMaskKillWounded);
        return;
    }

    const CEntityAlive* enemy = object().memory().enemy().selected();

    if (object().agent_manager().enemy().wounded_processor(enemy) != object().ID())
        return;

    //	not a bug since killer do not look at enemy and can be too close
    //	to see him straight forward
    //	VERIFY						(object().memory().visual().visible_now(enemy));
    object().sight().setup(CSightAction(enemy, true));

    if (!object().sound().active_sound_count(true))
        m_storage->set_property(eWorldPropertyWoundedEnemyPrepared, true);
}

//////////////////////////////////////////////////////////////////////////
// CStalkerActionKillWounded
//////////////////////////////////////////////////////////////////////////

CStalkerActionKillWounded::CStalkerActionKillWounded(CAI_Stalker* object, LPCSTR action_name)
    : inherited(object, action_name)
{
}

void CStalkerActionKillWounded::initialize()
{
    inherited::initialize();

    m_storage->set_property(eWorldPropertyPausedAfterKill, true);
    object().movement().set_desired_direction(0);
    object().movement().set_path_type(MovementManager::ePathTypeLevelPath);
    object().movement().set_detail_path_type(DetailPathManager::eDetailPathTypeSmooth);
    object().movement().set_mental_state(eMentalStateDanger);
    object().movement().set_body_state(eBodyStateStand);
    object().movement().set_movement_type(eMovementTypeStand);
}

void CStalkerActionKillWounded::finalize() { inherited::finalize(); }
void CStalkerActionKillWounded::execute()
{
    inherited::execute();

    if (!object().memory().enemy().selected())
        return;

    const CEntityAlive* enemy = object().memory().enemy().selected();
    object().sight().setup(CSightAction(enemy, true));
    object().set_goal(eObjectActionFire1, weapon_to_kill(&object()), MIN_QUEUE, MAX_QUEUE, MIN_INTERVAL, MAX_INTERVAL);

    if (!object().inventory().ActiveItem())
        return;

    if (object().memory().visual().visible_now(enemy) && object().can_kill_enemy() && !object().can_kill_member())
        return;

    // this is fake
    // but sometimes enemy can not be visible
    // when it plays animation inside another object
    // therefore we should use this ugly workaround
    // and hit enemy virtually
    NET_Packet P;
    SHit HS;
    HS.GenHeader(GE_HIT, enemy->ID());
    HS.whoID = object().ID();
    HS.weaponID = weapon_to_kill(&object())->object().ID();
    HS.dir = Fvector().set(0.f, 0.f, 1.f);
    HS.power = 1.f;
    HS.boneID = smart_cast<IKinematics*>((const_cast<CEntityAlive*>(enemy))->Visual())->LL_GetBoneRoot();
    HS.p_in_bone_space = Fvector().set(0.f, 0.f, 0.f);
    HS.impulse = 1.f;
    HS.hit_type = ALife::eHitTypeWound;
    HS.Write_Packet(P);
    object().u_EventSend(P);
}

//////////////////////////////////////////////////////////////////////////
// CStalkerActionPauseAfterKill
//////////////////////////////////////////////////////////////////////////

CStalkerActionPauseAfterKill::CStalkerActionPauseAfterKill(CAI_Stalker* object, LPCSTR action_name)
    : inherited(object, action_name)
{
}

void CStalkerActionPauseAfterKill::initialize()
{
    inherited::initialize();

    object().movement().set_desired_direction(0);
    object().movement().set_path_type(MovementManager::ePathTypeLevelPath);
    object().movement().set_detail_path_type(DetailPathManager::eDetailPathTypeSmooth);
    object().movement().set_mental_state(eMentalStateDanger);
    object().movement().set_body_state(eBodyStateStand);
    object().movement().set_movement_type(eMovementTypeStand);
    object().CObjectHandler::set_goal(
        eObjectActionAimReady1, weapon_to_kill(&object()), MIN_QUEUE, MAX_QUEUE, MIN_INTERVAL, MAX_INTERVAL);
    object().sight().setup(CSightAction(SightManager::eSightTypeCurrentDirection, true));
}

void CStalkerActionPauseAfterKill::execute()
{
    inherited::execute();

    if (!completed())
        return;

    m_storage->set_property(eWorldPropertyPausedAfterKill, false);
}
