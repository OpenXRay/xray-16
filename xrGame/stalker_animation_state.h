////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_animation_state.h
//	Created 	: 25.02.2003
//  Modified 	: 19.11.2004
//	Author		: Dmitriy Iassenev
//	Description : Stalker state animations
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ai/ai_monsters_anims.h"
#include "stalker_animation_names.h"

class CStalkerAnimationState {
public:
	typedef CAniCollection<CAniVector,		movement_action_names>	MOVEMENT_ACTIONS;
	typedef CAniCollection<CAniVector,		weapon_action_names>	WEAPON_ACTIONS;
	typedef CAniCollection<CAniVector,		global_names>			GLOBAL_ANIMATIONS;
	typedef CAniCollection<WEAPON_ACTIONS,	weapon_names>			WEAPON_ANIMATIONS;
	typedef CAniCollection<MOVEMENT_ACTIONS,movement_names>			MOVEMENT_ANIMATIONS;
	typedef CAniFVector	  <					in_place_names>			IN_PLACE_ANIMATIONS;

public:
	GLOBAL_ANIMATIONS		m_global;
	WEAPON_ANIMATIONS		m_torso;
	MOVEMENT_ANIMATIONS		m_movement;
	IN_PLACE_ANIMATIONS		*m_in_place;

public:
					CStalkerAnimationState	();
					CStalkerAnimationState	(const CStalkerAnimationState &animations);
	virtual			~CStalkerAnimationState	();
			void	Load					(IKinematicsAnimated *kinematics, LPCSTR base_name);
};

#include "stalker_animation_state_inline.h"