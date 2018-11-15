////////////////////////////////////////////////////////////////////////////
//  Module      : ai_space.h
//  Created     : 12.11.2003
//  Modified    : 12.11.2003
//  Author      : Dmitriy Iassenev
//  Description : AI space class
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
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

CAI_Space* g_ai_space;

CAI_Space& CAI_Space::GetInstance()
{
    if (!g_ai_space)
    {
        g_ai_space = new CAI_Space();
        g_ai_space->init();
    }
    return *g_ai_space;
}

void CAI_Space::init()
{
    R_ASSERT(!m_inited);

    if (!GEnv.isDedicatedServer)
    {
        AISpaceBase::Initialize();

        m_ef_storage = xr_make_unique<CEF_Storage>();
        m_cover_manager = xr_make_unique<CCoverManager>();
        m_moving_objects = xr_make_unique<::moving_objects>();

        VERIFY(!GEnv.ScriptEngine);
        GEnv.ScriptEngine = new CScriptEngine();
        RestartScriptEngine();
    }

    m_inited = true;
}

CAI_Space::~CAI_Space()
{
    if (GEnv.ScriptEngine != nullptr)
    {
        m_events_notifier.FireEvent(EVENT_SCRIPT_ENGINE_RESET);
    }

    unload();
    xr_delete(GEnv.ScriptEngine); // XXX: wrapped into try..catch(...) in vanilla source
}

void CAI_Space::RegisterScriptClasses()
{
#ifdef DBG_DISABLE_SCRIPTS
    return;
#else
    string_path S;
    FS.update_path(S, "$game_config$", "script.ltx");
    CInifile* l_tpIniFile = new CInifile(S);
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
    for (u32 i = 0; i < registratorCount; i++)
    {
        _GetItem(*registrators, i, I);
        luabind::functor<void> result;
        if (!GEnv.ScriptEngine->functor(I, result))
        {
            GEnv.ScriptEngine->script_log(LuaMessageType::Error, "Cannot load class registrator %s!", I);
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
    CInifile* l_tpIniFile = new CInifile(S);
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
        for (u32 i = 0; i < scriptCount; i++)
        {
            _GetItem(*scriptString, i, scriptName);
            GEnv.ScriptEngine->load_file(scriptName, CScriptEngine::GlobalNamespace);
        }
    }
    xr_delete(l_tpIniFile);
#endif
}

void CAI_Space::SetupScriptEngine()
{
    XRay::ScriptExporter::Reset(); // mark all nodes as undone
    GEnv.ScriptEngine->init(XRay::ScriptExporter::Export, true);
    RegisterScriptClasses();
    object_factory().register_script();
    LoadCommonScripts();
}

void CAI_Space::RestartScriptEngine()
{
    if (GEnv.ScriptEngine != nullptr)
    {
        m_events_notifier.FireEvent(EVENT_SCRIPT_ENGINE_RESET);
    }

    SetupScriptEngine();
#ifdef DEBUG
    get_moving_objects().clear();
#endif // DEBUG

    if (GEnv.ScriptEngine != nullptr)
    {
        m_events_notifier.FireEvent(EVENT_SCRIPT_ENGINE_STARTED);
    }
}

void CAI_Space::load(LPCSTR level_name)
{
    VERIFY(m_game_graph);

    unload(true);

#ifdef DEBUG
    Memory.mem_compact();
    u32 mem_usage = Memory.mem_usage();
    CTimer timer;
    timer.Start();
#endif
    AISpaceBase::Load(level_name);
    m_cover_manager->compute_static_cover();
    m_moving_objects->on_level_load();

    m_doors_manager.reset(new ::doors::manager(level_graph().header().box()));

#ifdef DEBUG
    Msg("* Loading ai space is successfully completed (%.3fs, %7.3f Mb)", timer.GetElapsed_sec(),
        float(Memory.mem_usage() - mem_usage) / 1048576.0);
#endif
}

void CAI_Space::unload(bool reload)
{
    if (GEnv.isDedicatedServer)
        return;
    GEnv.ScriptEngine->unload();
    m_doors_manager.reset(nullptr);
    AISpaceBase::Unload(reload);
}

void CAI_Space::set_alife(CALifeSimulator* alife_simulator)
{
    VERIFY((!m_alife_simulator && alife_simulator) || (m_alife_simulator && !alife_simulator));
    m_alife_simulator = alife_simulator;

    VERIFY(!alife_simulator || !m_game_graph);
    if (alife_simulator)
        return;
    SetGameGraph(nullptr);
}

bool CAI_Space::Unsubscribe(CEventNotifierCallback::CID cid, EEventID event_id)
{
    return m_events_notifier.UnregisterCallback(cid, event_id);
}
