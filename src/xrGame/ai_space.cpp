////////////////////////////////////////////////////////////////////////////
//  Module      : ai_space.h
//  Created     : 12.11.2003
//  Modified    : 12.11.2003
//  Author      : Dmitriy Iassenev
//  Description : AI space class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "pch_script.h"
#include "game_graph.h"
#include "game_level_cross_table.h"
#include "level_graph.h"
#include "graph_engine.h"
#include "ef_storage.h"
#include "ai_space.h"
#include "cover_manager.h"
#include "cover_point.h"
#include "xrScriptEngine/script_engine.hpp"
#include "object_factory.h"
#include "patrol_path_storage.h"
#include "alife_simulator.h"
#include "moving_objects.h"
#include "doors_manager.h"

ENGINE_API  bool g_dedicated_server;

CAI_Space *g_ai_space = 0;

CAI_Space::CAI_Space                ()
{
    m_ef_storage            = 0;
    m_game_graph            = 0;
    m_graph_engine          = 0;
    m_cover_manager         = 0;
    m_level_graph           = 0;
    m_alife_simulator       = 0;
    m_patrol_path_storage   = 0;
    m_moving_objects        = 0;
    m_doors_manager         = 0;
}

void CAI_Space::init                ()
{
    if (g_dedicated_server)
        return;
    VERIFY                  (!m_ef_storage);
    m_ef_storage            = xr_new<CEF_Storage>();

    VERIFY                  (!m_graph_engine);
    m_graph_engine          = xr_new<CGraphEngine>(1024);

    VERIFY                  (!m_cover_manager);
    m_cover_manager         = xr_new<CCoverManager>();

    VERIFY                  (!m_patrol_path_storage);
    m_patrol_path_storage   = xr_new<CPatrolPathStorage>();

    VERIFY                  (!m_moving_objects);
    m_moving_objects        = xr_new<::moving_objects>();
    VERIFY(!GlobalEnv.ScriptEngine);
    GlobalEnv.ScriptEngine = xr_new<CScriptEngine>();    
    SetupScriptEngine();
}

CAI_Space::~CAI_Space               ()
{
    unload                  ();
    xr_delete(GlobalEnv.ScriptEngine); // XXX: wrapped into try..catch(...) in vanilla source
    xr_delete               (m_doors_manager);
    xr_delete               (m_moving_objects);
    xr_delete               (m_patrol_path_storage);
    xr_delete               (m_cover_manager);
    xr_delete               (m_graph_engine);
    xr_delete               (m_ef_storage);
    VERIFY                  (!m_game_graph);
}

void CAI_Space::RegisterScriptClasses()
{
#ifdef DBG_DISABLE_SCRIPTS
    return;
#else
    string_path S;
    FS.update_path(S, "$game_config$", "script.ltx");
    CInifile *l_tpIniFile = xr_new<CInifile>(S);
    R_ASSERT(l_tpIniFile);
    if (!l_tpIniFile->section_exist("common"))
    {
        xr_delete(l_tpIniFile);
        return;
    }
    shared_str registrators = READ_IF_EXISTS(l_tpIniFile, r_string, "common", "class_registrators", "");
    xr_delete(l_tpIniFile);
    u32 registratorCount = _GetItemCount(*registrators);
    string256 I;
    for (u32 i = 0; i<registratorCount; i++)
    {
        _GetItem(*registrators, i, I);
        luabind::functor<void> result;
        if (!script_engine().functor(I, result))
        {
            script_engine().script_log(LuaMessageType::Error, "Cannot load class registrator %s!", I);
            continue;
        }
        result(const_cast<CObjectFactory*>(&object_factory()));
    }
#endif
}

void CAI_Space::LoadCommonScripts()
{
#ifdef DBG_DISABLE_SCRIPTS
    return;
#else
    string_path S;
    FS.update_path(S, "$game_config$", "script.ltx");
    CInifile *l_tpIniFile = xr_new<CInifile>(S);
    R_ASSERT(l_tpIniFile);
    if (!l_tpIniFile->section_exist("common"))
    {
        xr_delete(l_tpIniFile);
        return;
    }
    if (l_tpIniFile->line_exist("common", "script"))
    {
        shared_str scriptString = l_tpIniFile->r_string("common", "script");
        u32 scriptCount = _GetItemCount(*scriptString);
        string256 scriptName;
        for (u32 i = 0; i<scriptCount; i++)
        {
            _GetItem(*scriptString, i, scriptName);
            script_engine().load_file(scriptName, script_engine().GlobalNamespace);
        }
    }
    xr_delete(l_tpIniFile);
#endif
}

void CAI_Space::SetupScriptEngine()
{
    XRay::ScriptExporter::Reset(); // mark all nodes as undone
    GlobalEnv.ScriptEngine->init(XRay::ScriptExporter::Export, true);
    RegisterScriptClasses();
    object_factory().register_script();
    LoadCommonScripts();
}

