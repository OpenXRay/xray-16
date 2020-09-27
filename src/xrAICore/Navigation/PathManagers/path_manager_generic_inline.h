////////////////////////////////////////////////////////////////////////////
//	Module 		: path_manager_generic_inline.h
//	Created 	: 21.03.2002
//  Modified 	: 03.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Generic path manager inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION                                                                                        \
    template <typename _Graph, typename _DataStorage, typename _Parameters, typename _dist_type, typename _index_type, \
        typename _iteration_type>

#define CGenericPathManager \
    CPathManagerGeneric<_Graph, _DataStorage, _Parameters, _dist_type, _index_type, _iteration_type>

TEMPLATE_SPECIALIZATION
IC CGenericPathManager::CPathManagerGeneric()
{
    graph = 0;
    data_storage = 0;
    path = 0;
    max_visited_node_count = 0;
    best_node_index = nullptr;
}

TEMPLATE_SPECIALIZATION
CGenericPathManager::~CPathManagerGeneric() {}
TEMPLATE_SPECIALIZATION
IC void CGenericPathManager::init() {}
TEMPLATE_SPECIALIZATION
IC void CGenericPathManager::setup(const _Graph* _graph, _DataStorage* _data_storage, xr_vector<_index_type>* _path,
    const _index_type& _start_node_index, const _index_type& _goal_node_index, const _Parameters& params)
{
    graph = _graph;
    data_storage = _data_storage;
    path = _path;
    start_node_index = _start_node_index;
    goal_node_index = _goal_node_index;
    max_visited_node_count = params.max_visited_node_count;
    max_range = params.max_range;
    max_iteration_count = params.max_iteration_count;
}

TEMPLATE_SPECIALIZATION
IC _dist_type CGenericPathManager::evaluate(
    const _index_type& node_index1, const _index_type& node_index2, const const_iterator& i) const
{
    VERIFY(graph);
    return (graph->get_edge_weight(node_index1, node_index2, i));
}

TEMPLATE_SPECIALIZATION
IC _dist_type CGenericPathManager::estimate(const _index_type& vertex_id) const
{
    VERIFY(graph);
    return (_dist_type(0));
}

TEMPLATE_SPECIALIZATION
IC void CGenericPathManager::init_path()
{
    if (path)
        path->clear();
}

TEMPLATE_SPECIALIZATION
template <typename T>
IC void CGenericPathManager::create_path(T& vertex)
{
    VERIFY(data_storage);
    //		Msg						("Path
    //[IC=xxx][VNC=%d][BV=%f]",data_storage->get_visited_node_count(),data_storage->get_best().f());
    if (path)
        data_storage->get_node_path(*path, &vertex);
}

TEMPLATE_SPECIALIZATION
IC const _index_type& CGenericPathManager::start_node() const { return (start_node_index); }
TEMPLATE_SPECIALIZATION
IC const _index_type& CGenericPathManager::goal_node() const { return (goal_node_index); }
TEMPLATE_SPECIALIZATION
IC bool CGenericPathManager::is_goal_reached(const _index_type& vertex_id) const
{
    return (vertex_id == goal_node_index);
}

TEMPLATE_SPECIALIZATION
IC bool CGenericPathManager::is_limit_reached(const _iteration_type iteration_count) const
{
    VERIFY(data_storage);
    return ((data_storage->get_best().f() >= max_range) || (iteration_count >= max_iteration_count) ||
        (data_storage->get_visited_node_count() >= max_visited_node_count));
}

TEMPLATE_SPECIALIZATION
IC bool CGenericPathManager::is_accessible(const _index_type& vertex_id) const
{
    VERIFY(graph);
    return (graph->is_accessible(vertex_id));
}

TEMPLATE_SPECIALIZATION
IC bool CGenericPathManager::is_metric_euclidian() const
{
    //#pragma todo("Dima to Dima : implement path manager for non-euclidian heuristics")
    return (true);
}

TEMPLATE_SPECIALIZATION
IC void CGenericPathManager::begin(const _index_type& vertex_id, const_iterator& begin, const_iterator& end)
{
    best_node_index = &vertex_id;
    graph->begin(vertex_id, begin, end);
}

TEMPLATE_SPECIALIZATION
IC const _index_type CGenericPathManager::get_value(const_iterator& i) const
{
    return (graph->value(*best_node_index, i));
}

TEMPLATE_SPECIALIZATION
IC void CGenericPathManager::finalize() {}
TEMPLATE_SPECIALIZATION
IC const typename CGenericPathManager::const_iterator& CGenericPathManager::edge(const_iterator& i) const
{
    return (i);
}

#undef TEMPLATE_SPECIALIZATION
#undef CGenericPathManager
