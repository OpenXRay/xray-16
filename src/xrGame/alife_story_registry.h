////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_story_registry.h
//	Created 	: 02.06.2004
//  Modified 	: 02.06.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife story registry
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "alife_space.h"

class CSE_ALifeDynamicObject;

class CALifeStoryRegistry {
public:
	typedef ALife::STORY_P_MAP		STORY_REGISTRY;

protected:
	STORY_REGISTRY					m_objects;

public:
	virtual							~CALifeStoryRegistry	();
			void					add						(ALife::_STORY_ID id, CSE_ALifeDynamicObject *object, bool no_assert = false);
	IC		void					remove					(ALife::_STORY_ID id, bool no_assert = false);
	IC		const STORY_REGISTRY	&objects				() const;
	IC		CSE_ALifeDynamicObject	*object					(ALife::_STORY_ID id, bool no_assert = false) const;
};

#include "alife_story_registry_inline.h"