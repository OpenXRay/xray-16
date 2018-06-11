////////////////////////////////////////////////////////////////////////////
//	Module 		: path_manager_game_level.h
//	Created 	: 04.10.2004
//  Modified 	: 04.10.2004
//	Author		: Dmitriy Iassenev
//	Description : Game level path manager
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "xrAICore/Navigation/game_graph.h"
#include "xrAICore/Navigation/PathManagers/path_manager_params_game_level.h"

template <typename _DataStorage, typename _dist_type, typename _index_type, typename _iteration_type>
class CPathManager<CGameGraph, _DataStorage, SGameLevel<_dist_type, _index_type, _iteration_type>, _dist_type,
    _index_type, _iteration_type>
    : public CPathManager<CGameGraph, _DataStorage, SBaseParameters<_dist_type, _index_type, _iteration_type>,
          _dist_type, _index_type, _iteration_type>
{
protected:
    typedef CGameGraph _Graph;
    typedef SGameLevel<_dist_type, _index_type, _iteration_type> _Parameters;
    typedef CPathManager<_Graph, _DataStorage, SBaseParameters<_dist_type, _index_type, _iteration_type>,
        _dist_type, _index_type, _iteration_type>
        inherited;

protected:
    _Parameters* m_evaluator;

public:
    virtual ~CPathManager();
    IC void setup(const _Graph* graph, _DataStorage* _data_storage, xr_vector<_index_type>* _path,
        const _index_type& _start_node_index, const _index_type& _goal_node_index, _Parameters& params);
    IC _dist_type estimate(const _index_type& node_index) const;
    IC bool is_goal_reached(const _index_type& node_index);
    template <typename T>
    IC void create_path(T& vertex);
};

#include "xrAICore/Navigation/PathManagers/path_manager_game_level_inline.h"
