////////////////////////////////////////////////////////////////////////////
//	Module 		: path_manager_level_nearest_vertex_inline.h
//	Created 	: 21.03.2002
//  Modified 	: 03.03.2004
//	Author		: Dmitriy Iassenev
//	Description : path manager level nearest vertex inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION \
    template <typename _DataStorage, typename _dist_type, typename _index_type, typename _iteration_type>

#define CNearestVertexPathManager                                                                                 \
    CPathManager<CLevelGraph, _DataStorage, SNearestVertex<_dist_type, _index_type, _iteration_type>, _dist_type, \
        _index_type, _iteration_type\
>

TEMPLATE_SPECIALIZATION
CNearestVertexPathManager::~CPathManager() {}
TEMPLATE_SPECIALIZATION
IC void CNearestVertexPathManager::setup(const _Graph* _graph, _DataStorage* _data_storage,
    xr_vector<_index_type>* _path, const _index_type& _start_node_index, const _index_type& _goal_node_index,
    const _Parameters& parameters)
{
    inherited::setup(_graph, _data_storage, _path, _start_node_index, _goal_node_index, parameters);

    this->graph->unpack_xz(*(this->graph->vertex(_start_node_index)), x0, y0);
    max_range_sqr = iFloor(_sqr(this->max_range) / this->m_sqr_distance_xz + .5f);
    m_cell_dist = this->graph->header().cell_size();

    m_target_position = parameters.m_target_position;
    m_best_distance_to_target = flt_max;
    VERIFY(this->path);
    this->path->clear();
}

TEMPLATE_SPECIALIZATION
IC bool CNearestVertexPathManager::is_goal_reached(const _index_type& node_index)
{
    VERIFY(this->path);
    this->best_node = this->graph->vertex(node_index);

    const float current_distance = m_target_position.distance_to_xz_sqr(this->graph->vertex_position(this->best_node));
    if (current_distance < m_best_distance_to_target)
    {
        m_best_distance_to_target = current_distance;
        this->path->clear();
        this->path->push_back(node_index);
    }

    return (false);
}

TEMPLATE_SPECIALIZATION
IC _dist_type CNearestVertexPathManager::evaluate(
    const _index_type& node_index1, const _index_type& node_index2, const _Graph::const_iterator& /**i**/)
{
    VERIFY(this->graph);
    return (m_cell_dist);
}

TEMPLATE_SPECIALIZATION
IC _dist_type CNearestVertexPathManager::estimate(const _index_type& node_index) const
{
    VERIFY(this->graph);
    return (_dist_type(0));
}

TEMPLATE_SPECIALIZATION
IC bool CNearestVertexPathManager::is_accessible(const _index_type& vertex_id) const
{
    if (!inherited::is_accessible(vertex_id))
        return (false);

    int x4, y4;
    this->graph->unpack_xz(this->graph->vertex(vertex_id), x4, y4);
    return (u32(_sqr(x0 - x4) + _sqr(y0 - y4)) <= max_range_sqr);
}

TEMPLATE_SPECIALIZATION
IC bool CNearestVertexPathManager::is_limit_reached(const _iteration_type iteration_count) const
{
    VERIFY(this->data_storage);
    return (
        (iteration_count >= this->max_iteration_count) || (this->data_storage->get_visited_node_count() >= this->max_visited_node_count));
}

TEMPLATE_SPECIALIZATION
template <typename T>
IC void CNearestVertexPathManager::create_path(T& vertex)
{
}

#undef TEMPLATE_SPECIALIZATION
#undef CNearestVertexPathManager
