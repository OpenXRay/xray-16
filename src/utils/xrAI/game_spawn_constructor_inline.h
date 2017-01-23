////////////////////////////////////////////////////////////////////////////
//	Module 		: game_spawn_constructor_inline.h
//	Created 	: 16.10.2004
//  Modified 	: 16.10.2004
//	Author		: Dmitriy Iassenev
//	Description : Game spawn constructor inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

extern u32	dwfGetIDByLevelName	(CInifile *ini, LPCSTR level_name);

IC	CGameGraph &CGameSpawnConstructor::game_graph		() const
{
	return						(*m_game_graph);
}

IC	CInifile &CGameSpawnConstructor::game_info			()
{
	return						(*m_game_info);
}

IC	CGameSpawnConstructor::SPAWN_GRAPH &CGameSpawnConstructor::spawn_graph	()
{
	return						(*m_spawn_graph);
}

IC	u32	CGameSpawnConstructor::level_id					(LPCSTR level_name)
{
	return						(dwfGetIDByLevelName(&game_info(),level_name));
}

IC	ALife::_SPAWN_ID CGameSpawnConstructor::spawn_id	()
{
	return						(m_spawn_id++);
}

IC	void CGameSpawnConstructor::add_level_points		(const LEVEL_POINT_STORAGE &level_points)
{
	m_level_points.insert		(m_level_points.end(),level_points.begin(),level_points.end());
}

IC	void CGameSpawnConstructor::add_level_changer		(CSE_ALifeLevelChanger *level_changer)
{
	m_critical_section.Enter	();
	m_level_changers.push_back	(level_changer);
	m_critical_section.Leave	();
}

IC	void CGameSpawnConstructor::add_edge				(ALife::_SPAWN_ID id0, ALife::_SPAWN_ID id1, float weight)
{
	m_critical_section.Enter	();
	spawn_graph().add_edge		(id0,id1,weight);
	m_critical_section.Leave	();
}

IC	u32	CGameSpawnConstructor::level_point_count		() const
{
	return						((u32)m_level_points.size());
}

IC	CGameSpawnConstructor::LEVEL_CHANGER_STORAGE &CGameSpawnConstructor::level_changers	()
{
	return						(m_level_changers);
}

IC	CPatrolPathStorage &CGameSpawnConstructor::patrol_path_storage	() const
{
	VERIFY						(m_patrol_path_storage);
	return						(*m_patrol_path_storage);
}

IC	void CGameSpawnConstructor::process_spawns		(xr_vector<ALife::_SPAWN_ID> &spawns)
{
	std::sort								(spawns.begin(),spawns.end());
	xr_vector<ALife::_SPAWN_ID>::iterator	I = std::unique(spawns.begin(),spawns.end());
	spawns.erase							(I,spawns.end());
}
