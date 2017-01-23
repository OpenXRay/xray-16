////////////////////////////////////////////////////////////////////////////
//	Module 		: xr_graph_merge.h
//	Created 	: 25.01.2003
//  Modified 	: 25.01.2003
//	Author		: Dmitriy Iassenev
//	Description : Merging level graphs for off-line AI NPC computations
////////////////////////////////////////////////////////////////////////////

#pragma once

struct CLevelInfo {
	u8				m_id;
	shared_str		m_name;
	Fvector			m_offset;
	shared_str		m_section;

	CLevelInfo		(u8 id, shared_str name, const Fvector &offset, shared_str section) :
		m_id		(id),
		m_name		(name),
		m_offset	(offset),
		m_section	(section)
	{
	}

	IC	bool	operator< (const CLevelInfo &info) const
	{
		return		(m_id < info.m_id);
	}
};

extern void xrMergeGraphs(
	LPCSTR game_graph_id,
	LPCSTR name,
	bool rebuild
);