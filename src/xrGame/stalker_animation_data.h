////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_animation_data.h
//	Created 	: 13.10.2005
//  Modified 	: 13.10.2005
//	Author		: Dmitriy Iassenev
//	Description : Stalker animation data
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "stalker_animation_state.h"
#include "stalker_animation_names.h"

class CStalkerAnimationData {
public:
	typedef CStalkerAnimationState::WEAPON_ACTIONS				WEAPON_ACTIONS;
	typedef CAniCollection<WEAPON_ACTIONS,weapon_names>			GLOBAL_ANIMATIONS;
	typedef CAniCollection<CStalkerAnimationState,state_names>	PART_ANIMATIONS;
	typedef CAniFVector<head_names>								HEAD_ANIMATIONS;

public:
	PART_ANIMATIONS		m_part_animations;
	HEAD_ANIMATIONS		m_head_animations;
	GLOBAL_ANIMATIONS	m_global_animations;

public:
						CStalkerAnimationData	(IKinematicsAnimated *skeleton_animated);
};
