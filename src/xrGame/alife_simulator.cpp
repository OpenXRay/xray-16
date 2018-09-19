////////////////////////////////////////////////////////////////////////////
//  Module      : alife_simulator.cpp
//  Created     : 25.12.2002
//  Modified    : 13.05.2004
//  Author      : Dmitriy Iassenev
//  Description : ALife Simulator
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "alife_simulator.h"
#include "xrServer_Objects_ALife.h"
#include "ai_space.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrScriptEngine/script_engine.hpp"
#include "MainMenu.h"
#include "object_factory.h"
#include "alife_object_registry.h"
#include "xrEngine/XR_IOConsole.h"

#ifdef DEBUG
#include "moving_objects.h"
#endif // DEBUG

LPCSTR alife_section = "alife";

extern void destroy_lua_wpn_params();

CALifeSimulator::CALifeSimulator(IPureServer* server, shared_str* command_line)
    : CALifeUpdateManager(server, alife_section), CALifeInteractionManager(server, alife_section),
      CALifeSimulatorBase(server, alife_section)
{
    // XXX: why do we need to reinitialize script engine?
    if (!strstr(Core.Params, "-keep_lua"))
    {
        destroy_lua_wpn_params();
        MainMenu()->DestroyInternal(true);
        xr_delete(g_object_factory);
        ai().SetupScriptEngine();
#ifdef DEBUG
        ai().get_moving_objects().clear();
#endif // DEBUG
    }

    ai().set_alife(this);

    setup_command_line(command_line);

    typedef IGame_Persistent::params params;
    params& p = g_pGamePersistent->m_game_params;

    R_ASSERT2(xr_strlen(p.m_game_or_spawn) && !xr_strcmp(p.m_alife, "alife") && !xr_strcmp(p.m_game_type, "single"),
        "Invalid server options!");

    string256 temp;
    xr_strcpy(temp, p.m_game_or_spawn);
    xr_strcat(temp, "/");
    xr_strcat(temp, p.m_game_type);
    xr_strcat(temp, "/");
    xr_strcat(temp, p.m_alife);
    *command_line = temp;

    LPCSTR start_game_callback = pSettings->r_string(alife_section, "start_game_callback");
    luabind::functor<void> functor;
    R_ASSERT2(GEnv.ScriptEngine->functor(start_game_callback, functor), "failed to get start game callback");
    functor();

    load(p.m_game_or_spawn, !xr_strcmp(p.m_new_or_load, "load") ? false : true, !xr_strcmp(p.m_new_or_load, "new"));
}

CALifeSimulator::~CALifeSimulator()
{
    VERIFY(!ai().get_alife());

    configs_type::iterator i = m_configs_lru.begin();
    configs_type::iterator const e = m_configs_lru.end();
    for (; i != e; ++i)
        FS.r_close((*i).second);
}

void CALifeSimulator::destroy()
{
    //  validate                    ();
    CALifeUpdateManager::destroy();
    VERIFY(ai().get_alife());
    ai().set_alife(0);
}

void CALifeSimulator::setup_simulator(CSE_ALifeObject* object)
{
    //  VERIFY2                     (!object->m_alife_simulator,object->s_name_replace);
    object->m_alife_simulator = this;
}

void CALifeSimulator::reload(LPCSTR section) { CALifeUpdateManager::reload(section); }
struct string_prdicate
{
    shared_str m_value;

    inline string_prdicate(shared_str const& value) : m_value(value) {}
    inline bool operator()(std::pair<shared_str, IReader*> const& value) const
    {
        return !xr_strcmp(m_value, value.first);
    }
}; // struct string_prdicate

IReader const* CALifeSimulator::get_config(shared_str config) const
{
    configs_type::iterator const found =
        std::find_if(m_configs_lru.begin(), m_configs_lru.end(), string_prdicate(config));
    if (found != m_configs_lru.end())
    {
        configs_type::value_type temp = *found;
        m_configs_lru.erase(found);
        m_configs_lru.insert(m_configs_lru.begin(), std::make_pair(temp.first, temp.second));
        return temp.second;
    }

    string_path file_name;
    FS.update_path(file_name, "$game_config$", config.c_str());
    if (!FS.exist(file_name))
        return 0;

    m_configs_lru.insert(m_configs_lru.begin(), std::make_pair(config, FS.r_open(file_name)));
    return m_configs_lru.front().second;
}

namespace detail
{
bool object_exists_in_alife_registry(u32 id)
{
    if (ai().get_alife())
    {
        return ai().alife().objects().object((ALife::_OBJECT_ID)id, true) != 0;
    }
    return false;
}

} // detail
