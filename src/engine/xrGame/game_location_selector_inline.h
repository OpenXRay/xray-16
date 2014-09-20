////////////////////////////////////////////////////////////////////////////
//	Module 		: game_location_selector_inline.h
//	Created 	: 02.10.2001
//  Modified 	: 18.11.2003
//	Author		: Dmitriy Iassenev
//	Description : Game location selector inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION template <\
	typename _VertexEvaluator,\
	typename _vertex_id_type\
>

#define CGameLocationSelector CBaseLocationSelector<CGameGraph,_VertexEvaluator,_vertex_id_type>

TEMPLATE_SPECIALIZATION
IC	CGameLocationSelector::CBaseLocationSelector	(CRestrictedObject *object, CLocationManager *location_manager) :
	inherited	(object)
{
	m_location_manager				= location_manager;
	VERIFY							(location_manager);
}

TEMPLATE_SPECIALIZATION
IC	CGameLocationSelector::~CBaseLocationSelector	()
{
}

TEMPLATE_SPECIALIZATION
IC	void CGameLocationSelector::set_selection_type	(const ESelectionType selection_type)
{
	m_selection_type	= selection_type;
}

TEMPLATE_SPECIALIZATION
IC	void CGameLocationSelector::reinit			(const CGameGraph *graph)
{
	inherited::reinit				(graph);
	m_selection_type				= eSelectionTypeRandomBranching;
	if (graph)
		graph->set_invalid_vertex	(m_previous_vertex_id);
	else
		m_previous_vertex_id		= GameGraph::_GRAPH_ID(-1);
}

TEMPLATE_SPECIALIZATION
IC	void CGameLocationSelector::select_location	(const _vertex_id_type start_vertex_id, _vertex_id_type &dest_vertex_id)
{
	switch (m_selection_type) {
		case eSelectionTypeMask : {
			if (used())
				perform_search	(start_vertex_id);
			else
				m_failed		= false;
			break;
		}
		case eSelectionTypeRandomBranching : {
			if (m_graph)
				select_random_location(start_vertex_id,dest_vertex_id);
			m_failed			= m_failed && (start_vertex_id == dest_vertex_id);
			break;
		}
		default :				NODEFAULT;
	}
}

TEMPLATE_SPECIALIZATION
IC	void CGameLocationSelector::select_random_location(const _vertex_id_type start_vertex_id, _vertex_id_type &dest_vertex_id)
{
	VERIFY						(m_graph);
	VERIFY						(m_graph->valid_vertex_id(start_vertex_id));

	if (!m_graph->valid_vertex_id(m_previous_vertex_id))
		m_previous_vertex_id	= GameGraph::_GRAPH_ID(start_vertex_id);

	u32							branch_factor = 0;

	const GameGraph::TERRAIN_VECTOR				&vertex_types = m_location_manager->vertex_types();
	GameGraph::TERRAIN_VECTOR::const_iterator	B = vertex_types.begin(), I;
	GameGraph::TERRAIN_VECTOR::const_iterator	E = vertex_types.end();

	_Graph::const_iterator		i,e;
	m_graph->begin				(start_vertex_id,i,e);
	for ( ; i != e; ++i) {
		// * не соответствует предыдещей вершине
		if ((*i).vertex_id() == m_previous_vertex_id)
			continue;

		// * вершина на текущем уровне?
		if ((m_graph->vertex((*i).vertex_id())->level_id() != ai().level_graph().level_id()))
			continue;

		// * accessible
		if (!accessible((*i).vertex_id()))
			continue;

		const u8				*curr_types = m_graph->vertex((*i).vertex_id())->vertex_type();

		// * подходит по маске
		for (I = B; I != E; ++I)
			if (m_graph->mask((*I).tMask,curr_types))
				++branch_factor;
	}

	if (!branch_factor) {
		if ((start_vertex_id != m_previous_vertex_id) && accessible(m_previous_vertex_id))
			dest_vertex_id		= m_previous_vertex_id;
		else
			dest_vertex_id		= start_vertex_id;
	}
	else {
		u32						choice = ::Random.randI(0,branch_factor);
		branch_factor			= 0;
		bool					found = false;
		m_graph->begin			(start_vertex_id,i,e);
		for ( ; i != e; ++i) {
			// * не соответствует предыдещей вершине
			if ((*i).vertex_id() == m_previous_vertex_id)
				continue;

			// * вершина на текущем уровне?
			if ((m_graph->vertex((*i).vertex_id())->level_id() != ai().level_graph().level_id()))
				continue;

			// * accessible 
			if (!accessible((*i).vertex_id()))
				continue;

			const u8			*curr_types = m_graph->vertex((*i).vertex_id())->vertex_type();

			// * подходит по маске
			for (I = B; I != E; ++I)
				if (m_graph->mask((*I).tMask,curr_types)) {
					if (choice != branch_factor) {
						++branch_factor;
						continue;
					}

					dest_vertex_id	= (*i).vertex_id();
					found		= true;
					break;
				}

			if (found)
				break;
		}
	}
	m_previous_vertex_id		= GameGraph::_GRAPH_ID(start_vertex_id);
}

TEMPLATE_SPECIALIZATION
IC	void CGameLocationSelector::selection_type		() const
{
	return				(m_selection_type);
}

TEMPLATE_SPECIALIZATION
IC	bool CGameLocationSelector::actual				(const _vertex_id_type start_vertex_id, bool path_completed)
{
	if (m_selection_type != eSelectionTypeRandomBranching)
		return				(inherited::actual(start_vertex_id,path_completed));
	return					(!path_completed);
}

TEMPLATE_SPECIALIZATION
IC	bool CGameLocationSelector::accessible			(const _vertex_id_type vertex_id) const
{
	return					(m_restricted_object ? m_restricted_object->accessible(m_graph->vertex(vertex_id)->level_vertex_id()) : true);
}

#undef TEMPLATE_SPECIALIZATION
#undef CGameLocationSelector