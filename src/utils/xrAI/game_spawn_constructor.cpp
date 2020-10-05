////////////////////////////////////////////////////////////////////////////
//	Module 		: game_spawn_constructor.cpp
//	Created 	: 16.10.2004
//  Modified 	: 16.10.2004
//	Author		: Dmitriy Iassenev
//	Description : Game spawn constructor
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "game_spawn_constructor.h"
#include "xrAICore/Navigation/graph_engine.h"
#include "xrAICore/Navigation/PatrolPath/patrol_path_storage.h"
#include "level_spawn_constructor.h"
#include "xrAI.h"

extern LPCSTR GAME_CONFIG;
extern LPCSTR generate_temp_file_name(LPCSTR header0, LPCSTR header1, string_path& buffer);

#define NO_MULTITHREADING

CGameSpawnConstructor::CGameSpawnConstructor(LPCSTR name, LPCSTR output, LPCSTR start, bool no_separator_check)
    :
#ifdef CONFIG_PROFILE_LOCKS
      m_critical_section(MUTEX_PROFILE_ID(CGameSpawnConstructor)),
#endif // CONFIG_PROFILE_LOCKS
      m_thread_manager(ProxyStatus, ProxyProgress)
{
    load_spawns(name, no_separator_check);
    process_spawns();
    process_actor(start);
    save_spawn(name, output);
}

CGameSpawnConstructor::~CGameSpawnConstructor()
{
    delete_data(m_level_spawns);
    delete_data(m_spawn_graph);
    xr_delete(m_game_graph);
    xr_delete(m_game_info);
    xr_delete(m_patrol_path_storage);
}

IC shared_str CGameSpawnConstructor::actor_level_name()
{
    string256 temp;
    return (strconcat(sizeof(temp), temp,
        *game_graph().header().level(game_graph().vertex(m_actor->m_tGraphID)->level_id()).name(), ".spawn"));
}

extern void read_levels(CInifile* ini, xr_set<CLevelInfo>& m_levels, bool rebuild_graph, xr_vector<LPCSTR>*);
void fill_needed_levels(pstr levels, xr_vector<LPCSTR>& result);

void CGameSpawnConstructor::load_spawns(LPCSTR name, bool no_separator_check)
{
    m_spawn_id = 0;

    // init spawn graph
    m_spawn_graph = xr_new<SPAWN_GRAPH>();

    // init ini file
    m_game_info = xr_new<CInifile>(INI_FILE);
    R_ASSERT(m_game_info->section_exist("levels"));

    // init patrol path storage
    m_patrol_path_storage = xr_new<CPatrolPathStorage>();
    xr_vector<LPCSTR> needed_levels;
    string4096 levels_string;
    xr_strcpy(levels_string, name);
    xr_strlwr(levels_string);
    fill_needed_levels(levels_string, needed_levels);

    // fill level info
    read_levels(&game_info(), m_levels, false, &needed_levels);

    // init game graph
    generate_temp_file_name("game_graph", "", m_game_graph_id);
    xrMergeGraphs(m_game_graph_id, name, false);
    m_game_graph = xr_new<CGameGraph>(m_game_graph_id);

    // load levels
    GameGraph::SLevel level;
    for (const auto &i : m_levels)
    {
        level.m_offset = i.m_offset;
        level.m_name = i.m_name;
        level.m_id = i.m_id;
        Msg("%9s %2d %s", "level", level.id(), *i.m_name);
        m_level_spawns.push_back(xr_new<CLevelSpawnConstructor>(level, this, no_separator_check));
    }

    string256 temp;
    xr_sprintf(temp, "There are no valid levels (with AI-map and graph) in the section 'levels' in the '%s' to build spawn file from!", GAME_CONFIG);
    R_ASSERT2(!m_level_spawns.empty(), temp);
}

void CGameSpawnConstructor::process_spawns()
{
    for (auto &i: m_level_spawns)
#ifdef NO_MULTITHREADING
        i->Execute();
#else
        m_thread_manager.start(i);
    m_thread_manager.wait();
#endif

    for (auto &i : m_level_spawns)
        i->update();

    verify_level_changers();
    verify_spawns();
}

void CGameSpawnConstructor::verify_spawns(ALife::_SPAWN_ID spawn_id)
{
    auto J = std::find(m_temp0.begin(), m_temp0.end(), spawn_id);
    R_ASSERT3(J == m_temp0.end(), "RECURSIVE Spawn group chain found in spawn",
        m_spawn_graph->vertex(spawn_id)->data()->object().name_replace());
    m_temp0.push_back(spawn_id);

    auto vertex = m_spawn_graph->vertex(spawn_id);
    for (const auto &i : vertex->edges())
        verify_spawns(i.vertex_id());
}

void CGameSpawnConstructor::verify_spawns()
{
    for (const auto &i : m_spawn_graph->vertices())
    {
        m_temp0.clear();
        verify_spawns(i.second->vertex_id());
    }
}

void CGameSpawnConstructor::verify_level_changers()
{
    if (m_level_changers.empty())
        return;

    Msg("List of the level changers which are invalid for some reasons");

    for (const auto &i : m_level_changers)
        Msg("%s", i->name_replace());

    VERIFY2(m_level_changers.empty(), "Some of the level changers setup incorrectly");
}

