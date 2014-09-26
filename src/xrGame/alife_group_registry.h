////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_group_registry.h
//	Created 	: 28.10.2005
//  Modified 	: 28.10.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife group registry
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "alife_space.h"

class CSE_ALifeOnlineOfflineGroup;
class CSE_ALifeDynamicObject;

class CALifeGroupRegistry {
public:
	typedef CSE_ALifeOnlineOfflineGroup				OBJECT;
	typedef xr_map<ALife::_OBJECT_ID,OBJECT*>		OBJECTS;

protected:
	OBJECTS					m_objects;

public:
	virtual					~CALifeGroupRegistry	();
			void			add						(CSE_ALifeDynamicObject *object);
			void			remove					(CSE_ALifeDynamicObject *object);
			OBJECT			&object					(const ALife::_OBJECT_ID &id) const;
	IC		const OBJECTS	&objects				() const;
			void			on_after_game_load		();
};

#include "alife_group_registry_inline.h"