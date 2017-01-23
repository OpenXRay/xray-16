////////////////////////////////////////////////////////////////////////////
//	Module 		: path_manager_game_vertex_type_inline.h
//	Created 	: 21.03.2002
//  Modified 	: 04.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Game vertex type path manager inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION \
	template <\
		typename _DataStorage,\
		typename _dist_type,\
		typename _index_type,\
		typename _iteration_type\
	>

#define CGameVertexTypePathManager CPathManager<\
	CGameGraph,\
	_DataStorage,\
	SGameLevel<\
		_dist_type,\
		_index_type,\
		_iteration_type\
	>,\
	_dist_type,\
	_index_type,\
	_iteration_type\
>

TEMPLATE_SPECIALIZATION
CGameVertexTypePathManager::~CPathManager			()
{
}

TEMPLATE_SPECIALIZATION
IC	void CGameVertexTypePathManager::setup			(
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
	m_evaluator		= &parameters;
	m_evaluator->m_vertex_id = _index_type(-1);
}

TEMPLATE_SPECIALIZATION
IC	_dist_type CGameVertexTypePathManager::estimate	(const _index_type &node_index) const
{
	return					(_dist_type(0));
}

TEMPLATE_SPECIALIZATION
IC	bool CGameVertexTypePathManager::is_goal_reached(const _index_type &node_index)
{
	VERIFY					(m_evaluator);
	if (graph->vertex(data_storage->get_best().index())->level_id() == m_evaluator->m_level_id) {
		m_evaluator->m_vertex_id	= data_storage->get_best().index();
		return				(true);
	}
	return					(false);
}

TEMPLATE_SPECIALIZATION
template <typename T>
IC	void CGameVertexTypePathManager::create_path	(T &vertex)
{
	if (path)
		inherited::create_path(vertex);
}

#undef TEMPLATE_SPECIALIZATION
#undef CGameVertexTypePathManager