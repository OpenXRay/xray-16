////////////////////////////////////////////////////////////////////////////
//  Module      : graph_engine.h
//  Created     : 21.03.2002
//  Modified    : 26.11.2003
//  Author      : Dmitriy Iassenev
//  Description : Graph engine
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "xrAICore/Navigation/a_star.h"
#include "xrAICore/Navigation/edge_path.h"
#include "xrAICore/Navigation/vertex_manager_fixed.h"
#include "xrAICore/Navigation/vertex_allocator_fixed.h"
#include "xrAICore/Navigation/data_storage_bucket_list.h"
#ifndef AI_COMPILER
#include "xrAICore/Navigation/vertex_manager_hash_fixed.h"
#include "xrAICore/Navigation/data_storage_binary_heap.h"
#endif
#include "xrAICore/Navigation/PathManagers/path_manager.h"
#include "xrAICore/Navigation/graph_engine_space.h"
#include "xrEngine/profiler.h"
#include "xrAICore/Components/problem_solver.h"
#include "xrAICore/Components/operator_condition.h"
#include "xrAICore/Components/condition_state.h"
#include "xrAICore/Components/operator_abstract.h"

namespace hash_fixed_vertex_manager
{
inline u32 to_u32(const GraphEngineSpace::CWorldState& other) { return other.hash_value(); }
}

using namespace GraphEngineSpace;

class CGraphEngine
{
public:
    // common algorithm
    using CPriorityQueue = CDataStorageBucketList<u32, u32, 8 * 1024, false>;
    using CVertexManager = CVertexManagerFixed<u32, u32, 8>;
#ifdef AI_COMPILER
    static const size_t VertexAllocatorReserve = 2 * 1024 * 1024;
#else
    static const size_t VertexAllocatorReserve = 64 * 1024;
#endif
    using CVertexAllocator = CVertexAllocatorFixed<VertexAllocatorReserve>;
    using AlgorithmStorage = CVertexPath<true>;
    using CAlgorithm = CAStar<_dist_type, CPriorityQueue, CVertexManager, CVertexAllocator, true, AlgorithmStorage>;

#ifndef AI_COMPILER
    // solver algorithm
    static const size_t SolverMaxVertexCount = 16 * 1024;
    using CSolverPriorityQueue = CDataStorageBinaryHeap;
    // XXX: originally CSolverAlgorithm is constructed with 16*1024 limit
    // while the following definitions use 8*1024 limit -- check if that's a mistake
    using CSolverVertexManager = CVertexManagerHashFixed<u32, _solver_index_type, 256, 8 * 1024>;
    using CSolverVertexAllocator = CVertexAllocatorFixed<8 * 1024>;
    using SolverAlgorithmStorage = CEdgePath<_solver_edge_type, true>;
    using CSolverAlgorithm = CAStar<_solver_dist_type, CSolverPriorityQueue, CSolverVertexManager,
        CSolverVertexAllocator, true, SolverAlgorithmStorage>;
    // string algorithm
    static const size_t StringMaxVertexCount = 1024;
    using CStringPriorityQueue = CDataStorageBinaryHeap;
    using CStringVertexManager = CVertexManagerHashFixed<u32, shared_str, 128, StringMaxVertexCount>;
    using CStringVertexAllocator = CVertexAllocatorFixed<StringMaxVertexCount>;
    using StringAlgorithmStorage = AlgorithmStorage;
    using StringAlgorithmDistance = float;
    using CStringAlgorithm = CAStar<StringAlgorithmDistance, CStringPriorityQueue, CStringVertexManager,
        CStringVertexAllocator, true, StringAlgorithmStorage>;
#endif // AI_COMPILER

    CAlgorithm* m_algorithm;

#ifndef AI_COMPILER
    CSolverAlgorithm* m_solver_algorithm;
    CStringAlgorithm* m_string_algorithm;
#endif
    CStatTimer PathTimer;

public:
    inline CGraphEngine(u32 max_vertex_count);
    virtual ~CGraphEngine();
#ifndef AI_COMPILER
    inline const CSolverAlgorithm& solver_algorithm() const;
#endif

    template <typename _Graph, typename _Parameters>
    inline bool search(const _Graph& graph, const shared_str& start_node, const shared_str& dest_node,
        xr_vector<shared_str>* node_path, _Parameters& parameters);

    template <typename _Graph, typename _Parameters>
    inline bool search(const _Graph& graph, const _index_type& start_node, const _index_type& dest_node,
        xr_vector<_index_type>* node_path, const _Parameters& parameters);

    template <typename _Graph, typename _Parameters>
    inline bool search(const _Graph& graph, const _index_type& start_node, const _index_type& dest_node,
        xr_vector<_index_type>* node_path, _Parameters& parameters);

    template <typename _Graph, typename _Parameters, typename _PathManager>
    inline bool search(const _Graph& graph, const _index_type& start_node, const _index_type& dest_node,
        xr_vector<_index_type>* node_path, const _Parameters& parameters, _PathManager& path_manager);

#ifndef AI_COMPILER
    template <typename T1, typename T2, typename T3, typename T4, typename T5, bool T6, typename T7, typename T8,
        typename _Parameters>
    inline bool search(const CProblemSolver<T1, T2, T3, T4, T5, T6, T7, T8>& graph,
        const _solver_index_type& start_node, const _solver_index_type& dest_node,
        xr_vector<_solver_edge_type>* node_path, const _Parameters& parameters);
#endif
};

#include "xrAICore/Navigation/graph_engine_inline.h"
