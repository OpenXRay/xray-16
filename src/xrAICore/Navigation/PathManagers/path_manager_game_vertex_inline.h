////////////////////////////////////////////////////////////////////////////
//	Module 		: path_manager_game_vertex_inline.h
//	Created 	: 05.07.2006
//  Modified 	: 05.07.2006
//	Author		: Dmitriy Iassenev
//	Description : Game vertex path manager inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION \
	template <\
		typename _DataStorage,\
		typename _dist_type,\
		typename _index_type,\
		typename _iteration_type\
	>

#define CGameVertexPathManager CPathManager<\
	CGameGraph,\
	_DataStorage,\
	SGameVertex<\
		_dist_type,\
		_index_type,\
		_iteration_type\
	>,\
	_dist_type,\
	_index_type,\
	_iteration_type\
>

TEMPLATE_SPECIALIZATION
IC	void CGameVertexPathManager::setup			(
			const _Graph			*_graph,
			_DataStorage			*_data_storage,
			xr_vector<_index_type>	*_path,
			const _index_type		&_start_node_index,
			const _index_type		&_goal_node_index,
			_Parameters				&parameters
		)
{
	inherited::setup(
		_graph,
		_data_storage,
		_path,
		_start_node_index,
		_goal_node_index,
		parameters
	);
	m_evaluator					= &parameters;
	m_evaluator->m_vertex_id	= _index_type(-1);
	m_start_is_accessible		= is_accessible(_start_node_index);
}

TEMPLATE_SPECIALIZATION
IC	bool CGameVertexPathManager::is_accessible	(const _index_type &vertex_id) const
{
	if (!inherited::is_accessible(vertex_id))
		return				(false);

	if (!m_start_is_accessible)
		return				(true);

	typedef _Parameters::VERTEX_TYPES::const_iterator	const_iterator;
#ifdef DEBUG
	if (m_evaluator->m_vertex_types->empty()) {
		Msg					("! warning : empty vertex types");
	}
#endif
	const_iterator			I = m_evaluator->m_vertex_types->begin();
	const_iterator			E = m_evaluator->m_vertex_types->end();
	for ( ; I != E; ++I)
		if (graph->mask((*I).tMask,graph->vertex(vertex_id)->vertex_type()))
			return			(true);

	return					(false);
}

#undef TEMPLATE_SPECIALIZATION
#undef CGameVertexTypePathManager