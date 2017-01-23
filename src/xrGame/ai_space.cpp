////////////////////////////////////////////////////////////////////////////
//  Module      : ai_space.h
//  Created     : 12.11.2003
//  Modified    : 12.11.2003
//  Author      : Dmitriy Iassenev
//  Description : AI space class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "pch_script.h"
#include "xrAICore/Navigation/game_graph.h"
#include "xrAICore/Navigation/game_level_cross_table.h"
#include "xrAICore/Navigation/level_graph.h"
#include "xrAICore/Navigation/graph_engine.h"
#include "ef_storage.h"
#include "ai_space.h"
#include "cover_manager.h"
#include "cover_point.h"
#include "xrScriptEngine/script_engine.hpp"
#include "object_factory.h"
#include "alife_simulator.h"
#include "moving_objects.h"
#include "doors_manager.h"

ENGINE_API  bool g_dedicated_server;

CAI_Space *g_ai_space = 0;

CAI_Space::CAI_Space                ()
{
    m_ef_storage            = 0;
    m_cover_manager         = 0;
    m_alife_simulator       = 0;
    m_moving_objects        = 0;
    m_doors_manager         = 0;
}

void CAI_Space::init                ()
{
    if (g_dedicated_server)
        return;
    AISpaceBase::Initialize();
    VERIFY                  (!m_ef_storage);
    m_ef_storage            = new CEF_Storage();
    
    VERIFY                  (!m_cover_manager);
    m_cover_manager         = new CCoverManager();

    VERIFY                  (!m_moving_objects);
    m_moving_objects        = new ::moving_objects();
    VERIFY(!GlobalEnv.ScriptEngine);
    GlobalEnv.ScriptEngine = new CScriptEngine();    
    SetupScriptEngine();
}

CAI_Space::~CAI_Space               ()
{
    unload                  ();
    xr_delete(GlobalEnv.ScriptEngine); // XXX: wrapped into try..catch(...) in vanilla source
    xr_delete               (m_doors_manager);
    xr_delete               (m_moving_objects);
    xr_delete               (m_cover_manager);
    xr_delete               (m_ef_storage);
}

void CAI_Space::RegisterScriptClasses()
{
#ifdef DBG_DISABLE_SCRIPTS
    return;
#else
    string_path S;
    FS.update_path(S, "$game_config$", "script.ltx");
    CInifile *l_tpIniFile = new CInifile(S);
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
    CInifile *l_tpIniFile = new CInifile(S);
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
    AISpaceBase::Load(level_name);
    m_cover_manager->compute_static_cover   ();
    m_moving_objects->on_level_load         ();

    VERIFY                  (!m_doors_manager);
    m_doors_manager         = new ::doors::manager(level_graph().header().box() );

#ifdef DEBUG
    Msg                     ("* Loading ai space is successfully completed (%.3fs, %7.3f Mb)",timer.GetElapsed_sec(),float(Memory.mem_usage() - mem_usage)/1048576.0);
#endif
}

void CAI_Space::unload              (bool reload)
{
    if (g_dedicated_server)
        return;
    script_engine().unload();
    xr_delete(m_doors_manager);
    AISpaceBase::Unload(reload);
}

void CAI_Space::set_alife               (CALifeSimulator *alife_simulator)
{
    VERIFY                  ((!m_alife_simulator && alife_simulator) || (m_alife_simulator && !alife_simulator));
    m_alife_simulator       = alife_simulator;

    VERIFY                  (!alife_simulator || !m_game_graph);
    if (alife_simulator)
        return;
    SetGameGraph(nullptr);
}
