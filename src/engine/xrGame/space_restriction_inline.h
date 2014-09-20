////////////////////////////////////////////////////////////////////////////
//	Module 		: space_restriction_inline.h
//	Created 	: 17.08.2004
//  Modified 	: 27.08.2004
//	Author		: Dmitriy Iassenev
//	Description : Space restriction inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CSpaceRestriction::CSpaceRestriction			(CSpaceRestrictionManager *space_restriction_manager, shared_str out_restrictions, shared_str in_restrictions)
{
	VERIFY							(space_restriction_manager);
	m_space_restriction_manager		= space_restriction_manager;
	m_out_restrictions				= out_restrictions;
	m_in_restrictions				= in_restrictions;
	m_initialized					= false;
	m_applied						= false;
}

IC	bool CSpaceRestriction::initialized				() const
{
	return							(m_initialized);
}

IC	shared_str CSpaceRestriction::out_restrictions		() const
{
	return							(m_out_restrictions);
}

IC	shared_str CSpaceRestriction::in_restrictions		() const
{
	return							(m_in_restrictions);
}

template <typename T1, typename T2>
IC	void CSpaceRestriction::add_border				(T1 p1, T2 p2)
{
	if (!initialized())
		return;

	VERIFY							(!m_applied);

	m_applied						= true;

	if (m_out_space_restriction) {
		ai().level_graph().set_mask	(border());
		return;
	}

#ifdef USE_FREE_IN_RESTRICTIONS
	FREE_IN_RESTRICTIONS::iterator	I = m_free_in_restrictions.begin();
	FREE_IN_RESTRICTIONS::iterator	E = m_free_in_restrictions.end();
	for ( ; I != E; ++I)
		if (affect((*I).m_restriction,p1,p2)) {
			VERIFY							(!(*I).m_enabled);
			(*I).m_enabled					= true;
			ai().level_graph().set_mask		((*I).m_restriction->border());
		}
#else
	ai().level_graph().set_mask		(m_in_space_restriction->border());
#endif
}

IC	bool CSpaceRestriction::applied					() const
{
	return							(m_applied);
}

IC	bool CSpaceRestriction::inside					(const Fsphere &sphere)
{
	return							(accessible(sphere));
}

IC	bool CSpaceRestriction::inside					(u32 level_vertex_id, bool partially_inside)
{
	return							(
		(
			m_out_space_restriction ? 
			m_out_space_restriction->inside(level_vertex_id,partially_inside) :
			true
		)
		&&
		(
			m_in_space_restriction ? 
			!m_in_space_restriction->inside(level_vertex_id,!partially_inside) :
			true
		)
	);
}
