////////////////////////////////////////////////////////////////////////////
//	Module 		: game_graph_builder.cpp
//	Created 	: 14.12.2005
//  Modified 	: 14.12.2005
//	Author		: Dmitriy Iassenev
//	Description : Game graph builder
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "game_graph_builder.h"
#include "xrServerEntities/xrServer_Objects_ALife.h"
#include "xrServerEntities/xrMessages.h"
#include "factory_api.h"
#include "guid_generator.h"

CGameGraphBuilder::CGameGraphBuilder()
{
    m_level_graph = 0;
    m_graph = 0;
    m_cross_table = 0;
}

CGameGraphBuilder::~CGameGraphBuilder()
{
    Msg("Freeing resources");

    xr_delete(m_level_graph);
    xr_delete(m_graph);
    xr_delete(m_cross_table);
}

void CGameGraphBuilder::create_graph(const float& start, const float& amount)
{
    Logger.Progress(start);

    VERIFY(!m_graph);
    m_graph = new graph_type();

    m_graph_guid = generate_guid();

    Logger.Progress(start + amount);
}

void CGameGraphBuilder::load_level_graph(const float& start, const float& amount)
{
    Logger.Progress(start);

    Msg("Loading AI map");

    VERIFY(!m_level_graph);
    m_level_graph = new CLevelGraph(*m_level_name);

    Msg("%d nodes loaded", level_graph().header().vertex_count());

    Logger.Progress(start + amount);
}

void CGameGraphBuilder::load_graph_point(NET_Packet& net_packet)
{
    string256 section_id;
    u16 id;
    net_packet.r_begin(id);
    R_ASSERT(M_SPAWN == id);
    net_packet.r_stringZ(section_id);

    //	if (xr_strcmp("graph_point",section_id))
    //		return;

    CSE_Abstract* entity = F_entity_Create(section_id);
    if (!entity)
    {
        Msg("Cannot create entity from section %s, skipping", section_id);
        return;
    }

    CSE_ALifeGraphPoint* graph_point = smart_cast<CSE_ALifeGraphPoint*>(entity);
    if (!graph_point)
    {
        F_entity_Destroy(entity);
        return;
    }

    entity->Spawn_Read(net_packet);

    vertex_type vertex;
    vertex.tLocalPoint = graph_point->o_Position;
    // Check for duplicate graph point positions
    for (const auto &i : graph().vertices())
    {
        if (i.second->data().tLocalPoint.distance_to_sqr(vertex.tLocalPoint) < EPS_L)
        {
            Msg("! removing graph point [%s][%f][%f][%f] because it is too close to the another graph point", entity->name_replace(), VPUSH(entity->o_Position));
            F_entity_Destroy(entity);
            return;
        }
    }

    vertex.tGlobalPoint = graph_point->o_Position;
    vertex.tNodeID = level_graph().valid_vertex_position(vertex.tLocalPoint) ? level_graph().vertex_id(vertex.tLocalPoint) : u32(-1);
    if (!level_graph().valid_vertex_id(vertex.tNodeID))
    {
        Msg("! removing graph point [%s][%f][%f][%f] because it is outside of the AI map", entity->name_replace(), VPUSH(entity->o_Position));
        F_entity_Destroy(entity);
        return;
    }

    for (const auto &i : graph().vertices())
    {
        if (i.second->data().tNodeID == vertex.tNodeID)
        {
            Msg("! removing graph point [%s][%f][%f][%f] because it has the same AI node as another graph point", entity->name_replace(), VPUSH(entity->o_Position));
            F_entity_Destroy(entity);
            return;
        }
    }

    vertex.tNeighbourCount = 0;
    memcpy(vertex.tVertexTypes, graph_point->m_tLocations, GameGraph::LOCATION_TYPE_COUNT * sizeof(GameGraph::_LOCATION_ID));
    vertex.tLevelID = 0;
    vertex.tDeathPointCount = 0;
    vertex.dwPointOffset = 0;

    graph().add_vertex(vertex, graph().vertices().size());

    F_entity_Destroy(entity);
}

