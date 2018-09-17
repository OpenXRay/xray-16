////////////////////////////////////////////////////////////////////////////
//	Module 		: path_manager_solver_inline.h
//	Created 	: 21.03.2002
//  Modified 	: 03.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Solver path manager inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION                                                                                   \
    template <typename T1, typename T2, typename T3, typename T4, typename T5, bool T6, typename T7, typename T8, \
        typename _DataStorage, typename _Parameters, typename _dist_type, typename _index_type,                   \
        typename _iteration_type>

#define CSolverPathManager                                                                                           \
    CPathManager<CProblemSolver<T1, T2, T3, T4, T5, T6, T7, T8>, _DataStorage, _Parameters, _dist_type, _index_type, \
        _iteration_type>

TEMPLATE_SPECIALIZATION
IC CSolverPathManager::~CPathManager() {}
TEMPLATE_SPECIALIZATION
IC void CSolverPathManager::setup(const _Graph* _graph, _DataStorage* _data_storage, xr_vector<_edge_type>* _path,
    const _index_type& _start_node_index, const _index_type& _goal_node_index, const _Parameters& params)
{
    this->graph = _graph;
    this->data_storage = _data_storage;
    m_edge_path = _path;
    this->start_node_index = _start_node_index;
    this->goal_node_index = _goal_node_index;
    this->max_visited_node_count = params.max_visited_node_count;
    this->max_range = (GraphEngineSpace::_solver_dist_type)params.max_range;
    this->max_iteration_count = params.max_iteration_count;
}

TEMPLATE_SPECIALIZATION
IC bool CSolverPathManager::is_goal_reached(const _index_type& vertex_id) const
{
    return (this->graph->is_goal_reached(vertex_id));
}

TEMPLATE_SPECIALIZATION
IC const _index_type& CSolverPathManager::get_value(const_iterator& i, bool reverse_search) const
{
    return (this->graph->value(*(this->best_node_index), i, reverse_search));
}

TEMPLATE_SPECIALIZATION
IC const typename CSolverPathManager::_edge_type& CSolverPathManager::edge(const_iterator& i) const
{
    return ((*i).m_operator_id);
}

TEMPLATE_SPECIALIZATION
IC _dist_type CSolverPathManager::evaluate(
    const _index_type& node_index1, const _index_type& node_index2, const const_iterator& i) const
{
    VERIFY(this->graph);
    return (this->graph->get_edge_weight(node_index1, node_index2, i));
}

TEMPLATE_SPECIALIZATION
IC _dist_type CSolverPathManager::estimate(const _index_type& vertex_id) const
{
    VERIFY(this->graph);
    //	return					((_dist_type)this->graph->get_edge_weight(vertex_id,start_node_index,m_iterator));
    return (1 * (_dist_type)this->graph->estimate_edge_weight(vertex_id));
    //	return					((_dist_type)0);
}

TEMPLATE_SPECIALIZATION
IC void CSolverPathManager::init_path()
{
    if (m_edge_path)
        m_edge_path->clear();
}

TEMPLATE_SPECIALIZATION
template <typename T>
IC void CSolverPathManager::create_path(T& vertex, _DataStorage& data_storage, bool reverse_order)
{
    VERIFY(this->data_storage);
    if (m_edge_path)
        data_storage.get_edge_path(*m_edge_path, &vertex, reverse_order);
}

TEMPLATE_SPECIALIZATION
template <typename T>
IC void CSolverPathManager::create_path(T& vertex)
{
    VERIFY(this->data_storage);
    if (m_edge_path)
        this->data_storage->get_edge_path(*m_edge_path, &vertex, _Graph::reverse_search);
}

#undef TEMPLATE_SPECIALIZATION
#undef CSolverPathManager
