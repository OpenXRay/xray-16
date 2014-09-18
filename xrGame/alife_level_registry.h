////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_level_registry.h
//	Created 	: 15.01.2003
//  Modified 	: 12.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife level registry
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "safe_map_iterator.h"
#include "xrServer_Objects_ALife.h"
#include "game_graph.h"
#include "ai_debug.h"

//#define FULL_LEVEL_UPDATE

class CSE_ALifeDynamicObject;

class CALifeLevelRegistry : public CSafeMapIterator<ALife::_OBJECT_ID,CSE_ALifeDynamicObject> {
protected:
	typedef CSafeMapIterator<ALife::_OBJECT_ID,CSE_ALifeDynamicObject> inherited;

protected:
	GameGraph::_LEVEL_ID			m_level_id;

public:
	IC								CALifeLevelRegistry	(const GameGraph::_LEVEL_ID &level_id);
	IC		void					add					(CSE_ALifeDynamicObject *tpALifeDynamicObject);
	IC		void					remove				(CSE_ALifeDynamicObject *tpALifeDynamicObject, bool no_assert = false);
	template <typename _update_predicate>
	IC		void					update				(const _update_predicate &predicate, bool const iterate_as_first_time_next_time);
	IC		GameGraph::_LEVEL_ID	level_id			() const;
	IC		CSE_ALifeDynamicObject	*object				(const ALife::_OBJECT_ID &id, bool no_assert = false) const;
};

#include "alife_level_registry_inline.h"