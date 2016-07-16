////////////////////////////////////////////////////////////////////////////
//  Module      : alife_simulator_base_inline.h
//  Created     : 25.12.2002
//  Modified    : 12.05.2004
//  Author      : Dmitriy Iassenev
//  Description : ALife Simulator base inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once
#include "alife_simulator_base.h"

const CALifeSimulatorHeader& CALifeSimulatorBase::header() const
{
    VERIFY(initialized());
    VERIFY(m_header);
    return *m_header;
}

const CALifeTimeManager& CALifeSimulatorBase::time() const
{
    VERIFY(initialized());
    VERIFY(m_time_manager);
    return *m_time_manager;
}

const CALifeSpawnRegistry& CALifeSimulatorBase::spawns() const
{
    VERIFY(initialized());
    VERIFY(m_spawns);
    return *m_spawns;
}

const CALifeObjectRegistry& CALifeSimulatorBase::objects() const
{
    VERIFY(initialized());
    VERIFY(m_objects);
    return *m_objects;
}

const CALifeGraphRegistry& CALifeSimulatorBase::graph() const
{
    VERIFY(initialized());
    VERIFY(m_graph_objects);
    return *m_graph_objects;
}

const CALifeScheduleRegistry& CALifeSimulatorBase::scheduled() const
{
    VERIFY(initialized());
    VERIFY(m_scheduled);
    return *m_scheduled;
}

const CALifeStoryRegistry& CALifeSimulatorBase::story_objects() const
{
    VERIFY(initialized());
    VERIFY(m_story_objects);
    return *m_story_objects;
}

const CALifeSmartTerrainRegistry& CALifeSimulatorBase::smart_terrains() const
{
    VERIFY(initialized());
    VERIFY(m_smart_terrains);
    return *m_smart_terrains;
}

const CALifeGroupRegistry& CALifeSimulatorBase::groups() const
{
    VERIFY(initialized());
    VERIFY(m_groups);
    return *m_groups;
}

CALifeSimulatorHeader& CALifeSimulatorBase::header()
{
    VERIFY(initialized());
    VERIFY(m_header);
    return *m_header;
}

CALifeTimeManager& CALifeSimulatorBase::time()
{
    VERIFY(initialized());
    VERIFY(m_time_manager);
    return *m_time_manager;
}

CALifeSpawnRegistry& CALifeSimulatorBase::spawns()
{
    VERIFY(initialized());
    VERIFY(m_spawns);
    return *m_spawns;
}

CALifeObjectRegistry& CALifeSimulatorBase::objects()
{
    VERIFY(initialized());
    VERIFY(m_objects);
    return *m_objects;
}

CALifeGraphRegistry& CALifeSimulatorBase::graph()
{
    VERIFY(initialized());
    VERIFY(m_graph_objects);
    return *m_graph_objects;
}

CALifeScheduleRegistry& CALifeSimulatorBase::scheduled()
{
    VERIFY(initialized());
    VERIFY(m_scheduled);
    return *m_scheduled;
}

CALifeStoryRegistry& CALifeSimulatorBase::story_objects()
{
    VERIFY(initialized());
    VERIFY(m_story_objects);
    return *m_story_objects;
}

CALifeSmartTerrainRegistry& CALifeSimulatorBase::smart_terrains()
{
    VERIFY(initialized());
    VERIFY(m_smart_terrains);
    return *m_smart_terrains;
}

CALifeGroupRegistry& CALifeSimulatorBase::groups()
{
    VERIFY(initialized());
    VERIFY(m_groups);
    return *m_groups;
}

CALifeRegistryContainer& CALifeSimulatorBase::registry() const
{
    VERIFY(initialized());
    VERIFY(m_registry_container);
    return *m_registry_container;
}

inventory::upgrade::Manager& CALifeSimulatorBase::inventory_upgrade_manager() const
{
    VERIFY(initialized());
    VERIFY(m_upgrade_manager);
    return *m_upgrade_manager;
}

CRandom32& CALifeSimulatorBase::random() { return (m_random); }
void CALifeSimulatorBase::setup_command_line(shared_str* command_line) { m_server_command_line = command_line; }

IPureServer& CALifeSimulatorBase::server() const
{
    VERIFY(m_server);
    return *m_server;
}

CALifeTimeManager& CALifeSimulatorBase::time_manager()
{
    VERIFY(initialized());
    VERIFY(m_time_manager);
    return *m_time_manager;
}

const CALifeTimeManager& CALifeSimulatorBase::time_manager() const
{
    VERIFY(initialized());
    VERIFY(m_time_manager);
    return *m_time_manager;
}

shared_str* CALifeSimulatorBase::server_command_line() const
{
    VERIFY(m_server_command_line);
    return m_server_command_line;
}

void CALifeSimulatorBase::can_register_objects(bool value)
{
    VERIFY(m_can_register_objects != value);
    m_can_register_objects = value;
}