void CGameGraphBuilder::load_graph_points(const float& start, const float& amount)
{
    Logger.Progress(start);

    Msg("Loading graph points");

    string_path spawn_file_name;
    strconcat(sizeof(spawn_file_name), spawn_file_name, *m_level_name, "level.spawn");
    IReader* reader = FS.r_open(spawn_file_name);
    u32 id;
    NET_Packet net_packet;
    for (IReader* chunk = reader->open_chunk_iterator(id); chunk; chunk = reader->open_chunk_iterator(id, chunk))
    {
        net_packet.B.count = chunk->length();
        chunk->r(net_packet.B.data, net_packet.B.count);
        load_graph_point(net_packet);
    }

    FS.r_close(reader);

    Msg("%d graph points loaded", graph().vertices().size());

    Logger.Progress(start + amount);
}

template <typename T>
IC bool sort_predicate_less(const T& first, const T& second)
{
    return (first.first < second.first);
}

template <typename T>
IC bool sort_predicate_greater(const T& first, const T& second)
{
    return (first.first > second.first);
}

void CGameGraphBuilder::mark_vertices(u32 level_vertex_id)
{
    m_mark_stack.reserve(8192);
    m_mark_stack.push_back(level_vertex_id);

    for (; !m_mark_stack.empty();)
    {
        level_vertex_id = m_mark_stack.back();
        m_mark_stack.resize(m_mark_stack.size() - 1);
        auto node = level_graph().vertex(level_vertex_id);
        m_marks[level_vertex_id] = true;
        for (const auto &i : {0, 1, 2, 3})
        {
            u32 next_level_vertex_id = node->link(i);

            if (!level_graph().valid_vertex_id(next_level_vertex_id))
                continue;

            if (!m_marks[next_level_vertex_id])
                m_mark_stack.push_back(next_level_vertex_id);
        }
    }
}

void CGameGraphBuilder::fill_marks(const float& start, const float& amount)
{
    Logger.Progress(start);

    m_marks.assign(level_graph().header().vertex_count(), false);
    for (const auto &i : graph().vertices())
        mark_vertices(i.second->data().level_vertex_id());
    m_marks.flip();

    Logger.Progress(start + amount);
}

void CGameGraphBuilder::fill_distances(const float& start, const float& amount)
{
    Logger.Progress(start);

    m_distances.resize(graph().vertices().size());
    for (auto &i : m_distances)
    {
        i.resize(level_graph().header().vertex_count());
        for (auto &j : i)
            j = u32(-1);
    }

    Logger.Progress(start + amount);
}

void CGameGraphBuilder::recursive_update(const u32& game_vertex_id, const float& start, const float& amount)
{
    Logger.Progress(start);

    u32 level_vertex_id = graph().vertex(game_vertex_id)->data().level_vertex_id();
    xr_vector<u32>& distances = m_distances[game_vertex_id];
    m_distances[m_results[level_vertex_id]][level_vertex_id] = u32(-1);

    m_current_fringe.reserve(distances.size());
    m_next_fringe.reserve(distances.size());
    distances.assign(distances.size(), u32(-1));
    m_current_fringe.push_back(level_vertex_id);
    u32 curr_dist = 0;
    u32 total_count = 0;

    u32 vertex_count = graph().header().vertex_count();

    float amount_i = amount / (float(vertex_count) * float(level_graph().header().vertex_count()));

    Logger.Progress(start);
    for (; !m_current_fringe.empty();)
    {
        for (const auto &i : m_current_fringe)
        {
            u32* result = &m_results[i];
            VERIFY(curr_dist < m_distances[*result][i]);
            *result = game_vertex_id;

            distances[i] = curr_dist;
            CLevelGraph::CLevelVertex* node = level_graph().vertex(i);
            for (const auto &j : {0, 1, 2, 3})
            {
                u32 dwNexNodeID = node->link(j);
                if (!level_graph().valid_vertex_id(dwNexNodeID))
                    continue;

                if (m_marks[dwNexNodeID])
                    continue;

                if (distances[dwNexNodeID] <= curr_dist)
                    continue;

                if (m_distances[m_results[dwNexNodeID]][dwNexNodeID] <= (curr_dist + 1))
                    continue;

                m_next_fringe.push_back(dwNexNodeID);
                m_marks[dwNexNodeID] = true;
            }
        }

        for (const auto &i : m_current_fringe)
            m_marks[i] = false;

        total_count += m_current_fringe.size();
        m_current_fringe = m_next_fringe;
        m_next_fringe.clear();
        ++curr_dist;

        Logger.Progress(start + amount_i * float(total_count));
    }

    Logger.Progress(start + amount);
}

