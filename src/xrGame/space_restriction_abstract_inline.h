////////////////////////////////////////////////////////////////////////////
//	Module 		: space_restriction_abstract_inline.h
//	Created 	: 10.08.2005
//  Modified 	: 10.08.2005
//	Author		: Dmitriy Iassenev
//	Description : Space restriction abstract inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CSpaceRestrictionAbstract::CSpaceRestrictionAbstract						()
{
	m_initialized									= false;
	m_accessible_neighbour_border_actual			= false;
}

IC	const xr_vector<u32> &CSpaceRestrictionAbstract::border						()
{
	if (!initialized())
		initialize									();

	THROW											(initialized());
	THROW3											(!m_border.empty(),"Space restrictor has no border!",*name());
	return											(m_border);
}

IC	bool CSpaceRestrictionAbstract::initialized									() const
{
	return											(m_initialized);
}

template <typename T>
IC	const xr_vector<u32> &CSpaceRestrictionAbstract::accessible_neighbour_border(T &restriction, bool out_restriction)
{
	if (!m_accessible_neighbour_border_actual)
		prepare_accessible_neighbour_border			(restriction,out_restriction);

	VERIFY2											(
		!m_accessible_neighbour_border.empty(),
		make_string(
			"space restrictor %s has no accessible neighbours (border size[%d])",
			*name(),
			border().size()
		)
	);
	return											(m_accessible_neighbour_border);
}

template <typename T>
IC	bool CSpaceRestrictionAbstract::accessible_neighbours						(T &restriction, u32 level_vertex_id, bool out_restriction)
{
	CLevelGraph::const_iterator						I, E;
	ai().level_graph().begin						(level_vertex_id,I,E);
	for ( ; I != E; ++I) {
		u32											current = ai().level_graph().value(level_vertex_id,I);
		if (!ai().level_graph().valid_vertex_id(current))
			continue;

		if (restriction->inside(current,!out_restriction) != out_restriction)
			continue;

		return										(true);
	}
	return											(false);
}

template <typename T>
IC	void CSpaceRestrictionAbstract::prepare_accessible_neighbour_border			(T &restriction, bool out_restriction)
{
	VERIFY											(!m_accessible_neighbour_border_actual);
	m_accessible_neighbour_border_actual			= true;

	VERIFY											(!border().empty());
	m_accessible_neighbour_border.reserve			(border().size());

	xr_vector<u32>::const_iterator					I = border().begin();
	xr_vector<u32>::const_iterator					E = border().end();
	for ( ; I != E; ++I)
		if (accessible_neighbours(restriction,*I,out_restriction))
			m_accessible_neighbour_border.push_back	(*I);
}
