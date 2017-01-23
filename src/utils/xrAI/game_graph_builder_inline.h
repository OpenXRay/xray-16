////////////////////////////////////////////////////////////////////////////
//	Module 		: game_graph_builder_inline.h
//	Created 	: 14.12.2005
//  Modified 	: 14.12.2005
//	Author		: Dmitriy Iassenev
//	Description : Game graph builder inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CLevelGraph	&CGameGraphBuilder::level_graph				() const
{
	VERIFY	(m_level_graph);
	return	(*m_level_graph);
}

IC	CGameGraphBuilder::graph_type &CGameGraphBuilder::graph	() const
{
	VERIFY	(m_graph);
	return	(*m_graph);
}

IC	CGameLevelCrossTable &CGameGraphBuilder::cross			() const
{
	VERIFY	(m_cross_table);
	return	(*m_cross_table);
}
