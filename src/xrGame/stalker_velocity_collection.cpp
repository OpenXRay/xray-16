////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_velocity_collection.cpp
//	Created 	: 23.12.2005
//  Modified 	: 23.12.2005
//	Author		: Dmitriy Iassenev
//	Description : Stalker velocity collection
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "stalker_velocity_collection.h"

using namespace MonsterSpace;

CStalkerVelocityCollection::CStalkerVelocityCollection	(const shared_str &section)
{
	m_danger[eBodyStateCrouch][eMovementTypeWalk][eMovementDirectionForward]	= pSettings->r_float(section,"danger_crouch_walk_forward");
	m_danger[eBodyStateCrouch][eMovementTypeWalk][eMovementDirectionBackward]	= pSettings->r_float(section,"danger_crouch_walk_backward");
	m_danger[eBodyStateCrouch][eMovementTypeWalk][eMovementDirectionLeft]		= pSettings->r_float(section,"danger_crouch_walk_left");
	m_danger[eBodyStateCrouch][eMovementTypeWalk][eMovementDirectionRight]		= pSettings->r_float(section,"danger_crouch_walk_right");

	m_danger[eBodyStateCrouch][eMovementTypeRun][eMovementDirectionForward]		= pSettings->r_float(section,"danger_crouch_run_forward");
	m_danger[eBodyStateCrouch][eMovementTypeRun][eMovementDirectionBackward]	= pSettings->r_float(section,"danger_crouch_run_backward");
	m_danger[eBodyStateCrouch][eMovementTypeRun][eMovementDirectionLeft]		= pSettings->r_float(section,"danger_crouch_run_left");
	m_danger[eBodyStateCrouch][eMovementTypeRun][eMovementDirectionRight]		= pSettings->r_float(section,"danger_crouch_run_right");

	m_danger[eBodyStateStand][eMovementTypeWalk][eMovementDirectionForward]		= pSettings->r_float(section,"danger_stand_walk_forward");
	m_danger[eBodyStateStand][eMovementTypeWalk][eMovementDirectionBackward]	= pSettings->r_float(section,"danger_stand_walk_backward");
	m_danger[eBodyStateStand][eMovementTypeWalk][eMovementDirectionLeft]		= pSettings->r_float(section,"danger_stand_walk_left");
	m_danger[eBodyStateStand][eMovementTypeWalk][eMovementDirectionRight]		= pSettings->r_float(section,"danger_stand_walk_right");

	m_danger[eBodyStateStand][eMovementTypeRun][eMovementDirectionForward]		= pSettings->r_float(section,"danger_stand_run_forward");
	m_danger[eBodyStateStand][eMovementTypeRun][eMovementDirectionBackward]		= pSettings->r_float(section,"danger_stand_run_backward");
	m_danger[eBodyStateStand][eMovementTypeRun][eMovementDirectionLeft]			= pSettings->r_float(section,"danger_stand_run_left");
	m_danger[eBodyStateStand][eMovementTypeRun][eMovementDirectionRight]		= pSettings->r_float(section,"danger_stand_run_right");

	m_free[eMovementTypeWalk]													= pSettings->r_float(section,"free_stand_walk_forward");
	m_free[eMovementTypeRun]													= pSettings->r_float(section,"free_stand_run_forward");

	m_panic																		= pSettings->r_float(section,"panic_stand_run_forward");
}
