////////////////////////////////////////////////////////////////////////////
//	Module 		: path_manager_level_flooder_inline.h
//	Created 	: 21.03.2002
//  Modified 	: 03.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Level flooder path manager inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION \
	template <\
		typename _DataStorage,\
		typename _dist_type,\
		typename _index_type,\
		typename _iteration_type\
	>

#define CLevelFlooderPathManager CPathManager<\
	CLevelGraph,\
	_DataStorage,\
	SFlooder<\
		_dist_type,\
		_index_type,\
		_iteration_type\
	>,\
	_dist_type,\
	_index_type,\
	_iteration_type\
>


TEMPLATE_SPECIALIZATION
CLevelFlooderPathManager::~CPathManager				()
{
}

TEMPLATE_SPECIALIZATION
IC	void CLevelFlooderPathManager::setup			(
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
//		graph->unpack_xz		(graph->vertex(_start_node_index),start_position.x,start_position.y);
	graph->unpack_xz		(*graph->vertex(_start_node_index),x0,y0);
	max_range_sqr			= iFloor(_sqr(max_range)/m_sqr_distance_xz + .5f);
	m_cell_dist				= graph->header().cell_size();
}

TEMPLATE_SPECIALIZATION
IC	bool CLevelFlooderPathManager::is_goal_reached	(const _index_type &node_index)
{
	VERIFY					(path);
	path->push_back			(node_index);
	best_node				= graph->vertex(node_index);
//		y1						= (float)(best_node->position().y());
	return					(false);
}

TEMPLATE_SPECIALIZATION
IC	_dist_type CLevelFlooderPathManager::evaluate	(const _index_type &node_index1, const _index_type &node_index2, const _Graph::const_iterator &/**i/**/)
{
	VERIFY					(graph);
	return					(m_cell_dist);
}

TEMPLATE_SPECIALIZATION
IC	_dist_type CLevelFlooderPathManager::estimate	(const _index_type &node_index) const
{
	VERIFY					(graph);
	return					(_dist_type(0));
}

TEMPLATE_SPECIALIZATION
IC	bool CLevelFlooderPathManager::is_accessible	(const _index_type &vertex_id) const
{
	if (!inherited::is_accessible(vertex_id))
		return				(false);
	int						x4,y4;
	graph->unpack_xz		(graph->vertex(vertex_id),x4,y4);
	return					(u32(_sqr(x0 - x4) + _sqr(y0 - y4)) <= max_range_sqr);
}

TEMPLATE_SPECIALIZATION
IC	bool CLevelFlooderPathManager::is_limit_reached	(const _iteration_type iteration_count) const
{
	VERIFY					(data_storage);
	return					(
		(iteration_count >= max_iteration_count)	||
		(data_storage->get_visited_node_count() >= max_visited_node_count)
	);
}

TEMPLATE_SPECIALIZATION
template <typename T>
IC	void CLevelFlooderPathManager::create_path		(T &vertex)
{
}

#undef TEMPLATE_SPECIALIZATION
#undef CLevelFlooderPathManager