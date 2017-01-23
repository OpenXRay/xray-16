////////////////////////////////////////////////////////////////////////////
//	Module 		: sight_control_action.h
//	Created 	: 05.04.2004
//  Modified 	: 05.04.2004
//	Author		: Dmitriy Iassenev
//	Description : Sight control action
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "sight_action.h"
#include "ai_monster_space.h"

class CSightControlAction : public CSightAction {
public:
	typedef MonsterSpace::SBoneRotation	SBoneRotation;

protected:
	float								m_weight;
	u32									m_inertia_time;
	u32									m_start_time;

public:
	IC									CSightControlAction	(float weight, u32 inertia_time, const CSightAction &sight_action);
	IC	float							weight				() const;
	IC	bool							completed			() const;
	IC	bool							use_torso_look		() const;
	IC	const SightManager::ESightType	&sight_type			() const;
	IC	const Fvector					&vector3d			() const;
	IC	const CGameObject				&object				() const;
};

#include "sight_control_action_inline.h"