void CGameGraphBuilder::iterate_distances(const float& start, const float& amount)
{
    Logger.Progress(start);

    m_results.assign(level_graph().header().vertex_count(), 0);

    auto size = graph().vertices().size();
    float amount_i = amount / float(size);

    for (auto i = 0; i < size; ++i)
    {
        if (i)
            for (int k = 0, kn = (int)level_graph().header().vertex_count(); k < kn; ++k)
                m_distances[i][k] = m_distances[i - 1][k];

        recursive_update(i, start + amount_i * float(i), amount_i);
    }

    Logger.Progress(start + amount);
}

void CGameGraphBuilder::save_cross_table(const float& start, const float& amount)
{
    Logger.Progress(start);

    Msg("Saving cross table");

    //	CTimer								timer;
    //	timer.Start							();

    CMemoryWriter tMemoryStream;
    CGameLevelCrossTable::CHeader tCrossTableHeader;

    tCrossTableHeader.dwVersion = XRAI_CURRENT_VERSION;
    tCrossTableHeader.dwNodeCount = level_graph().header().vertex_count();
    tCrossTableHeader.dwGraphPointCount = graph().header().vertex_count();
    tCrossTableHeader.m_level_guid = level_graph().header().guid();
    tCrossTableHeader.m_game_guid = m_graph_guid;

    tMemoryStream.open_chunk(CROSS_TABLE_CHUNK_VERSION);
    tMemoryStream.w(&tCrossTableHeader, sizeof(tCrossTableHeader));
    tMemoryStream.close_chunk();

    tMemoryStream.open_chunk(CROSS_TABLE_CHUNK_DATA);

    for (int i = 0, n = level_graph().header().vertex_count(); i < n; i++)
    {
        CGameLevelCrossTable::CCell tCrossTableCell;
        tCrossTableCell.tGraphIndex = (GameGraph::_GRAPH_ID)m_results[i];
        VERIFY(graph().header().vertex_count() > tCrossTableCell.tGraphIndex);
        tCrossTableCell.fDistance = float(m_distances[tCrossTableCell.tGraphIndex][i]) * level_graph().header().cell_size();
        tMemoryStream.w(&tCrossTableCell, sizeof(tCrossTableCell));
    }

    tMemoryStream.close_chunk();

    //	Msg						("CT:SAVE : %f",timer.GetElapsed_sec());
    //	Msg						("Flushing cross table");

    tMemoryStream.save_to(m_cross_table_name);
    //	Msg						("CT:SAVE : %f",timer.GetElapsed_sec());

    //	Msg						("Freiing cross table resources");

    m_marks.clear();
    m_mark_stack.clear();
    m_distances.clear();
    m_current_fringe.clear();
    m_next_fringe.clear();

    //	Msg						("CT:SAVE : %f",timer.GetElapsed_sec());
    Logger.Progress(start + amount);
}

void CGameGraphBuilder::build_cross_table(const float& start, const float& amount)
{
    Logger.Progress(start);

    Msg("Building cross table");

    //	CTimer					timer;
    //	timer.Start				();

    fill_marks(start + 0.000000f * amount, 0.018725f * amount);
    //	Msg						("CT : %f",timer.GetElapsed_sec());
    fill_distances(start + 0.018725f * amount, 0.183732f * amount);
    //	Msg						("CT : %f",timer.GetElapsed_sec());
    iterate_distances(start + 0.202457f * amount, 0.757202f * amount);
    //	Msg						("CT : %f",timer.GetElapsed_sec());
    save_cross_table(start + 0.959659f * amount, 0.040327f * amount);
    //	Msg						("CT : %f",timer.GetElapsed_sec());
    load_cross_table(start + 0.999986f * amount, 0.000014f * amount);
    //	Msg						("CT : %f",timer.GetElapsed_sec());

    Logger.Progress(start + amount);
}

