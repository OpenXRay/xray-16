////////////////////////////////////////////////////////////////////////////
//  Module      : graph_engine_inline.h
//  Created     : 21.03.2002
//  Modified    : 03.03.2004
//  Author      : Dmitriy Iassenev
//  Description : Graph engine inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once
#include "xrCore/Threading/ScopeLock.hpp"

inline CGraphEngine::CGraphEngine(u32 max_vertex_count)
{
    m_algorithm = new CAlgorithm(max_vertex_count);
    m_algorithm->data_storage().set_min_bucket_value(_dist_type(0));
    m_algorithm->data_storage().set_max_bucket_value(_dist_type(2000));
#ifndef AI_COMPILER
    m_solver_algorithm = new CSolverAlgorithm(SolverMaxVertexCount);
    m_string_algorithm = new CStringAlgorithm(StringMaxVertexCount);
#endif
}

inline CGraphEngine::~CGraphEngine()
{
    xr_delete(m_algorithm);
#ifndef AI_COMPILER
    xr_delete(m_solver_algorithm);
    xr_delete(m_string_algorithm);
#endif
}

#ifndef AI_COMPILER
inline const CGraphEngine::CSolverAlgorithm& CGraphEngine::solver_algorithm() const { return *m_solver_algorithm; }
#endif

template <typename _Graph, typename _Parameters>
inline bool CGraphEngine::search(const _Graph& graph, const _index_type& start_node, const _index_type& dest_node,
    xr_vector<_index_type>* node_path, const _Parameters& parameters)
{
    //ScopeLock scope(&m_lock);
    START_PROFILE("graph_engine")
    START_PROFILE("graph_engine/search")
    PathTimer.Begin();
    using CPathManagerGeneric =
        CPathManager<_Graph, CAlgorithm::CDataStorage, _Parameters, _dist_type, _index_type, _iteration_type>;
    CPathManagerGeneric path_manager;
    path_manager.setup(&graph, &m_algorithm->data_storage(), node_path, start_node, dest_node, parameters);
    bool successfull = m_algorithm->find(path_manager);
    PathTimer.End();
    return successfull;
    STOP_PROFILE
    STOP_PROFILE
}

template <typename _Graph, typename _Parameters>
inline bool CGraphEngine::search(const _Graph& graph, const _index_type& start_node, const _index_type& dest_node,
    xr_vector<_index_type>* node_path, _Parameters& parameters)
{
    //ScopeLock scope(&m_lock);
    START_PROFILE("graph_engine")
    START_PROFILE("graph_engine/search")
    PathTimer.Begin();
    using CPathManagerGeneric =
        CPathManager<_Graph, CAlgorithm::CDataStorage, _Parameters, _dist_type, _index_type, _iteration_type>;
    CPathManagerGeneric path_manager;
    path_manager.setup(&graph, &m_algorithm->data_storage(), node_path, start_node, dest_node, parameters);
    bool successfull = m_algorithm->find(path_manager);
    PathTimer.End();
    return successfull;
    STOP_PROFILE
    STOP_PROFILE
}

template <typename _Graph, typename _Parameters, typename _PathManager>
inline bool CGraphEngine::search(const _Graph& graph, const _index_type& start_node, const _index_type& dest_node,
    xr_vector<_index_type>* node_path, const _Parameters& parameters, _PathManager& path_manager)
{
    //ScopeLock scope(&m_lock);
    START_PROFILE("graph_engine")
    START_PROFILE("graph_engine/search")
    PathTimer.Begin();
    path_manager.setup(&graph, &m_algorithm->data_storage(), node_path, start_node, dest_node, parameters);
    bool successfull = m_algorithm->find(path_manager);
    PathTimer.End();
    return successfull;
    STOP_PROFILE
    STOP_PROFILE
}

#ifndef AI_COMPILER
template <typename T1, typename T2, typename T3, typename T4, typename T5, bool T6, typename T7, typename T8,
    typename _Parameters>
inline bool CGraphEngine::search(const CProblemSolver<T1, T2, T3, T4, T5, T6, T7, T8>& graph,
    const _solver_index_type& start_node, const _solver_index_type& dest_node, xr_vector<_solver_edge_type>* node_path,
    const _Parameters& parameters)
{
    //ScopeLock scope(&m_lock);
    START_PROFILE("graph_engine")
    START_PROFILE("graph_engine/problem_solver")
    PathTimer.Begin();
    using CSProblemSolver = CProblemSolver<T1, T2, T3, T4, T5, T6, T7, T8>;
    using CSolverPathManager = CPathManager<CSProblemSolver, CSolverAlgorithm::CDataStorage, _Parameters,
        _solver_dist_type, _solver_index_type, GraphEngineSpace::_iteration_type>;
    CSolverPathManager path_manager;
    path_manager.setup(&graph, &m_solver_algorithm->data_storage(), node_path, start_node, dest_node, parameters);
    bool successfull = m_solver_algorithm->find(path_manager);
    PathTimer.End();
    return successfull;
    STOP_PROFILE
    STOP_PROFILE
}

template <typename _Graph, typename _Parameters>
inline bool CGraphEngine::search(const _Graph& graph, const shared_str& start_node, const shared_str& dest_node,
    xr_vector<shared_str>* node_path, _Parameters& parameters)
{
    //ScopeLock scope(&m_lock);
    START_PROFILE("graph_engine")
    START_PROFILE("graph_engine/search")
    PathTimer.Begin();
    using CPathManagerGeneric =
        CPathManager<_Graph, CStringAlgorithm::CDataStorage, _Parameters, float, shared_str, u32>;
    CPathManagerGeneric path_manager;
    path_manager.setup(&graph, &m_string_algorithm->data_storage(), node_path, start_node, dest_node, parameters);
    bool successfull = m_string_algorithm->find(path_manager);
    PathTimer.End();
    return successfull;
    STOP_PROFILE
    STOP_PROFILE
}

#endif // AI_COMPILER
