////////////////////////////////////////////////////////////////////////////
//	Module 		: agent_explosive_manager.cpp
//	Created 	: 24.05.2004
//  Modified 	: 14.01.2005
//	Author		: Dmitriy Iassenev
//	Description : Agent explosive manager
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "agent_explosive_manager.h"
#include "agent_manager.h"
#include "agent_location_manager.h"
#include "agent_member_manager.h"
#include "Missile.h"
#include "Explosive.h"
#include "member_order.h"
#include "ai/stalker/ai_stalker.h"
#include "memory_manager.h"
#include "visual_memory_manager.h"
#include "danger_object_location.h"

const float GRENADE_RADIUS = 10.f;
const u32 AFTER_GRENADE_DESTROYED_INTERVAL = 1000;

struct CRemoveExplosivesPredicate
{
    IC bool operator()(CDangerExplosive& explosive) const { return (!!explosive.m_reactor); }
};

void CAgentExplosiveManager::remove_links(IGameObject* object)
{
    TO_BE_DESTROYED::iterator I = std::find(m_explosives_to_remove.begin(), m_explosives_to_remove.end(), object->ID());
    if (I != m_explosives_to_remove.end())
        m_explosives_to_remove.erase(I);

    EXPLOSIVES::iterator J = std::find(m_explosives.begin(), m_explosives.end(), object->ID());
    if (J != m_explosives.end())
        m_explosives.erase(J);
}

void CAgentExplosiveManager::register_explosive(const CExplosive* explosive, const CGameObject* game_object)
{
    {
        xr_vector<CDangerExplosive>::iterator I = std::find(m_explosives.begin(), m_explosives.end(), explosive);
        if (I != m_explosives.end())
            return;
    }
    {
        TO_BE_DESTROYED::iterator I =
            std::find(m_explosives_to_remove.begin(), m_explosives_to_remove.end(), game_object->ID());
        if (I != m_explosives_to_remove.end())
            return;
    }

    m_explosives_to_remove.push_back(game_object->ID());
    m_explosives.push_back(CDangerExplosive(explosive, game_object, 0, Device.dwTimeGlobal));

    u32 interval = AFTER_GRENADE_DESTROYED_INTERVAL;
    const CMissile* missile = smart_cast<const CMissile*>(explosive);
    if (missile && (missile->destroy_time() > Device.dwTimeGlobal))
        interval = missile->destroy_time() - Device.dwTimeGlobal + AFTER_GRENADE_DESTROYED_INTERVAL;

    object().location().add(new CDangerObjectLocation(game_object, Device.dwTimeGlobal, interval, GRENADE_RADIUS));
}

bool CAgentExplosiveManager::process_explosive(CMemberOrder& member)
{
    float min_dist_sqr = flt_max;
    CDangerExplosive* best_grenade = 0;
    xr_vector<CDangerExplosive>::iterator I = m_explosives.begin();
    xr_vector<CDangerExplosive>::iterator E = m_explosives.end();
    for (; I != E; ++I)
    {
        if (!member.object().memory().visual().visible_now((*I).m_game_object))
            continue;

        float dist_sqr = (*I).m_game_object->Position().distance_to_sqr(member.object().Position());
        if (dist_sqr < min_dist_sqr)
        {
            if ((*I).m_reactor &&
                ((*I).m_reactor->Position().distance_to_sqr((*I).m_game_object->Position()) <= min_dist_sqr))
                continue;
            min_dist_sqr = dist_sqr;
            best_grenade = &*I;
        }
    }

    if (!best_grenade)
        return (false);

    best_grenade->m_reactor = &member.object();
    return (true);
}

void CAgentExplosiveManager::react_on_explosives()
{
    for (;;)
    {
        bool changed = false;
        CAgentMemberManager::iterator I = object().member().combat_members().begin();
        CAgentMemberManager::iterator E = object().member().combat_members().end();
        for (; I != E; ++I)
            if (!(*I)->grenade_reaction().m_processing)
                changed = process_explosive(**I);

        if (!changed)
            break;
    }

    {
        EXPLOSIVES::iterator I = m_explosives.begin();
        EXPLOSIVES::iterator E = m_explosives.end();
        for (; I != E; ++I)
        {
            if (!(*I).m_reactor)
                continue;

            CMemberOrder::CGrenadeReaction& reaction = object().member().member((*I).m_reactor).grenade_reaction();
            reaction.m_grenade = (*I).m_grenade;
            reaction.m_game_object = (*I).m_game_object;
            reaction.m_time = (*I).m_time;
            reaction.m_processing = true;
        }

        m_explosives.erase(
            std::remove_if(m_explosives.begin(), m_explosives.end(), CRemoveExplosivesPredicate()), m_explosives.end());
    }
}

void CAgentExplosiveManager::update() {}