void CGameGraphBuilder::load_cross_table(const float& start, const float& amount)
{
    Logger.Progress(start);

    Msg("Loading cross table");

    VERIFY(!m_cross_table);
    m_cross_table = new CGameLevelCrossTable(m_cross_table_name);

    Logger.Progress(start + amount);
}

void CGameGraphBuilder::fill_neighbours(const u32& game_vertex_id)
{
    m_marks.assign(level_graph().header().vertex_count(), false);
    m_current_fringe.clear();

    u32 level_vertex_id = graph().vertex(game_vertex_id)->data().level_vertex_id();

    m_mark_stack.reserve(8192);
    m_mark_stack.push_back(level_vertex_id);

    for (; !m_mark_stack.empty();)
    {
        level_vertex_id = m_mark_stack.back();
        m_mark_stack.resize(m_mark_stack.size() - 1);
        auto node = level_graph().vertex(level_vertex_id);
        m_marks[level_vertex_id] = true;
        for (const auto &i : {0, 1, 2, 3})
        {
            u32 next_level_vertex_id = node->link(i);

            if (!level_graph().valid_vertex_id(next_level_vertex_id))
                continue;

            if (m_marks[next_level_vertex_id])
                continue;

            GameGraph::_GRAPH_ID next_game_vertex_id = cross().vertex(next_level_vertex_id).game_vertex_id();
            VERIFY(next_game_vertex_id < graph().vertices().size());
            if (next_game_vertex_id != (GameGraph::_GRAPH_ID)game_vertex_id)
            {
                if (std::find(m_current_fringe.begin(), m_current_fringe.end(), next_game_vertex_id) ==
                    m_current_fringe.end())
                    m_current_fringe.push_back(next_game_vertex_id);
                continue;
            }

            m_mark_stack.push_back(next_level_vertex_id);
        }
    }
}

float CGameGraphBuilder::path_distance(const u32& game_vertex_id0, const u32& game_vertex_id1)
{
    //	return
    //(graph().vertex(game_vertex_id0)->data().level_point().distance_to(graph().vertex(game_vertex_id1)->data().level_point()));

    VERIFY(m_graph_engine);

    graph_type::CVertex& vertex0 = *graph().vertex(game_vertex_id0);
    graph_type::CVertex& vertex1 = *graph().vertex(game_vertex_id1);

    typedef GraphEngineSpace::CStraightLineParams CStraightLineParams;
    CStraightLineParams parameters(vertex0.data().level_point(), vertex1.data().level_point());

    float pure_distance = vertex0.data().level_point().distance_to_xz(vertex1.data().level_point());
    //	float					pure_distance = vertex0.data().level_point().distance_to(vertex1.data().level_point());
    VERIFY(pure_distance < parameters.max_range);

    u32 level_vertex_id = level_graph().check_position_in_direction(
        vertex0.data().level_vertex_id(), vertex0.data().level_point(), vertex1.data().level_point());
    if (level_graph().valid_vertex_id(level_vertex_id))
        return (pure_distance);

    bool successfull = m_graph_engine->search(
        level_graph(), vertex0.data().level_vertex_id(), vertex1.data().level_vertex_id(), &m_path, parameters);

    if (successfull)
        return (parameters.m_distance);

    Msg("Cannot build path from [%d] to [%d]", game_vertex_id0, game_vertex_id1);
    Msg("Cannot build path from [%f][%f][%f] to [%f][%f][%f]", VPUSH(vertex0.data().level_point()),
        VPUSH(vertex1.data().level_point()));
    R_ASSERT2(false, "Cannot build path, check AI map");
    return flt_max;
}

void CGameGraphBuilder::generate_edges(const u32& game_vertex_id)
{
    auto vertex = graph().vertex(game_vertex_id);

    for (const auto &i: m_current_fringe)
    {
        VERIFY(!vertex->edge(i));
        float distance = path_distance(game_vertex_id, i);
        graph().add_edge(game_vertex_id, i, distance);
    }
}

