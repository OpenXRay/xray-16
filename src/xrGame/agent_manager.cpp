////////////////////////////////////////////////////////////////////////////
//	Module 		: agent_manager.cpp
//	Created 	: 24.05.2004
//  Modified 	: 24.05.2004
//	Author		: Dmitriy Iassenev
//	Description : Agent manager
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "agent_manager.h"
#include "agent_corpse_manager.h"
#include "agent_enemy_manager.h"
#include "agent_explosive_manager.h"
#include "agent_location_manager.h"
#include "agent_member_manager.h"
#include "agent_memory_manager.h"
#include "agent_manager_planner.h"
#include "xrEngine/profiler.h"

CAgentManager::CAgentManager()
{
    init_scheduler();
    init_components();
}

CAgentManager::~CAgentManager()
{
    VERIFY(member().members().empty());
#ifdef USE_SCHEDULER_IN_AGENT_MANAGER
    remove_scheduler();
#endif // USE_SCHEDULER_IN_AGENT_MANAGER
    remove_components();
}

void CAgentManager::init_scheduler()
{
#ifdef USE_SCHEDULER_IN_AGENT_MANAGER
    shedule.t_min = 1000;
    shedule.t_max = 1000;
    shedule_register();
#else // USE_SCHEDULER_IN_AGENT_MANAGER
    m_last_update_time = 0;
    m_update_rate = 1000;
#endif // USE_SCHEDULER_IN_AGENT_MANAGER
}

void CAgentManager::init_components()
{
    m_corpse = new CAgentCorpseManager(this);
    m_enemy = new CAgentEnemyManager(this);
    m_explosive = new CAgentExplosiveManager(this);
    m_location = new CAgentLocationManager(this);
    m_member = new CAgentMemberManager(this);
    m_memory = new CAgentMemoryManager(this);
    m_brain = new CAgentManagerPlanner();
    brain().setup(this);
}

#ifdef USE_SCHEDULER_IN_AGENT_MANAGER
void CAgentManager::remove_scheduler() { shedule_unregister(); }
#endif // USE_SCHEDULER_IN_AGENT_MANAGER

void CAgentManager::remove_components()
{
    xr_delete(m_corpse);
    xr_delete(m_enemy);
    xr_delete(m_explosive);
    xr_delete(m_location);
    xr_delete(m_member);
    xr_delete(m_memory);
    xr_delete(m_brain);
}

void CAgentManager::remove_links(IGameObject* object)
{
    corpse().remove_links(object);
    enemy().remove_links(object);
    explosive().remove_links(object);
    location().remove_links(object);
    member().remove_links(object);
    memory().remove_links(object);
    brain().remove_links(object);
}

void CAgentManager::update_impl()
{
    VERIFY(!member().members().empty());

    memory().update();
    corpse().update();
    enemy().update();
    explosive().update();
    location().update();
    member().update();
    brain().update();
}

#ifdef USE_SCHEDULER_IN_AGENT_MANAGER
void CAgentManager::shedule_Update(u32 time_delta)
{
    START_PROFILE("Agent_Manager")

    ScheduledBase::shedule_Update(time_delta);

    update_impl();

    STOP_PROFILE
}

float CAgentManager::shedule_Scale() { return (.5f); }
#else // USE_SCHEDULER_IN_AGENT_MANAGER

void CAgentManager::update()
{
    if (Device.dwTimeGlobal <= m_last_update_time)
        return;

    if (Device.dwTimeGlobal - m_last_update_time < m_update_rate)
        return;

    m_last_update_time = Device.dwTimeGlobal;
    update_impl();
}

#endif // USE_SCHEDULER_IN_AGENT_MANAGER
