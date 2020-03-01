////////////////////////////////////////////////////////////////////////////
//	Module 		: verify_level_graph.cpp
//	Created 	: 25.05.2004
//  Modified 	: 25.05.2004
//	Author		: Dmitriy Iassenev
//	Description : Verifying level graph
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "xrAICore/Navigation/level_graph.h"

#define PUSH(a) *stack_iterator++ = (a)
#define POP() vertex = *--stack_iterator

void floodfill(const CLevelGraph& level_graph, CLevelGraph::CLevelVertex** stack_storage, xr_vector<bool>& marks, const u32 start_vertex_id)
{
    CLevelGraph::CLevelVertex** stack_iterator = stack_storage;
    CLevelGraph::CLevelVertex* vertex = nullptr;
    PUSH(level_graph.vertex(start_vertex_id));
    while (stack_iterator != stack_storage)
    {
        POP();
        auto vertex_id = level_graph.vertex_id(vertex);
        if (marks[vertex_id])
            continue;
        marks[vertex_id] = true;
        for (const auto j : { 0, 1, 2, 3 })
        {
            auto neighbour_vertex_id = vertex->link(j);
            if (level_graph.valid_vertex_id(neighbour_vertex_id) && !marks[neighbour_vertex_id])
                PUSH(level_graph.vertex(neighbour_vertex_id));
        }
    }
}

bool verify_invalid_links(const CLevelGraph& graph)
{
    bool result = true;
    for (const auto &vertex : graph)
    {
        auto vertex_id = graph.vertex_id(&vertex);
        for (const auto j : { 0, 1, 2, 3 })
        {
            auto link_vertex_id = vertex.link(j);
            if (!graph.valid_vertex_id(link_vertex_id))
                continue;

            if (vertex_id == link_vertex_id)
            {
                Msg("Vertex %d[%f][%f][%f] has link to itself", vertex_id, VPUSH(graph.vertex_position(vertex)));
                result = false;
                continue;
            }
        }
    }
    return result;
}

void verify_level_graph(LPCSTR name, bool verbose)
{
    Msg("Verifying level %s", name);
    Logger.Phase("Verifying level graph");
    Logger.Progress(0.f);
    CLevelGraph level_graph(name);

    if (!level_graph.header().vertex_count())
    {
        Logger.Progress(1.f);
        Msg("Level graph is empty!");
        return;
    }

    if (!verify_invalid_links(level_graph))
    {
        Logger.Progress(1.f);
        Msg("AI map is CORRUPTED : REGENERATE AI-MAP");
        return;
    }

    xr_vector<u32> single_links;
    single_links.reserve(level_graph.header().vertex_count());
    Logger.Progress(0.05f);

    for (const auto &vertex : level_graph)
    {
        for (const auto j : {0, 1, 2, 3})
        {
            auto neighbour_vertex_id = vertex.link(j);
            if (level_graph.valid_vertex_id(neighbour_vertex_id) && // Valid vertex
                level_graph.vertex(neighbour_vertex_id)->link((j + 2) % 4) != level_graph.vertex_id(&vertex)) // Single vertex
            {
                single_links.push_back(neighbour_vertex_id);
            }
        }
        Logger.Progress(0.05f + 0.05f * float(level_graph.vertex_id(&vertex)) / float(level_graph.header().vertex_count()));
    }

    bool no_single_links = single_links.empty();
    Logger.Progress(0.1f);
    if (single_links.empty())
        single_links.push_back(0);

    std::sort(single_links.begin(), single_links.end());
    auto I = std::unique(single_links.begin(), single_links.end());
    single_links.erase(I, single_links.end());

    if (!no_single_links)
    {
        if (verbose)
            for (const auto &i : single_links)
                Msg("Vertex %d[%f][%f][%f] is single linked!", i, VPUSH(level_graph.vertex_position(i)));
        Msg("There are %d single linked nodes!", single_links.size());
    }

    Logger.Progress(0.15f);
    CLevelGraph::CLevelVertex** stack_storage = (CLevelGraph::CLevelVertex**)xr_malloc(level_graph.header().vertex_count() * sizeof(CLevelGraph::CLevelVertex*));
    xr_vector<bool> marks;
    bool valid = true;
    for (auto &i : single_links)
    {
        marks.assign(level_graph.header().vertex_count(), false);
        floodfill(level_graph, stack_storage, marks, i);
        for (auto &j : marks)
        {
            if (!j)
            {
                valid = false;
                auto J = std::distance(&marks.front(), &j);
                Msg("AI-map is NOT valid :\nNode \n%6d[%f][%f][%f]\ncannot be reached from the node\n%6d[%f][%f][%f]\n",
                    J, VPUSH(level_graph.vertex_position(J)),
                    i, VPUSH(level_graph.vertex_position(i)));
                break;
            }
        }

        if (!valid)
            break;
        Logger.Progress(0.15f + 0.85f * float(std::distance(&single_links.front(), &i)) / float(single_links.size()));
    }

    xr_free(stack_storage);
    Logger.Progress(1.f);

    if (valid)
        Msg("AI-map is valid!");

    Msg("Verifying level %s completed", name);
}
