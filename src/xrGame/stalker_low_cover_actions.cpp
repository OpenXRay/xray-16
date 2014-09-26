////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_low_cover_actions.cpp
//	Created 	: 05.09.2007
//  Modified 	: 05.09.2007
//	Author		: Dmitriy Iassenev
//	Description : Stalker low cover actions
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "stalker_low_cover_actions.h"
#include "ai/stalker/ai_stalker.h"
#include "sight_manager.h"
#include "memory_manager.h"
#include "enemy_manager.h"
#include "stalker_movement_manager_smart_cover.h"
#include "memory_space.h"
#include "agent_manager.h"
#include "agent_member_manager.h"
#include "stalker_decision_space.h"
#include "sound_player.h"
#include "ai/stalker/ai_stalker_space.h"
#include "script_game_object.h"
#include "stalker_combat_planner.h"
#include "stalker_planner.h"

using namespace StalkerDecisionSpace;

//////////////////////////////////////////////////////////////////////////
// CStalkerActionGetReadyToKillLowCover
//////////////////////////////////////////////////////////////////////////

CStalkerActionGetReadyToKillLowCover::CStalkerActionGetReadyToKillLowCover(CAI_Stalker *object, LPCSTR action_name) :
	inherited(object,action_name)
{
}

void CStalkerActionGetReadyToKillLowCover::initialize					()
{
	inherited::initialize				();

	object().brain().affect_cover		(true);
}

void CStalkerActionGetReadyToKillLowCover::execute						()
{
	inherited::execute					();
	
	object().movement().set_body_state	(eBodyStateCrouch);
	object().sight().setup				(CSightAction(SightManager::eSightTypeCurrentDirection));
	aim_ready_force_full				();
}

void CStalkerActionGetReadyToKillLowCover::finalize						()
{
	inherited::finalize					();

	object().brain().affect_cover		(false);
}

//////////////////////////////////////////////////////////////////////////
// CStalkerActionKillEnemyLowCover
//////////////////////////////////////////////////////////////////////////

CStalkerActionKillEnemyLowCover::CStalkerActionKillEnemyLowCover		(CAI_Stalker *object, LPCSTR action_name) :
	inherited(object,action_name)
{
}

void CStalkerActionKillEnemyLowCover::initialize						()
{
	inherited::initialize				();

	object().movement().set_body_state	(eBodyStateStand);

	object().brain().affect_cover		(true);

#ifndef SILENT_COMBAT
	play_attack_sound					(0,0,6000,4000);
#endif
}

void CStalkerActionKillEnemyLowCover::execute							()
{
	inherited::execute					();

	object().sight().setup				(CSightAction(object().memory().enemy().selected(),true,true));

	fire								();

	if (!object().memory().enemy().selected())
		return;

	CMemoryInfo							mem_object = object().memory().memory(object().memory().enemy().selected());

	if (!mem_object.m_object)
		return;
	
	object().best_cover					(mem_object.m_object_params.m_position);
}

void CStalkerActionKillEnemyLowCover::finalize							()
{
	inherited::finalize					();

	object().brain().affect_cover		(false);
}

//////////////////////////////////////////////////////////////////////////
// CStalkerActionHoldPositionLowCover
//////////////////////////////////////////////////////////////////////////

CStalkerActionHoldPositionLowCover::CStalkerActionHoldPositionLowCover		(CAI_Stalker *object, LPCSTR action_name) :
	inherited(object,action_name)
{
}

void CStalkerActionHoldPositionLowCover::initialize							()
{
	inherited::initialize				();

	object().brain().affect_cover		(true);

	object().movement().set_body_state	(eBodyStateStand);

	aim_ready							();

	set_inertia_time					(1000 + ::Random32.random(2000));
}

void CStalkerActionHoldPositionLowCover::execute							()
{
	inherited::execute					();

	CMemoryInfo							mem_object = object().memory().memory(object().memory().enemy().selected());

	if (!mem_object.m_object)
		return;

	object().sight().setup				(CSightAction(SightManager::eSightTypePosition,mem_object.m_object_params.m_position,true));

	if (completed()) {
		if	(
				object().agent_manager().member().can_detour() ||
				!object().agent_manager().member().cover_detouring() ||
				!fire_make_sense()
			) {
			CStalkerCombatPlanner		&planner = smart_cast<CStalkerCombatPlanner&>(object().brain().current_action());
			planner.CScriptActionPlanner::m_storage.set_property(eWorldPropertyLookedOut,true);
			planner.CScriptActionPlanner::m_storage.set_property(eWorldPropertyPositionHolded,true);
			planner.CScriptActionPlanner::m_storage.set_property(eWorldPropertyInCover,false);
		}
	}

	if (object().agent_manager().member().cover_detouring() && fire_make_sense()) {
		object().sound().play			(StalkerSpace::eStalkerSoundNeedBackup,3000,3000,10000,10000);
		fire							();
	}
	else {
		aim_ready						();
	}

	if (object().memory().enemy().selected()) {
		CMemoryInfo						mem_object = object().memory().memory(object().memory().enemy().selected());

		if (mem_object.m_object) {
			object().best_cover			(mem_object.m_object_params.m_position);
		}
	}
}

void CStalkerActionHoldPositionLowCover::finalize							()
{
	inherited::finalize					();

	object().brain().affect_cover		(false);
}