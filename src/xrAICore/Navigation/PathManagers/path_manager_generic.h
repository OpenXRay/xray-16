////////////////////////////////////////////////////////////////////////////
//	Module 		: path_manager_generic.h
//	Created 	: 21.03.2002
//  Modified 	: 03.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Generic path manager
////////////////////////////////////////////////////////////////////////////

#pragma once

template <typename _Graph, typename _DataStorage, typename _Parameters, typename _dist_type, typename _index_type,
    typename _iteration_type>
class CPathManagerGeneric
{
public:
    const _Graph* graph;
    xr_vector<_index_type>* path;

protected:
    _DataStorage* data_storage;
    _index_type start_node_index;
    _index_type goal_node_index;
    _dist_type max_range;
    _iteration_type max_iteration_count;
    u32 max_visited_node_count;
    const _index_type* best_node_index;

public:
    typedef typename _Graph::const_iterator const_iterator;

    CPathManagerGeneric();
    virtual ~CPathManagerGeneric();
    IC void init();
    IC void setup(const _Graph* graph, _DataStorage* _data_storage, xr_vector<_index_type>* _path,
        const _index_type& _start_node_index, const _index_type& _goal_node_index, const _Parameters& params);
    IC _dist_type evaluate(
        const _index_type& node_index1, const _index_type& node_index2, const const_iterator& i) const;
    IC _dist_type estimate(const _index_type& vertex_id) const;
    IC void init_path();
    template <typename T>
    IC void create_path(T& vertex);
    IC const _index_type& start_node() const;
    IC const _index_type& goal_node() const;
    IC bool is_goal_reached(const _index_type& vertex_id) const;
    IC bool is_limit_reached(const _iteration_type iteration_count) const;
    IC bool is_accessible(const _index_type& vertex_id) const;
    IC bool is_metric_euclidian() const;
    IC void begin(const _index_type& vertex_id, const_iterator& begin, const_iterator& end);
    IC const _index_type get_value(const_iterator& i) const;
    IC void finalize();
    IC const const_iterator& edge(const_iterator& i) const;
};

#include "xrAICore/Navigation/PathManagers/path_manager_generic_inline.h"
