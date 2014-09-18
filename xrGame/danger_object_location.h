////////////////////////////////////////////////////////////////////////////
//	Module 		: danger_object_location.h
//	Created 	: 24.05.2004
//  Modified 	: 14.01.2005
//	Author		: Dmitriy Iassenev
//	Description : Danger object location
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "danger_location.h"

class CGameObject;

class CDangerObjectLocation : public CDangerLocation {
private:
	const CGameObject		*m_object;

public:
	IC						CDangerObjectLocation	(const CGameObject *object, u32 level_time, u32 interval, float radius, const squad_mask_type &mask = squad_mask_type(-1));
	virtual const Fvector	&position				() const;
	virtual bool			useful					() const;
	virtual	bool			operator==				(const CObject *object) const;
};

#include "danger_object_location_inline.h"