void CGameGraphBuilder::generate_edges(const float& start, const float& amount)
{
    Logger.Progress(start);

    Msg("Generating edges");

    for (const auto &i : graph().vertices())
    {
        fill_neighbours(i.second->vertex_id());
        generate_edges(i.second->vertex_id());
    }

    Msg("%d edges built", graph().edge_count());

    Logger.Progress(start + amount);
}

void CGameGraphBuilder::connectivity_check(const float& start, const float& amount)
{
    Logger.Progress(start);

    Msg("Checking graph connectivity");

    Logger.Progress(start + amount);
}

void CGameGraphBuilder::create_tripples(const float& /*start*/, const float& /*amount*/)
{
    for (const auto &i: graph().vertices())
    {
        for (const auto &j : i.second->edges())
        {
            if ((j.vertex_id() < i.first) && graph().edge(j.vertex_id(), i.first))
                continue;

            const graph_type::CEdge* edge = graph().vertex(j.vertex_id())->edge(i.first);

            m_tripples.push_back(std::make_pair(std::min(j.weight(), edge ? edge->weight() : j.weight()),
                std::make_pair(i.first, j.vertex_id())));
        }
    }

    std::sort(m_tripples.begin(), m_tripples.end(), sort_predicate_greater<TRIPPLE>);
}

void CGameGraphBuilder::process_tripple(const TRIPPLE& tripple)
{
    const graph_type::CVertex& vertex0 = *graph().vertex(tripple.second.first);
    const graph_type::CVertex& vertex1 = *graph().vertex(tripple.second.second);

    for (const auto &i : vertex0.edges())
    {
        if (i.vertex_id() == tripple.second.second)
            continue;

        const graph_type::CEdge* edge;

        edge = vertex1.edge(i.vertex_id());
        if (edge)
        {
            VERIFY(_min(i.weight(), graph().edge(i.vertex_id(), tripple.second.first) ?
                           graph().edge(i.vertex_id(), tripple.second.first)->weight() :
                           i.weight()) <= tripple.first);
            VERIFY(_min(edge->weight(), graph().edge(edge->vertex_id(), tripple.second.second) ?
                           graph().edge(edge->vertex_id(), tripple.second.second)->weight() :
                           i.weight()) <= tripple.first);
            if (vertex0.edge(tripple.second.second))
                graph().remove_edge(tripple.second.first, tripple.second.second);
            if (vertex1.edge(tripple.second.first))
                graph().remove_edge(tripple.second.second, tripple.second.first);
            return;
        }

        edge = graph().vertex(i.vertex_id())->edge(tripple.second.second);
        if (edge)
        {
            VERIFY(_min(i.weight(), graph().edge(i.vertex_id(), tripple.second.first) ?
                           graph().edge(i.vertex_id(), tripple.second.first)->weight() :
                           i.weight()) <= tripple.first);
            VERIFY(edge->weight() <= tripple.first);
            if (vertex0.edge(tripple.second.second))
                graph().remove_edge(tripple.second.first, tripple.second.second);
            if (vertex1.edge(tripple.second.first))
                graph().remove_edge(tripple.second.second, tripple.second.first);
            return;
        }
    }
    return;
}

void CGameGraphBuilder::optimize_graph(const float& start, const float& amount)
{
    Logger.Progress(start);

    Msg("Optimizing graph");

    Msg("edges before optimization : %d", graph().edge_count());

    create_tripples(start + .00f, amount * .50f);

    for (const auto &i: m_tripples)
        process_tripple(i);

    Msg("edges after optimization : %d", graph().edge_count());

    Logger.Progress(start + amount);
}

