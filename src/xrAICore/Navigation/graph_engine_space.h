////////////////////////////////////////////////////////////////////////////
//  Module      : graph_engine_space.h
//  Created     : 21.03.2002
//  Modified    : 26.11.2003
//  Author      : Dmitriy Iassenev
//  Description : Graph engine
////////////////////////////////////////////////////////////////////////////

#pragma once
#include "xrCore/xrCore.h"

template <typename _condition_type, typename _value_type>
class COperatorConditionAbstract;

template <typename _world_property>
class CConditionState;

template <typename _world_property, typename _edge_value_type>
class COperatorAbstract;

template <typename _dist_type, typename _index_type, typename _iteration_type>
struct SBaseParameters;

template <typename _dist_type, typename _index_type, typename _iteration_type>
struct SFlooder;

template <typename _dist_type, typename _index_type, typename _iteration_type>
struct SStraightLineParams;

template <typename _dist_type, typename _index_type, typename _iteration_type>
struct SNearestVertex;

template <typename _dist_type, typename _index_type, typename _iteration_type>
struct SGameLevel;

template <typename _dist_type, typename _index_type, typename _iteration_type>
struct SGameVertex;

namespace GraphEngineSpace
{
    using _dist_type = float;
    using _index_type = u32;
    using _iteration_type = u32;
    using _solver_dist_type = u16;
    using _solver_condition_type = u32;
    using _solver_value_type = bool;

    struct CSolverConditionValue
    {
        _solver_condition_type m_condition;
        _solver_value_type m_value;

        CSolverConditionValue(const _solver_condition_type &condition, const _solver_value_type &value)
        {
            m_condition = condition;
            m_value = value;
        }

        bool operator==(const _solver_condition_type &condition) const
        { return condition==m_condition; }
    };

    using CSolverConditionStorage = xr_vector<CSolverConditionValue>;

    using CWorldProperty = COperatorConditionAbstract<_solver_condition_type, _solver_value_type>;

    using CWorldState = CConditionState<CWorldProperty>;

    using CWorldOperator = COperatorAbstract<CWorldProperty, _solver_dist_type>;

    using _solver_index_type = CWorldState;
    using _solver_edge_type = u32;

    using CSolverBaseParameters = SBaseParameters<_solver_dist_type, _solver_index_type, _iteration_type>;
    
    using CBaseParameters = SBaseParameters<_dist_type, _index_type, _iteration_type>;

    using CFlooder = SFlooder<_dist_type, _index_type, _iteration_type>;
    
    using CStraightLineParams = SStraightLineParams<_dist_type, _index_type, _iteration_type>;
    
    using CNearestVertexParameters = SNearestVertex<_dist_type, _index_type, _iteration_type>;
    
    using CGameLevelParams = SGameLevel<_dist_type, _index_type, _iteration_type>;
    
    using CGameVertexParams = SGameVertex<_dist_type, _index_type, _iteration_type>;
};
