////////////////////////////////////////////////////////////////////////////
//	Module 		: space_restriction_composition.h
//	Created 	: 17.08.2004
//  Modified 	: 27.08.2004
//	Author		: Dmitriy Iassenev
//	Description : Space restriction composition
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "space_restriction_base.h"
#include "space_restriction_bridge.h"
#include "space_restriction_holder.h"

extern int g_restriction_checker;

class CSpaceRestrictionComposition : public CSpaceRestrictionBase {
public:
	using CSpaceRestrictionBase::inside;

protected:
	typedef SpaceRestrictionHolder::CBaseRestrictionPtr CBaseRestrictionPtr;
	typedef xr_vector<CBaseRestrictionPtr> RESTRICTIONS;

protected:
	RESTRICTIONS			m_restrictions;
	shared_str				m_space_restrictors;
	CSpaceRestrictionHolder	*m_space_restriction_holder;
	Fsphere					m_sphere;

#ifdef DEBUG
private:
				void		check_restrictor_type			();
#endif // DEBUG

protected:
	IC			void		merge							(CBaseRestrictionPtr restriction);

public:
	IC						CSpaceRestrictionComposition	(CSpaceRestrictionHolder *space_restriction_holder, shared_str space_restrictors);
		virtual				~CSpaceRestrictionComposition	();
		virtual void		initialize						();
		virtual bool		inside							(const Fsphere &sphere);
	IC	virtual shared_str	name							() const;
	IC	virtual bool		shape							() const;
	IC	virtual bool		default_restrictor				() const;
		virtual	Fsphere		sphere							() const;
#ifdef DEBUG
				void		test_correctness				();
#endif
};

#include "space_restriction_composition_inline.h"
