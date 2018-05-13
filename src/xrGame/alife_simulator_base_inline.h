////////////////////////////////////////////////////////////////////////////
//  Module      : alife_simulator_base_inline.h
//  Created     : 25.12.2002
//  Modified    : 12.05.2004
//  Author      : Dmitriy Iassenev
//  Description : ALife Simulator base inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once
#include "alife_simulator_base.h"

#ifdef DEBUG // for debug builds, the functions are instantiated by alife_simulator_base.cpp
#define ASB_INL
#else
#define ASB_INL inline
#endif

ASB_INL const CALifeSimulatorHeader& CALifeSimulatorBase::header() const
{
    VERIFY(initialized());
    VERIFY(m_header);
    return *m_header;
}

ASB_INL const CALifeTimeManager& CALifeSimulatorBase::time() const
{
    VERIFY(initialized());
    VERIFY(m_time_manager);
    return *m_time_manager;
}

ASB_INL const CALifeSpawnRegistry& CALifeSimulatorBase::spawns() const
{
    VERIFY(initialized());
    VERIFY(m_spawns);
    return *m_spawns;
}

ASB_INL const CALifeObjectRegistry& CALifeSimulatorBase::objects() const
{
    VERIFY(initialized());
    VERIFY(m_objects);
    return *m_objects;
}

ASB_INL const CALifeGraphRegistry& CALifeSimulatorBase::graph() const
{
    VERIFY(initialized());
    VERIFY(m_graph_objects);
    return *m_graph_objects;
}

ASB_INL const CALifeScheduleRegistry& CALifeSimulatorBase::scheduled() const
{
    VERIFY(initialized());
    VERIFY(m_scheduled);
    return *m_scheduled;
}

ASB_INL const CALifeStoryRegistry& CALifeSimulatorBase::story_objects() const
{
    VERIFY(initialized());
    VERIFY(m_story_objects);
    return *m_story_objects;
}

ASB_INL const CALifeSmartTerrainRegistry& CALifeSimulatorBase::smart_terrains() const
{
    VERIFY(initialized());
    VERIFY(m_smart_terrains);
    return *m_smart_terrains;
}

ASB_INL const CALifeGroupRegistry& CALifeSimulatorBase::groups() const
{
    VERIFY(initialized());
    VERIFY(m_groups);
    return *m_groups;
}

ASB_INL CALifeSimulatorHeader& CALifeSimulatorBase::header()
{
    VERIFY(initialized());
    VERIFY(m_header);
    return *m_header;
}

ASB_INL CALifeTimeManager& CALifeSimulatorBase::time()
{
    VERIFY(initialized());
    VERIFY(m_time_manager);
    return *m_time_manager;
}

ASB_INL CALifeSpawnRegistry& CALifeSimulatorBase::spawns()
{
    VERIFY(initialized());
    VERIFY(m_spawns);
    return *m_spawns;
}

ASB_INL CALifeObjectRegistry& CALifeSimulatorBase::objects()
{
    VERIFY(initialized());
    VERIFY(m_objects);
    return *m_objects;
}

ASB_INL CALifeGraphRegistry& CALifeSimulatorBase::graph()
{
    VERIFY(initialized());
    VERIFY(m_graph_objects);
    return *m_graph_objects;
}

ASB_INL CALifeScheduleRegistry& CALifeSimulatorBase::scheduled()
{
    VERIFY(initialized());
    VERIFY(m_scheduled);
    return *m_scheduled;
}

ASB_INL CALifeStoryRegistry& CALifeSimulatorBase::story_objects()
{
    VERIFY(initialized());
    VERIFY(m_story_objects);
    return *m_story_objects;
}

ASB_INL CALifeSmartTerrainRegistry& CALifeSimulatorBase::smart_terrains()
{
    VERIFY(initialized());
    VERIFY(m_smart_terrains);
    return *m_smart_terrains;
}

ASB_INL CALifeGroupRegistry& CALifeSimulatorBase::groups()
{
    VERIFY(initialized());
    VERIFY(m_groups);
    return *m_groups;
}

ASB_INL CALifeRegistryContainer& CALifeSimulatorBase::registry() const
{
    VERIFY(initialized());
    VERIFY(m_registry_container);
    return *m_registry_container;
}

ASB_INL inventory::upgrade::Manager& CALifeSimulatorBase::inventory_upgrade_manager() const
{
    VERIFY(initialized());
    VERIFY(m_upgrade_manager);
    return *m_upgrade_manager;
}

ASB_INL CRandom32& CALifeSimulatorBase::random() { return m_random; }
ASB_INL void CALifeSimulatorBase::setup_command_line(shared_str* command_line)
{ m_server_command_line = command_line; }

ASB_INL IPureServer& CALifeSimulatorBase::server() const
{
    VERIFY(m_server);
    return *m_server;
}

ASB_INL CALifeTimeManager& CALifeSimulatorBase::time_manager()
{
    VERIFY(initialized());
    VERIFY(m_time_manager);
    return *m_time_manager;
}

ASB_INL const CALifeTimeManager& CALifeSimulatorBase::time_manager() const
{
    VERIFY(initialized());
    VERIFY(m_time_manager);
    return *m_time_manager;
}

ASB_INL shared_str* CALifeSimulatorBase::server_command_line() const
{
    VERIFY(m_server_command_line);
    return m_server_command_line;
}

ASB_INL void CALifeSimulatorBase::can_register_objects(bool value)
{
    VERIFY(m_can_register_objects != value);
    m_can_register_objects = value;
}

#undef ASB_INL
