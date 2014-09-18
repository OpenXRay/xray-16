////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_stalker_impl.h
//	Created 	: 25.02.2003
//  Modified 	: 25.02.2003
//	Author		: Dmitriy Iassenev
//	Description : AI Behaviour for monster "Stalker" (inline functions implementation)
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "../../level.h"
#include "../../seniority_hierarchy_holder.h"
#include "../../team_hierarchy_holder.h"
#include "../../squad_hierarchy_holder.h"
#include "../../group_hierarchy_holder.h"
#include "../../effectorshot.h"
#include "stalker_movement_manager_smart_cover.h"
#include "smart_cover_animation_selector.h"
#include "smart_cover_animation_planner.h"

IC	CAgentManager &CAI_Stalker::agent_manager	() const
{
	return			(Level().seniority_holder().team(g_Team()).squad(g_Squad()).group(g_Group()).agent_manager());
}

IC	Fvector CAI_Stalker::weapon_shot_effector_direction	(const Fvector &current) const
{
#if 1
	return			current;
#else // #if 1
	VERIFY			(weapon_shot_effector().IsActive());
	Fvector			result;
	weapon_shot_effector().GetDeltaAngle(result);

	float			y,p;
	current.getHP	(y,p);

	result.setHP	(-result.y + y, -result.x + p);

	return			(result);
#endif // #if 1
}