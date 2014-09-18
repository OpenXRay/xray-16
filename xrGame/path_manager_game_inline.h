////////////////////////////////////////////////////////////////////////////
//	Module 		: path_manager_game_inline.h
//	Created 	: 21.03.2002
//  Modified 	: 03.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Game path manager inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION \
	template <\
		typename _DataStorage,\
		typename _Parameters,\
		typename _dist_type,\
		typename _index_type,\
		typename _iteration_type\
	>

#define CGamePathManager CPathManager<CGameGraph,_DataStorage,_Parameters,_dist_type,_index_type,_iteration_type>

TEMPLATE_SPECIALIZATION
CGamePathManager::~CPathManager				()
{
}

TEMPLATE_SPECIALIZATION
IC	void CGamePathManager::setup			(
		const _Graph			*_graph,
		_DataStorage			*_data_storage,
		xr_vector<_index_type>	*_path,
		const _index_type		&_start_node_index,
		const _index_type		&_goal_node_index,
		const _Parameters		&parameters
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
	goal_vertex				= graph->vertex(goal_node_index);
}

TEMPLATE_SPECIALIZATION
IC	_dist_type CGamePathManager::evaluate	(const _index_type &node_index1, const _index_type &node_index2, const _Graph::const_iterator &i) const
{
	VERIFY					(graph);
	return					((*i).distance());
}

TEMPLATE_SPECIALIZATION
IC	_dist_type CGamePathManager::estimate	(const _index_type &node_index) const
{
	VERIFY					(graph);
	return					(goal_vertex->game_point().distance_to(graph->vertex(node_index)->game_point()));
}

TEMPLATE_SPECIALIZATION
IC	bool CGamePathManager::is_limit_reached	(const _iteration_type iteration_count) const
{
	VERIFY					(data_storage);
	return					(false);
}

TEMPLATE_SPECIALIZATION
IC	bool CGamePathManager::is_accessible	(const _index_type &vertex_id) const
{
	VERIFY					(graph);
	return					(graph->accessible(vertex_id));
}

#undef TEMPLATE_SPECIALIZATION
#undef CGamePathManager