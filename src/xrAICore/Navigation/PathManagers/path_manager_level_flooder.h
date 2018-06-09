////////////////////////////////////////////////////////////////////////////
//	Module 		: path_manager_level_flooder.h
//	Created 	: 21.03.2002
//  Modified 	: 03.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Level flooder path manager
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "xrAICore/Navigation/PathManagers/path_manager_level.h"

template <typename _DataStorage, typename _dist_type, typename _index_type, typename _iteration_type>
class CPathManager<CLevelGraph, _DataStorage, SFlooder<_dist_type, _index_type, _iteration_type>, _dist_type,
    _index_type, _iteration_type>
    : public CPathManager<CLevelGraph, _DataStorage, SBaseParameters<_dist_type, _index_type, _iteration_type>,
          _dist_type, _index_type, _iteration_type>
{
protected:
    typedef CLevelGraph _Graph;
    typedef SFlooder<_dist_type, _index_type, _iteration_type> _Parameters;
    typedef CPathManager<_Graph, _DataStorage, SBaseParameters<_dist_type, _index_type, _iteration_type>,
        _dist_type, _index_type, _iteration_type>
        inherited;

protected:
    int x0, y0;
    u32 max_range_sqr;
    float m_cell_dist;

public:
    virtual ~CPathManager();
    IC void setup(const _Graph* graph, _DataStorage* _data_storage, xr_vector<_index_type>* _path,
        const _index_type& _start_node_index, const _index_type& _goal_node_index, const _Parameters& params);
    IC bool is_goal_reached(const _index_type& node_index);
    IC _dist_type evaluate(
        const _index_type& node_index1, const _index_type& node_index2, const _Graph::const_iterator& i);
    IC _dist_type estimate(const _index_type& node_index) const;
    IC bool is_accessible(const _index_type& vertex_id) const;
    IC bool is_limit_reached(const _iteration_type iteration_count) const;
    template <typename T>
    IC void create_path(T& vertex);
};

#include "xrAICore/Navigation/PathManagers/path_manager_level_flooder_inline.h"
