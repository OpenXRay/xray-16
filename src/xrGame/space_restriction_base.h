////////////////////////////////////////////////////////////////////////////
//	Module 		: space_restriction_base.h
//	Created 	: 17.08.2004
//  Modified 	: 27.08.2004
//	Author		: Dmitriy Iassenev
//	Description : Space restriction base
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "space_restriction_abstract.h"

class CSpaceRestrictionBase : public CSpaceRestrictionAbstract {
private:
	typedef CSpaceRestrictionAbstract		inherited;

public:
#ifdef DEBUG
	xr_vector<u32>		m_test_storage;
	bool				m_correct;
#endif

protected:
			void		process_borders		();

public:
			bool		inside				(u32 level_vertex_id, bool partially_inside);
			bool		inside				(u32 level_vertex_id, bool partially_inside, float radius);
	virtual	bool		inside				(const Fsphere &sphere) = 0;
	virtual bool		shape				() const = 0;
	virtual bool		default_restrictor	() const = 0;
	virtual	Fsphere		sphere				() const = 0;

public:
#ifdef DEBUG
	IC		bool		correct				() const;
#endif
};

#include "space_restriction_base_inline.h"