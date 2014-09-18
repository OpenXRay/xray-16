////////////////////////////////////////////////////////////////////////////
//	Module 		: level_spawn_constructor_inline.h
//	Created 	: 16.10.2004
//  Modified 	: 16.10.2004
//	Author		: Dmitriy Iassenev
//	Description : Level spawn constructor inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CLevelSpawnConstructor::CLevelSpawnConstructor			(const CGameGraph::SLevel &level, CGameSpawnConstructor *game_spawn_constructor, bool no_separator_check) :
	CThread						(level.id())
{
	m_level						= level;
	m_game_spawn_constructor	= game_spawn_constructor;
	m_no_separator_check		= no_separator_check;
	thDestroyOnComplete			= FALSE;
	m_actor						= 0;
	m_level_graph				= 0;
	m_cross_table				= 0;
	m_graph_engine				= 0;
}

IC	CSE_ALifeCreatureActor *CLevelSpawnConstructor::actor	() const
{
	return						(m_actor);
}

IC	const CGameGraph::SLevel &CLevelSpawnConstructor::level	() const
{
	return						(m_level);
}

IC	CGameSpawnConstructor &CLevelSpawnConstructor::game_spawn_constructor	() const
{
	VERIFY						(m_game_spawn_constructor);
	return						(*m_game_spawn_constructor);
}
