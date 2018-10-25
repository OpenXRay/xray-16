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

//----------------------- Event processing-----------------------

CAI_Space::CEventCallback::CID CAI_Space::CEventCallbackStorage::RegisterCallback(CEventCallback* cb)
{
    m_lock.lock();

    size_t i, cb_count = m_callbacks.size();

    for (i = 0; i < cb_count; ++i)
    {
        if (!m_callbacks[i])
        {
            break;
        }
    }

    if (i == cb_count)
    {
        m_callbacks.resize(cb_count + 1);
    }

    m_callbacks[i].reset(cb);

    m_lock.unlock();
    return i;
}
bool CAI_Space::CEventCallbackStorage::UnregisterCallback(CEventCallback::CID cid)
{
    bool result = false;
    m_lock.lock();

    if (cid < m_callbacks.size() && m_callbacks[cid])
    {
        m_callbacks[cid].reset(nullptr);
        result = true;
    }

    m_lock.unlock();
    return result;
}

void CAI_Space::CEventCallbackStorage::ExecuteCallbacks()
{
    m_lock.lock();

    for (auto& cb : m_callbacks)
    {
        if (cb)
        {
            cb->ProcessEvent();
        }
    }

    m_lock.unlock();
}

CAI_Space::CEventCallback::CID CAI_Space::CNotifier::RegisterCallback(CEventCallback* cb, EEventID event_id)
{
    R_ASSERT(event_id < EVENT_COUNT);
    return m_callbacks[event_id].RegisterCallback(cb);
}

bool CAI_Space::CNotifier::UnregisterCallback(CEventCallback::CID cid, EEventID event_id)
{
    R_ASSERT(event_id < EVENT_COUNT);
    return m_callbacks[event_id].UnregisterCallback(cid);
}

void CAI_Space::CNotifier::FireEvent(EEventID event_id)
{
    R_ASSERT(event_id < EVENT_COUNT);
    m_callbacks[event_id].ExecuteCallbacks();
}

//----------------------- Main CAI_Space stuff-----------------------

static CAI_Space g_ai_space;

CAI_Space& CAI_Space::GetInstance()
{
    auto& instance = g_ai_space;
    if (!instance.m_inited)
    {
        instance.init();
    }
    return instance;
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
    m_events_notifier.FireEvent(CNotifier::EVENT_SCRIPT_ENGINE_RESET);
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
        m_events_notifier.FireEvent(CNotifier::EVENT_SCRIPT_ENGINE_RESET);
    }

    SetupScriptEngine();
#ifdef DEBUG
    get_moving_objects().clear();
#endif // DEBUG

    m_events_notifier.FireEvent(CNotifier::EVENT_SCRIPT_ENGINE_STARTED);
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

CAI_Space::CEventCallback::CID CAI_Space::Subscribe(CEventCallback* cb, CNotifier::EEventID event_id)
{
    return m_events_notifier.RegisterCallback(cb, event_id);
}

bool CAI_Space::Unsubscribe(CAI_Space::CEventCallback::CID cid, CNotifier::EEventID event_id)
{
    return m_events_notifier.UnregisterCallback(cid, event_id);
}