void CGameSpawnConstructor::save_spawn(LPCSTR name, LPCSTR output)
{
    CMemoryWriter stream;

    m_spawn_header.m_version = XRAI_CURRENT_VERSION;
    m_spawn_header.m_guid = generate_guid();
    m_spawn_header.m_graph_guid = game_graph().header().guid();
    m_spawn_header.m_spawn_count = spawn_graph().vertex_count();
    m_spawn_header.m_level_count = (u32)m_level_spawns.size();

    stream.open_chunk(0);
    stream.w_u32(m_spawn_header.m_version);
    save_data(m_spawn_header.m_guid, stream);
    save_data(m_spawn_header.m_graph_guid, stream);
    stream.w_u32(m_spawn_header.m_spawn_count);
    stream.w_u32(m_spawn_header.m_level_count);
    stream.close_chunk();

    stream.open_chunk(1);
    save_data(spawn_graph(), stream);
    stream.close_chunk();

    stream.open_chunk(2);
    save_data(m_level_points, stream);
    stream.close_chunk();

    stream.open_chunk(3);
    save_data(m_patrol_path_storage, stream);
    stream.close_chunk();

    stream.open_chunk(4);
    m_game_graph->save(stream);
    stream.close_chunk();

    stream.save_to(*spawn_name(output));
}

shared_str CGameSpawnConstructor::spawn_name(LPCSTR output)
{
    string_path file_name;
    if (!output)
        FS.update_path(file_name, "$game_spawn$", *actor_level_name());
    else
    {
        actor_level_name();
        string_path out;
        strconcat(sizeof(out), out, output, ".spawn");
        FS.update_path(file_name, "$game_spawn$", out);
    }
    return (file_name);
}

void CGameSpawnConstructor::add_story_object(ALife::_STORY_ID id, CSE_ALifeDynamicObject* object, LPCSTR level_name)
{
    if (id == INVALID_STORY_ID)
        return;

    auto I = m_story_objects.find(id);
    if (I != m_story_objects.end())
    {
        Msg("Object %s, story id %d", object->name_replace(), object->m_story_id);
        Msg("Object %s, story id %d", (*I).second->name_replace(), (*I).second->m_story_id);
        VERIFY3(I == m_story_objects.end(), "There are several objects which has the same unique story ID, level ", level_name);
    }

    m_story_objects.insert(std::make_pair(id, object));
}

void CGameSpawnConstructor::add_object(CSE_Abstract* object)
{
    m_critical_section.Enter();
    object->m_tSpawnID = spawn_id();
    spawn_graph().add_vertex(xr_new<CServerEntityWrapper>(object), object->m_tSpawnID);
    m_critical_section.Leave();
}

void CGameSpawnConstructor::remove_object(CSE_Abstract* object) { spawn_graph().remove_vertex(object->m_tSpawnID); }
void CGameSpawnConstructor::process_actor(LPCSTR start_level_name)
{
    m_actor = nullptr;

    for (const auto &i : m_level_spawns)
    {
        if (!i->actor())
            continue;

        Msg("Actor is on the level %s", *game_graph().header().level(game_graph().vertex(i->actor()->m_tGraphID)->level_id()).name());
        VERIFY2(!m_actor, "There must be the SINGLE level with ACTOR!");
        m_actor = i->actor();
    }

    R_ASSERT2(m_actor, "There is no ACTOR spawn point!");

    if (!start_level_name)
        return;

    if (!xr_strcmp(*actor_level_name(), start_level_name))
        return;

    const auto& level = game_graph().header().level(start_level_name);
    auto dest = GameGraph::_GRAPH_ID(-1);
    GraphEngineSpace::CGameLevelParams evaluator(level.id());
    auto graph_engine = xr_new<CGraphEngine>(game_graph().header().vertex_count());

    bool failed = !graph_engine->search(game_graph(), m_actor->m_tGraphID, GameGraph::_GRAPH_ID(-1), nullptr, evaluator);
    if (failed)
    {
        Msg("! Cannot build path via game graph from the current level to the level %s!", start_level_name);
        float min_dist = flt_max;
        auto current = game_graph().vertex(m_actor->m_tGraphID)->game_point();
        auto n = game_graph().header().vertex_count();
        for (GameGraph::_GRAPH_ID i = 0; i < n; ++i)
        {
            if (game_graph().vertex(i)->level_id() == level.id())
            {
                float distance = game_graph().vertex(i)->game_point().distance_to_sqr(current);
                if (distance < min_dist)
                {
                    min_dist = distance;
                    dest = i;
                }
            }
        }
        if (!game_graph().vertex(dest))
        {
            Msg("! There is no game vertices on the level %s, cannot jump to the specified level", start_level_name);
            xr_delete(graph_engine);
            return;
        }
    }
    else
        dest = (GameGraph::_GRAPH_ID)evaluator.selected_vertex_id();

    m_actor->m_tGraphID = dest;
    m_actor->m_tNodeID = game_graph().vertex(dest)->level_vertex_id();
    m_actor->o_Position = game_graph().vertex(dest)->level_point();

    xr_delete(graph_engine);
}

void clear_temp_folder()
{
    string_path query;
    FS.update_path(query, "$app_data_root$", "temp\\*.*");
    string_path path_root;
    FS.update_path(path_root, "$app_data_root$", "temp\\");
    string_path path_final;

    _finddata_t file;
    auto handle = _findfirst(query, &file);
    if (handle == intptr_t(-1))
        return;

    xr_vector<shared_str> files;
    do
    {
        if (file.attrib & _A_SUBDIR)
            continue;

        strconcat(sizeof(path_final), path_final, path_root, file.name);
        files.push_back(path_final);
    } while (!_findnext(handle, &file));

    _findclose(handle);

    for (const auto &i : files)
    {
        if (DeleteFile(*i))
            Msg("file %s is successfully deleted", *i);
        else
            Msg("cannot delete file %s", *i);
    }
}
