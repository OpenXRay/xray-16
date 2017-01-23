////////////////////////////////////////////////////////////////////////////
//	Module 		: space_restrictor_wrapper_inline.h
//	Created 	: 28.11.2005
//  Modified 	: 28.11.2005
//	Author		: Dmitriy Iassenev
//	Description : space restrictor wrapper inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CSpaceRestrictorWrapper::object_type &CSpaceRestrictorWrapper::object	() const
{
	VERIFY	(m_object);
	return	(*m_object);
}

IC	CLevelGraph &CSpaceRestrictorWrapper::level_graph						() const
{
	VERIFY	(m_level_graph);
	return	(*m_level_graph);
}

IC	CGraphEngine &CSpaceRestrictorWrapper::graph_engine						() const
{
	VERIFY	(m_graph_engine);
	return	(*m_graph_engine);
}
