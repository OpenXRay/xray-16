////////////////////////////////////////////////////////////////////////////
//	Module 		: script_process.cpp
//	Created 	: 19.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script process class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "script_engine.hpp"
#include "script_process.hpp"
#include "script_thread.hpp"
#include "Common/object_broker.h"

string4096 g_ca_stdout; // XXX: allocate dynamically for each CScriptEngine instance

CScriptProcess::CScriptProcess(CScriptEngine* scriptEngine, shared_str name, shared_str scripts) : m_name(name)
{
    this->scriptEngine = scriptEngine;
#ifdef DEBUG
    Msg("* Initializing %s script process", *m_name);
#endif
    string256 I;
    for (u32 i = 0, n = _GetItemCount(*scripts); i < n; i++)
        add_script(_GetItem(*scripts, i, I), false, false);
    m_iterator = 0;
}

CScriptProcess::~CScriptProcess() { delete_data(m_scripts); }
void CScriptProcess::run_scripts()
{
    pstr S;
    for (; !m_scripts_to_run.empty();)
    {
        pstr I = m_scripts_to_run.back().m_script_name;
        bool do_string = m_scripts_to_run.back().m_do_string;
        bool reload = m_scripts_to_run.back().m_reload;
        S = xr_strdup(I);
        m_scripts_to_run.pop_back();

        CScriptThread* script = scriptEngine->CreateScriptThread(S, do_string, reload);
        xr_free(S);

        if (script->active())
            m_scripts.push_back(script);
        else
            xr_delete(script);
    }
}

// Oles:
// changed to process one script per-frame
// changed log-output to stack-based buffer (avoid persistent 4K storage)
void CScriptProcess::update()
{
#ifdef DBG_DISABLE_SCRIPTS
    m_scripts_to_run.clear();
    return;
#else
    run_scripts();
    if (m_scripts.empty())
        return;
    // update script
    g_ca_stdout[0] = 0;
    u32 _id = ++m_iterator % m_scripts.size();
    if (!m_scripts[_id]->update())
    {
        xr_delete(m_scripts[_id]);
        m_scripts.erase(m_scripts.begin() + _id);
        --m_iterator; // try to avoid skipping
    }
    if (g_ca_stdout[0])
    {
        fputc(0, stderr);
        scriptEngine->script_log(LuaMessageType::Info, "%s", g_ca_stdout);
        fflush(stderr);
    }
#if defined(DEBUG)
    try
    {
#pragma todo("Dima cant find this function 'lua_setgcthreshold' ")
        lua_gc(scriptEngine->lua(), LUA_GCSTEP, 0);
    }
    catch (...)
    {
    }
#endif
#endif
}

void CScriptProcess::add_script(LPCSTR script_name, bool do_string, bool reload)
{
    m_scripts_to_run.emplace_back(script_name, do_string, reload);
}
