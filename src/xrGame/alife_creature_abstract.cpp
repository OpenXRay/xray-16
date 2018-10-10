////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_creature_abstract.cpp
//	Created 	: 27.10.2005
//  Modified 	: 27.10.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife creature abstract class
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "monster_community.h"
#include "Level.h"
#include "ai_space.h"
#include "alife_simulator.h"
#include "alife_time_manager.h"

void CSE_ALifeCreatureAbstract::on_spawn()
{
    inherited::on_spawn();

    m_dynamic_out_restrictions.clear();
    m_dynamic_in_restrictions.clear();

    if (smart_cast<CSE_ALifeGroupAbstract*>(this))
        return;

    MONSTER_COMMUNITY monster_community;
    monster_community.set(pSettings->r_string(s_name, "species"));
    if (monster_community.team() != 255)
        s_team = monster_community.team();

    if (!g_Alive())
        m_game_death_time = 0; // alife().time_manager().game_time();
}

void CSE_ALifeCreatureActor::add_online(const bool& update_registries)
{
    CSE_ALifeTraderAbstract::add_online(update_registries);
}

void CSE_ALifeCreatureActor::add_offline(
    const xr_vector<ALife::_OBJECT_ID>& saved_children, const bool& update_registries)
{
    CSE_ALifeTraderAbstract::add_offline(saved_children, update_registries);
}
