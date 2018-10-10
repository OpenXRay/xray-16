////////////////////////////////////////////////////////////////////////////
//	Module 		: path_manager_game.h
//	Created 	: 21.03.2002
//  Modified 	: 03.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Game path manager
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "xrAICore/Navigation/game_graph.h"

template <typename _DataStorage, typename _Parameters, typename _dist_type, typename _index_type,
    typename _iteration_type>
class CPathManager<CGameGraph, _DataStorage, _Parameters, _dist_type, _index_type, _iteration_type>
    : public CPathManagerGeneric<CGameGraph, _DataStorage, _Parameters, _dist_type, _index_type, _iteration_type>
{
protected:
    typedef CGameGraph _Graph;
    typedef CPathManagerGeneric<_Graph, _DataStorage, _Parameters, _dist_type, _index_type, _iteration_type>
        inherited;

protected:
    const _Graph::CVertex* goal_vertex;

public:
    virtual ~CPathManager();
    IC void setup(const _Graph* graph, _DataStorage* _data_storage, xr_vector<_index_type>* _path,
        const _index_type& _start_node_index, const _index_type& _goal_node_index, const _Parameters& params);
    IC _dist_type evaluate(
        const _index_type& node_index1, const _index_type& node_index2, const _Graph::const_iterator& i) const;
    IC _dist_type estimate(const _index_type& node_index) const;
    IC bool is_limit_reached(const _iteration_type iteration_count) const;
    IC bool is_accessible(const _index_type& vertex_id) const;
};

#include "xrAICore/Navigation/PathManagers/path_manager_game_inline.h"
