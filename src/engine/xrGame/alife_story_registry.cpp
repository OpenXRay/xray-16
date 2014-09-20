////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_story_registry.cpp
//	Created 	: 02.06.2004
//  Modified 	: 02.06.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife story registry
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "alife_story_registry.h"
#include "xrServer_Objects_ALife.h"
#include "ai_space.h"
#include "game_graph.h"

CALifeStoryRegistry::~CALifeStoryRegistry	()
{
}

void CALifeStoryRegistry::add				(ALife::_STORY_ID id, CSE_ALifeDynamicObject *object, bool no_assert)
{
	if (id == INVALID_STORY_ID)
		return;

#ifdef DEBUG
	Msg("Adding Story item ID [%u], Object [%s] at level [%s]",id, object->name_replace(), *ai().game_graph().header().level(ai().game_graph().vertex(object->m_tGraphID)->level_id()).name());
#endif

	ALife::STORY_P_PAIR_IT	I = m_objects.find(id);
	if (I != m_objects.end()) {
		R_ASSERT2			(no_assert,"Specified story object is already in the Story registry!");
		return;
	} 

	m_objects.insert		(std::make_pair(id,object));
}
