////////////////////////////////////////////////////////////////////////////
//	Module 		: danger_object.h
//	Created 	: 14.02.2005
//  Modified 	: 14.02.2005
//	Author		: Dmitriy Iassenev
//	Description : Danger object
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "entity_alive.h"

class CDangerObject {
public:
	enum EDangerType {
		eDangerTypeBulletRicochet = u32(0),
		eDangerTypeAttackSound,
		eDangerTypeEntityAttacked,
		eDangerTypeEntityDeath,
		eDangerTypeFreshEntityCorpse,
		eDangerTypeAttacked,
		eDangerTypeGrenade,
		eDangerTypeEnemySound,
		eDangerTypeDummy = u32(-1),
	};

	enum EDangerPerceiveType {
		eDangerPerceiveTypeVisual = u32(0),
		eDangerPerceiveTypeSound,
		eDangerPerceiveTypeHit,
		eDangerPerceiveTypeDummy = u32(-1),
	};

private:
	const CEntityAlive					*m_object;
	const CObject						*m_dependent_object;
	Fvector								m_position;
	u32									m_time;
	EDangerType							m_type;
	EDangerPerceiveType					m_perceive_type;

public:
	IC									CDangerObject			(const CEntityAlive	*object, const Fvector &position, u32 time, const EDangerType &type, const EDangerPerceiveType &perceive_type, const CObject *dependent_object = 0);
	virtual								~CDangerObject			();
	IC		const CEntityAlive			*object					() const;
	IC		const Fvector				&position				() const;
	IC		u32							time					() const;
	IC		EDangerType					type					() const;
	IC		EDangerPerceiveType			perceive_type			() const;
	IC		const CObject				*dependent_object		() const;
	IC		void						clear_dependent_object	();
	IC		bool						operator==				(const CDangerObject &object) const;
};

#include "danger_object_inline.h"