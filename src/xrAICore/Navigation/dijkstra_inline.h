////////////////////////////////////////////////////////////////////////////
//  Module      : dijkstra.h
//  Created     : 21.03.2002
//  Modified    : 02.03.2004
//  Author      : Dmitriy Iassenev
//  Description : Implementation of the Dijkstra algorithm
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION                                                                                \
    template <typename TDistance, typename TPriorityQueue, typename TVertexManager, typename TVertexAllocator, \
        bool EuclidianHeuristics, typename TPathBuilder, typename TIteration, typename TVertexData>

#define CSDijkstra                                                                                            \
    CDijkstra<TDistance, TPriorityQueue, TVertexManager, TVertexAllocator, EuclidianHeuristics, TPathBuilder, \
        TIteration, TVertexData>

TEMPLATE_SPECIALIZATION
inline CSDijkstra::CDijkstra(const u32 max_vertex_count)
{
    m_data_storage = xr_new<CDataStorage>(max_vertex_count);
    m_search_started = false;
}

TEMPLATE_SPECIALIZATION
inline CSDijkstra::~CDijkstra() { xr_delete(m_data_storage); }
TEMPLATE_SPECIALIZATION
inline typename CSDijkstra::CDataStorage& CSDijkstra::data_storage() { return *m_data_storage; }
TEMPLATE_SPECIALIZATION
inline const typename CSDijkstra::CDataStorage& CSDijkstra::data_storage() const { return *m_data_storage; }
TEMPLATE_SPECIALIZATION
template <typename TPathManager>
inline void CSDijkstra::initialize(TPathManager& path_manager)
{
    THROW2(!m_search_started, "Recursive graph engine usage is not allowed!");
    m_search_started = true;
    // initialize data structures before we started path search
    data_storage().init();
    // initialize path manager before we started path search
    path_manager.init();
    // create a node
    Vertex& start = data_storage().create_vertex(path_manager.start_node());
    // assign correspoding values to the created node
    start.f() = Distance(0);
    // assign null parent to the start node
    data_storage().assign_parent(start, 0);
    // add start node to the opened list
    data_storage().add_opened(start);
}

TEMPLATE_SPECIALIZATION
template <typename TPathManager>
inline void CSDijkstra::finalize(TPathManager& path_manager)
{
    // finalize path manager after we finished path search
    path_manager.finalize();
    m_search_started = false;
}

TEMPLATE_SPECIALIZATION
template <typename TPathManager>
inline bool CSDijkstra::step(TPathManager& path_manager)
{
    // get the best node, i.e. a node with the minimum 'f'
    Vertex& best = data_storage().get_best();
    // check if this node is the one we are searching for
    if (path_manager.is_goal_reached(best.index()))
    {
        // we reached the goal, so we have to create a path
        path_manager.init_path();
        path_manager.create_path(best);
        // and return success
        return true;
    }
    // put best node to the closed list
    data_storage().add_best_closed();
    // and remove this node from the opened one
    data_storage().remove_best_opened();
    // iterating on the best node neighbours
    typename TPathManager::const_iterator i, e;
    path_manager.begin(best.index(), i, e);
    for (; i != e; ++i)
    {
        const Index& neighbour_index = path_manager.get_value(i);
        // check if neighbour is accessible
        if (!path_manager.is_accessible(neighbour_index))
            continue;
        // check if neighbour is visited, i.e. is in the opened or
        // closed lists
        if (data_storage().is_visited(neighbour_index))
        {
            // so, this neighbour node has been already visited
            // therefore get the pointer to this node
            Vertex& neighbour = data_storage().get_node(neighbour_index);
            // check if this node is in the opened list
            if (data_storage().is_opened(neighbour))
            {
                // compute 'g' for the node
                Distance f = best.f() + path_manager.evaluate(best.index(), neighbour_index, i);
                // check if new path is better than the older one
                if (neighbour.f() > f)
                {
                    // so, new path is better
                    // assign corresponding values to the node
                    Distance d = neighbour.f();
                    neighbour.f() = f;
                    // assign correct parent to the node to be able
                    // to retreive a path
                    data_storage().assign_parent(neighbour, &best, path_manager.edge(i));
                    // notify data storage about node decreasing value
                    data_storage().decrease_opened(neighbour, d);
                    // continue iterating on neighbours
                    continue;
                }
                // so, new path is worse
                // continue iterating on neighbours
                continue;
            }
            // continue iterating on neighbours
            continue;
        }
        else
        {
            // so, this neighbour node is not in the opened or closed lists
            // put neighbour node to the opened list
            Vertex& neighbour = data_storage().create_vertex(neighbour_index);
            // fill the corresponding node parameters
            neighbour.f() = best.f() + path_manager.evaluate(best.index(), neighbour_index, i);
            // assign best node as its parent
            data_storage().assign_parent(neighbour, &best, path_manager.edge(i));
            // add start node to the opened list
            data_storage().add_opened(neighbour);
            // continue iterating on neighbours
            continue;
        }
    }
    // this iteration haven't got the goal node, therefore return failure
    return false;
}

TEMPLATE_SPECIALIZATION
template <typename TPathManager>
inline bool CSDijkstra::find(TPathManager& path_manager)
{
    // initialize data structures with new search
    initialize(path_manager);
    // iterate while opened list is not empty
    for (TIteration i = TIteration(0); !data_storage().is_opened_empty(); ++i)
    {
        // check if we reached limit
        if (path_manager.is_limit_reached(i))
        {
            // so we reached limit, return failure
            finalize(path_manager);
            return false;
        }
        // so, limit is not reached
        // check if new step will get us success
        if (step(path_manager))
        {
            // so this step reached the goal, return success
            finalize(path_manager);
            return true;
        }
    }
    // so, opened list is empty, return failure
    finalize(path_manager);
    return false;
}

#undef TEMPLATE_SPECIALIZATION
#undef CSDijkstra