void CAI_Space::load                (LPCSTR level_name)
{
    VERIFY                  (m_game_graph);

    unload                  (true);

#ifdef DEBUG
    Memory.mem_compact      ();
    u32                     mem_usage = Memory.mem_usage();
    CTimer                  timer;
    timer.Start             ();
#endif

    const CGameGraph::SLevel &current_level = game_graph().header().level(level_name);

    m_level_graph           = xr_new<CLevelGraph>();
    game_graph().set_current_level(current_level.id());
    R_ASSERT2               (cross_table().header().level_guid() == level_graph().header().guid(), "cross_table doesn't correspond to the AI-map");
    R_ASSERT2               (cross_table().header().game_guid() == game_graph().header().guid(), "graph doesn't correspond to the cross table");
    m_graph_engine          = xr_new<CGraphEngine>(
        _max(
            game_graph().header().vertex_count(),
            level_graph().header().vertex_count()
        )
    );
    
    R_ASSERT2               (current_level.guid() == level_graph().header().guid(), "graph doesn't correspond to the AI-map");
    
#ifdef DEBUG
    if (!xr_strcmp(current_level.name(),level_name))
        validate            (current_level.id());
#endif

    level_graph().level_id  (current_level.id());
    m_cover_manager->compute_static_cover   ();
    m_moving_objects->on_level_load         ();

    VERIFY                  (!m_doors_manager);
    m_doors_manager         = xr_new<::doors::manager>( ai().level_graph().header().box() );

#ifdef DEBUG
    Msg                     ("* Loading ai space is successfully completed (%.3fs, %7.3f Mb)",timer.GetElapsed_sec(),float(Memory.mem_usage() - mem_usage)/1048576.0);
#endif
}

void CAI_Space::unload              (bool reload)
{
    if (g_dedicated_server)
        return;

    script_engine().unload  ();

    xr_delete               (m_doors_manager);
    xr_delete               (m_graph_engine);
    xr_delete               (m_level_graph);

    if (!reload && m_game_graph)
        m_graph_engine      = xr_new<CGraphEngine>( game_graph().header().vertex_count() );
}

#ifdef DEBUG
void CAI_Space::validate            (const u32 level_id) const
{
    VERIFY                  (level_graph().header().vertex_count() == cross_table().header().level_vertex_count());
    for (GameGraph::_GRAPH_ID i=0, n = game_graph().header().vertex_count(); i<n; ++i)
        if ((level_id == game_graph().vertex(i)->level_id()) && 
            (!level_graph().valid_vertex_id(game_graph().vertex(i)->level_vertex_id()) ||
            (cross_table().vertex(game_graph().vertex(i)->level_vertex_id()).game_vertex_id() != i) ||
            !level_graph().inside(game_graph().vertex(i)->level_vertex_id(),game_graph().vertex(i)->level_point()))) {
            Msg             ("! Graph doesn't correspond to the cross table");
            R_ASSERT2       (false,"Graph doesn't correspond to the cross table");
        }

//  Msg                     ("death graph point id : %d",cross_table().vertex(455236).game_vertex_id());

    for (u32 i=0, n=game_graph().header().vertex_count(); i<n; ++i) {
        if (level_id != game_graph().vertex(i)->level_id())
            continue;

        CGameGraph::const_spawn_iterator    I, E;
        game_graph().begin_spawn            (i,I,E);
//      Msg                                 ("vertex [%d] has %d death points",i,game_graph().vertex(i)->death_point_count());
        for ( ; I != E; ++I) {
            VERIFY                          (cross_table().vertex((*I).level_vertex_id()).game_vertex_id() == i);
        }
    }
    

//  Msg                     ("* Graph corresponds to the cross table");
}
#endif

void CAI_Space::patrol_path_storage_raw (IReader &stream)
{
    if (g_dedicated_server)
        return;

    xr_delete                       (m_patrol_path_storage);
    m_patrol_path_storage           = xr_new<CPatrolPathStorage>();
    m_patrol_path_storage->load_raw (get_level_graph(),get_cross_table(),get_game_graph(),stream);
}

void CAI_Space::patrol_path_storage     (IReader &stream)
{
    if (g_dedicated_server)
        return;

    xr_delete                       (m_patrol_path_storage);
    m_patrol_path_storage           = xr_new<CPatrolPathStorage>();
    m_patrol_path_storage->load     (stream);
}

void CAI_Space::set_alife               (CALifeSimulator *alife_simulator)
{
    VERIFY                  ((!m_alife_simulator && alife_simulator) || (m_alife_simulator && !alife_simulator));
    m_alife_simulator       = alife_simulator;

    VERIFY                  (!alife_simulator || !m_game_graph);
    if (alife_simulator)
        return;

    VERIFY                  (m_game_graph);
    m_game_graph            = 0;
    xr_delete               (m_graph_engine);
}

void CAI_Space::game_graph              (CGameGraph *game_graph)
{
    VERIFY                  (m_alife_simulator);
    VERIFY                  (game_graph);
    VERIFY                  (!m_game_graph);
    m_game_graph            = game_graph;

//  VERIFY                  (!m_graph_engine);
    xr_delete               (m_graph_engine);
    m_graph_engine          = xr_new<CGraphEngine>(this->game_graph().header().vertex_count());
}

const CGameLevelCrossTable &CAI_Space::cross_table      () const
{
    return                  (game_graph().cross_table());
}

const CGameLevelCrossTable *CAI_Space::get_cross_table  () const
{
    return                  (&game_graph().cross_table());
}