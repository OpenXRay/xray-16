////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_velocity_collection.h
//	Created 	: 13.12.2005
//  Modified 	: 13.12.2005
//	Author		: Dmitriy Iassenev
//	Description : Stalker velocity collection
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ai_monster_space.h"

class CStalkerVelocityCollection {
private:
	float				m_danger[2][2][4];
	float				m_free[2];
	float				m_panic;

public:
						CStalkerVelocityCollection	(const shared_str &section);
	inline	float		velocity					(
		const MonsterSpace::EMentalState		&mental_state,
		const MonsterSpace::EBodyState			&body_state,
		const MonsterSpace::EMovementType		&movement_type,
		const MonsterSpace::EMovementDirection	&movement_direction
	) const;
};

#include "stalker_velocity_collection_inline.h"