void CGameGraphBuilder::save_graph(const float& start, const float& amount)
{
    Logger.Progress(start);

    Msg("Saving graph");

    // header
    CMemoryWriter writer;
    CGameGraph::CHeader header;
    header.m_version = XRAI_CURRENT_VERSION;
    VERIFY(graph().vertices().size() < (u32(1) << (8 * sizeof(GameGraph::_GRAPH_ID))));
    header.m_vertex_count = (GameGraph::_GRAPH_ID)graph().vertices().size();
    VERIFY(graph().edge_count() < (u32(1) << (8 * sizeof(GameGraph::_GRAPH_ID))));
    header.m_edge_count = (GameGraph::_GRAPH_ID)graph().edge_count();
    header.m_death_point_count = 0;
    header.m_guid = m_graph_guid;

    // levels
    CGameGraph::SLevel level;
    level.m_offset.set(0, 0, 0);
    level.m_id = 0;
    level.m_name = m_level_name;
    level.m_section = "";
    level.m_guid = level_graph().header().guid();

    header.m_levels.insert(std::make_pair(level.m_id, level));

    header.save(&writer);

    {
        u32 edge_offset = graph().vertices().size() * sizeof(CGameGraph::CVertex);

        for (const auto &i: graph().vertices())
        {
            CGameGraph::CVertex& vertex = i.second->data();

            VERIFY(i.second->edges().size() < 256);
            vertex.tNeighbourCount = (u8)i.second->edges().size();
            vertex.dwEdgeOffset = edge_offset;
            edge_offset += vertex.tNeighbourCount * sizeof(CGameGraph::CEdge);

            writer.w(&vertex, sizeof(CGameGraph::CVertex));
        }
    }

    {
        for (const auto &i : graph().vertices())
        {
            for (const auto &j: i.second->edges())
            {
                GameGraph::CEdge edge;
                VERIFY(j.vertex_id() < (u32(1) << (8 * sizeof(GameGraph::_GRAPH_ID))));
                edge.m_vertex_id = (GameGraph::_GRAPH_ID)j.vertex_id();
                edge.m_path_distance = j.weight();

                writer.w(&edge.m_vertex_id, sizeof(edge.m_vertex_id));
                writer.w_float(edge.m_path_distance);
            }
        }
    }

    writer.save_to(m_graph_name);
    Msg("%d bytes saved", int(writer.size()));

    Logger.Progress(start + amount);
}

void CGameGraphBuilder::build_graph(const float& start, const float& amount)
{
    Logger.Progress(start);

    Msg("Building graph");

    CTimer timer;
    timer.Start();

    m_graph_engine = new CGraphEngine(level_graph().header().vertex_count());
    Logger.Progress(start + 0.000000f * amount + amount * 0.067204f);
    //	Msg						("BG : %f",timer.GetElapsed_sec());

    generate_edges(start + 0.067204f * amount, amount * 0.922647f);
    //	Msg						("BG : %f",timer.GetElapsed_sec());

    xr_delete(m_graph_engine);
    Logger.Progress(start + 0.989851f * amount + amount * 0.002150f);
    //	Msg						("BG : %f",timer.GetElapsed_sec());

    connectivity_check(start + 0.992001f * amount, amount * 0.000030f);
    //	Msg						("BG : %f",timer.GetElapsed_sec());
    optimize_graph(start + 0.992031f * amount, amount * 0.000454f);
    //	Msg						("BG : %f",timer.GetElapsed_sec());
    save_graph(start + 0.992485f * amount, amount * 0.007515f);
    //	Msg						("BG : %f",timer.GetElapsed_sec());

    Logger.Progress(start + amount);
}

void CGameGraphBuilder::build_graph(LPCSTR graph_name, LPCSTR cross_table_name, LPCSTR level_name)
{
    Logger.Phase("Building level game graph");
    Msg("level \"%s\"", level_name);

    m_graph_name = graph_name;
    m_cross_table_name = cross_table_name;
    m_level_name = level_name;

    //	CTimer					timer;
    //	timer.Start				();

    create_graph(0.000000f, 0.000047f);
    //	Msg						("%f",timer.GetElapsed_sec());
    load_level_graph(0.000047f, 0.002470f);
    //	Msg						("%f",timer.GetElapsed_sec());
    load_graph_points(0.002517f, 0.111812f);
    //	Msg						("%f",timer.GetElapsed_sec());
    build_cross_table(0.114329f, 0.773423f);
    //	Msg						("%f",timer.GetElapsed_sec());
    build_graph(0.887752f, 0.112248f);
    //	Msg						("%f",timer.GetElapsed_sec());

    Msg("Level graph is generated successfully");
}
