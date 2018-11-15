////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_space.h
//	Created 	: 12.11.2003
//  Modified 	: 18.06.2004
//	Author		: Dmitriy Iassenev
//	Description : AI space class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ai_space.h"
#include "xrScriptEngine/script_engine.hpp"
#include "xrServerEntities/object_factory.h"

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
        if (!script_engine().functor(I, result))
        {
            script_engine().script_log(LuaMessageType::Error, "Cannot load class registrator %s!", I);
            continue;
        }
        result(const_cast<CObjectFactory*>(&object_factory()));
    }
#endif
}

void CAI_Space::init()
{
    R_ASSERT(!m_inited);

    VERIFY(!GEnv.ScriptEngine);
    GEnv.ScriptEngine = new CScriptEngine(true);
    XRay::ScriptExporter::Reset(); // mark all nodes as undone
    GEnv.ScriptEngine->init(XRay::ScriptExporter::Export, true);
    RegisterScriptClasses();
    object_factory().register_script();

    m_inited = true;
}

CAI_Space::~CAI_Space() { xr_delete(GEnv.ScriptEngine); }
