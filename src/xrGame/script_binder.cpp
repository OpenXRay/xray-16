////////////////////////////////////////////////////////////////////////////
//	Module 		: script_binder.cpp
//	Created 	: 26.03.2004
//  Modified 	: 26.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Script objects binder
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "ai_space.h"
#include "xrScriptEngine/script_engine.hpp"
#include "script_binder.h"
#include "xrServer_Objects_ALife.h"
#include "script_binder_object.h"
#include "script_game_object.h"
#include "GameObject.h"
#include "Level.h"

// comment next string when commiting
//#define DBG_DISABLE_SCRIPTS

CScriptBinder::CScriptBinder(CGameObject* owner)
{
    this->owner = owner;
    init();
}

CScriptBinder::~CScriptBinder() { VERIFY(!m_object); }
void CScriptBinder::init() { m_object = 0; }
void CScriptBinder::clear()
{
    try
    {
        xr_delete(m_object);
    }
    catch (...)
    {
        m_object = 0;
    }
    init();
}

void CScriptBinder::reinit()
{
    if (m_object)
    {
        try
        {
            m_object->reinit();
        }
        catch (...)
        {
            clear();
        }
    }
}

void CScriptBinder::reload(LPCSTR section)
{
#ifndef DBG_DISABLE_SCRIPTS
    VERIFY(!m_object);
    if (!pSettings->line_exist(section, "script_binding"))
        return;

    luabind::functor<void> lua_function;
    if (!GEnv.ScriptEngine->functor(pSettings->r_string(section, "script_binding"), lua_function))
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "function %s is not loaded!", pSettings->r_string(section, "script_binding"));
        return;
    }

    try
    {
        lua_function(owner->lua_game_object());
    }
    catch (...)
    {
        clear();
        return;
    }

    if (m_object)
    {
        try
        {
            m_object->reload(section);
        }
        catch (...)
        {
            clear();
        }
    }
#endif
}

BOOL CScriptBinder::net_Spawn(CSE_Abstract* DC)
{
    CSE_Abstract* abstract = (CSE_Abstract*)DC;
    CSE_ALifeObject* object = smart_cast<CSE_ALifeObject*>(abstract);
    if (object && m_object)
    {
        try
        {
            return ((BOOL)m_object->net_Spawn(object));
        }
        catch (...)
        {
            clear();
        }
    }

    return (TRUE);
}

void CScriptBinder::net_Destroy()
{
    if (m_object)
    {
#ifdef _DEBUG
        Msg("* Core object %s is UNbinded from the script object", owner->cName());
#endif // _DEBUG
        try
        {
            m_object->net_Destroy();
        }
        catch (...)
        {
            clear();
        }
    }
    xr_delete(m_object);
}

void CScriptBinder::set_object(CScriptBinderObject* object)
{
    if (IsGameTypeSingle())
    {
        VERIFY2(!m_object, "Cannot bind to the object twice!");
#ifdef _DEBUG
        Msg("* Core object %s is binded with the script object", owner->cName());
#endif // _DEBUG
        m_object = object;
    }
    else
    {
        xr_delete(object);
    }
}

void CScriptBinder::shedule_Update(u32 time_delta)
{
    if (m_object)
    {
        try
        {
            m_object->shedule_Update(time_delta);
        }
        catch (...)
        {
            clear();
        }
    }
}

void CScriptBinder::save(NET_Packet& output_packet)
{
    if (m_object)
    {
        try
        {
            m_object->save(&output_packet);
        }
        catch (...)
        {
            clear();
        }
    }
}

void CScriptBinder::load(IReader& input_packet)
{
    if (m_object)
    {
        try
        {
            m_object->load(&input_packet);
        }
        catch (...)
        {
            clear();
        }
    }
}

BOOL CScriptBinder::net_SaveRelevant()
{
    if (m_object)
    {
        try
        {
            return (m_object->net_SaveRelevant());
        }
        catch (...)
        {
            clear();
        }
    }
    return (FALSE);
}

void CScriptBinder::net_Relcase(IGameObject* object)
{
    CGameObject* game_object = smart_cast<CGameObject*>(object);
    if (m_object && game_object)
    {
        try
        {
            m_object->net_Relcase(game_object->lua_game_object());
        }
        catch (...)
        {
            clear();
        }
    }
}
