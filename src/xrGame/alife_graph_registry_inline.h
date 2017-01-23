////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_graph_registry_inline.h
//	Created 	: 15.01.2003
//  Modified 	: 12.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife graph registry inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CALifeLevelRegistry &CALifeGraphRegistry::level	() const
{
	VERIFY						(m_level);
	return						(*m_level);
}

IC	void CALifeGraphRegistry::change	(CSE_ALifeDynamicObject *object, GameGraph::_GRAPH_ID tGraphPointID, GameGraph::_GRAPH_ID tNextGraphPointID)
{
	VERIFY3						(object->used_ai_locations()/** && (object->interactive() || object->m_bOnline)/**/,*object->s_name,object->name_replace());
	remove						(object,tGraphPointID);
	add							(object,tNextGraphPointID);
	object->m_tGraphID			= tNextGraphPointID;
	object->o_Position			= ai().game_graph().vertex(object->m_tGraphID)->level_point();
	object->m_tNodeID			= ai().game_graph().vertex(object->m_tGraphID)->level_vertex_id();
}

IC	void CALifeGraphRegistry::assign	(CSE_ALifeMonsterAbstract *monster)
{
	monster->m_tNextGraphID		= monster->m_tPrevGraphID = monster->m_tGraphID;
	monster->m_fDistanceToPoint	= monster->m_fDistance;
	CGameGraph::const_iterator	i,e;
	ai().game_graph().begin		(monster->m_tNextGraphID,i,e);
	for ( ; i != e; ++i)
		if ((*i).distance() > monster->m_fDistance) {
			monster->m_fDistanceFromPoint	= (*i).distance() - monster->m_fDistance;
			break;
		}
}

IC	void CALifeGraphRegistry::set_process_time	(const float &process_time)
{
	m_process_time				= process_time;
	if (m_level)
		level().set_process_time(m_process_time);
}

IC	CSE_ALifeCreatureActor *CALifeGraphRegistry::actor	() const
{
	return						(m_actor);
}

IC	const CALifeGraphRegistry::GRAPH_REGISTRY &CALifeGraphRegistry::objects	() const
{
	return						(m_objects);
}

template <typename F>
IC	void CALifeGraphRegistry::iterate_objects			(GameGraph::_GRAPH_ID game_vertex_id, const F& f)
{
	iterate						(((CGraphPointInfo&)(objects()[game_vertex_id])).objects(),f);
}

template <typename F, typename C>
IC	void CALifeGraphRegistry::iterate					(C &c, const F& f)
{
	c.update					(f);
}
