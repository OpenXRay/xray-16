////////////////////////////////////////////////////////////////////////////
//	Module 		: path_manager_level_evaluator_inline.h
//	Created 	: 21.03.2002
//  Modified 	: 03.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Level evaluator path manager inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION \
	template <\
		typename _DataStorage,\
		typename _dist_type,\
		typename _index_type,\
		typename _iteration_type\
	>

#define CLevelEvaluatorPathManager CPathManager<\
	CLevelGraph,\
	_DataStorage,\
	CAbstractVertexEvaluator,\
	_dist_type,\
	_index_type,\
	_iteration_type\
>

TEMPLATE_SPECIALIZATION
CLevelEvaluatorPathManager::~CPathManager			()
{
}

TEMPLATE_SPECIALIZATION
IC	void CLevelEvaluatorPathManager::setup		(
		const _Graph			*_graph,
		_DataStorage			*_data_storage,
		xr_vector<_index_type>	*_path,
		const _index_type		&_start_node_index,
		const _index_type		&_goal_node_index,
		_Parameters				&parameters
	)
{
	m_evaluator				= &parameters;
	m_evaluator->max_range	= parameters.m_fSearchRange;
	inherited::setup(
		_graph,
		_data_storage,
		_path,
		_start_node_index,
		_goal_node_index,
		parameters
	);
	m_evaluator->m_fBestCost= flt_max;
	path					= parameters.m_path;
	graph->set_invalid_vertex(m_evaluator->m_dwBestNode);
}

TEMPLATE_SPECIALIZATION
IC	_dist_type CLevelEvaluatorPathManager::estimate	(const _index_type &node_index) const
{
	return					(_dist_type(0));
}

TEMPLATE_SPECIALIZATION
IC	bool CLevelEvaluatorPathManager::is_goal_reached(const _index_type &node_index)
{
	VERIFY					(m_evaluator);
	m_evaluator->m_tpCurrentNode = graph->vertex(node_index);
	m_evaluator->m_fDistance	 = data_storage->get_best().g();
	float					value = m_evaluator->ffEvaluate();
	if (value < m_evaluator->m_fBestCost) {
		m_evaluator->m_fBestCost	= value;
		m_evaluator->m_dwBestNode	= node_index;
	}

	best_node				= graph->vertex(node_index);
//	y1						= (float)(best_node->position().y());

	return					(false);
}

TEMPLATE_SPECIALIZATION
IC	void CLevelEvaluatorPathManager::finalize		()
{
	if (path && graph->valid_vertex_id(m_evaluator->m_dwBestNode))
		data_storage->get_node_path(*path,&data_storage->get_node(m_evaluator->m_dwBestNode));
}

#undef TEMPLATE_SPECIALIZATION
#undef CLevelEvaluatorPathManager