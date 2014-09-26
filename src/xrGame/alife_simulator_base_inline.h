////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_simulator_base_inline.h
//	Created 	: 25.12.2002
//  Modified 	: 12.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife Simulator base inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	bool								CALifeSimulatorBase::initialized			() const
{
	return						(m_initialized);
}

IC	const CALifeSimulatorHeader			&CALifeSimulatorBase::header				() const
{
	VERIFY						(initialized());
	VERIFY						(m_header);
	return						(*m_header);
}

IC	const CALifeTimeManager				&CALifeSimulatorBase::time					() const
{
	VERIFY						(initialized());
	VERIFY						(m_time_manager);
	return						(*m_time_manager);
}

IC	const CALifeSpawnRegistry			&CALifeSimulatorBase::spawns				() const
{
	VERIFY						(initialized());
	VERIFY						(m_spawns);
	return						(*m_spawns);
}

IC	const CALifeObjectRegistry			&CALifeSimulatorBase::objects				() const
{
	VERIFY						(initialized());
	VERIFY						(m_objects);
	return						(*m_objects);
}

IC	const CALifeGraphRegistry			&CALifeSimulatorBase::graph					() const
{
	VERIFY						(initialized());
	VERIFY						(m_graph_objects);
	return						(*m_graph_objects);
}

IC	const CALifeScheduleRegistry		&CALifeSimulatorBase::scheduled				() const
{
	VERIFY						(initialized());
	VERIFY						(m_scheduled);
	return						(*m_scheduled);
}

IC	const CALifeStoryRegistry			&CALifeSimulatorBase::story_objects			() const
{
	VERIFY						(initialized());
	VERIFY						(m_story_objects);
	return						(*m_story_objects);
}

IC	const CALifeSmartTerrainRegistry	&CALifeSimulatorBase::smart_terrains		() const
{
	VERIFY						(initialized());
	VERIFY						(m_smart_terrains);
	return						(*m_smart_terrains);
}

IC	const CALifeGroupRegistry			&CALifeSimulatorBase::groups				() const
{
	VERIFY						(initialized());
	VERIFY						(m_groups);
	return						(*m_groups);
}

IC	CALifeSimulatorHeader				&CALifeSimulatorBase::header				()
{
	VERIFY						(initialized());
	VERIFY						(m_header);
	return						(*m_header);
}

IC	CALifeTimeManager					&CALifeSimulatorBase::time					()
{
	VERIFY						(initialized());
	VERIFY						(m_time_manager);
	return						(*m_time_manager);
}

IC	CALifeSpawnRegistry					&CALifeSimulatorBase::spawns				()
{
	VERIFY						(initialized());
	VERIFY						(m_spawns);
	return						(*m_spawns);
}

IC	CALifeObjectRegistry				&CALifeSimulatorBase::objects				()
{
	VERIFY						(initialized());
	VERIFY						(m_objects);
	return						(*m_objects);
}

IC	CALifeGraphRegistry					&CALifeSimulatorBase::graph					()
{
	VERIFY						(initialized());
	VERIFY						(m_graph_objects);
	return						(*m_graph_objects);
}

IC	CALifeScheduleRegistry				&CALifeSimulatorBase::scheduled				()
{
	VERIFY						(initialized());
	VERIFY						(m_scheduled);
	return						(*m_scheduled);
}

IC	CALifeStoryRegistry					&CALifeSimulatorBase::story_objects			()
{
	VERIFY						(initialized());
	VERIFY						(m_story_objects);
	return						(*m_story_objects);
}

IC	CALifeSmartTerrainRegistry			&CALifeSimulatorBase::smart_terrains		()
{
	VERIFY						(initialized());
	VERIFY						(m_smart_terrains);
	return						(*m_smart_terrains);
}

IC	CALifeGroupRegistry					&CALifeSimulatorBase::groups				()
{
	VERIFY						(initialized());
	VERIFY						(m_groups);
	return						(*m_groups);
}

IC	CALifeRegistryContainer		&CALifeSimulatorBase::registry						() const
{
	VERIFY						(initialized());
	VERIFY						(m_registry_container);
	return						(*m_registry_container);
}

IC	inventory::upgrade::Manager	&CALifeSimulatorBase::inventory_upgrade_manager		() const
{
	VERIFY						(initialized());
	VERIFY						(m_upgrade_manager);
	return						(*m_upgrade_manager);
}

IC	CRandom32							&CALifeSimulatorBase::random				()
{
	return						(m_random);
}

IC	void								CALifeSimulatorBase::setup_command_line		(shared_str *command_line)
{
	m_server_command_line		= command_line;
}

IC	xrServer							&CALifeSimulatorBase::server				() const
{
	VERIFY						(m_server);
	return						(*m_server);
}

IC	CALifeTimeManager					&CALifeSimulatorBase::time_manager			()
{
	VERIFY						(initialized());
	VERIFY						(m_time_manager);
	return						(*m_time_manager);
}

IC	const CALifeTimeManager				&CALifeSimulatorBase::time_manager			() const
{
	VERIFY						(initialized());
	VERIFY						(m_time_manager);
	return						(*m_time_manager);
}

IC	shared_str							*CALifeSimulatorBase::server_command_line	() const
{
	VERIFY						(m_server_command_line);
	return						(m_server_command_line);
}

template <typename T>
IC	T									&CALifeSimulatorBase::registry				(T *t) const
{
	return						(registry()(t));
}

IC	void								CALifeSimulatorBase::can_register_objects	(const bool &value)
{
	VERIFY						(m_can_register_objects != value);
	m_can_register_objects		= value;
}

IC	const bool							&CALifeSimulatorBase::can_register_objects	() const
{
	return						(m_can_register_objects);
}
