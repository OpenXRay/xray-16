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

// Nifty counter required to solve initilization order problem
// Please, do NOT initialize this static members directly, it breaks the initialization logic
// And yes, according to the standard ($3.6.2/1): "Objects with static storage duration (3.7.1) shall be
// zero-initialized (8.5) before any other initialization takes place" So, it must be automatically
// initialized with zeros before executing the constructor
static CAI_Space* s_ai_space;
static u32 s_nifty_counter;

SAI_Space_Initializer::SAI_Space_Initializer()
{
    if (s_nifty_counter++ == 0)
    {
        s_ai_space = new CAI_Space();
    }
}

SAI_Space_Initializer::~SAI_Space_Initializer()
{
    if (--s_nifty_counter == 0)
    {
        xr_delete(s_ai_space);
    }
}

CAI_Space& CAI_Space::GetInstance()
{
    VERIFY(s_ai_space);

    auto& instance = *s_ai_space;
    if (!instance.m_inited)
    {
        instance.init();
    }
    return instance;
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
