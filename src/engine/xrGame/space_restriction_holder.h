////////////////////////////////////////////////////////////////////////////
//	Module 		: space_restriction_holder.h
//	Created 	: 17.08.2004
//  Modified 	: 27.08.2004
//	Author		: Dmitriy Iassenev
//	Description : Space restriction holder
////////////////////////////////////////////////////////////////////////////

#pragma once

class CSpaceRestrictionBridge;
class CSpaceRestrictor;

template <typename _1, typename _2> class intrusive_ptr;

namespace RestrictionSpace {
	struct CTimeIntrusiveBase;
	enum ERestrictorTypes;
};

namespace SpaceRestrictionHolder {
	typedef intrusive_ptr<CSpaceRestrictionBridge,RestrictionSpace::CTimeIntrusiveBase> CBaseRestrictionPtr;
};

class CSpaceRestrictionHolder {
public:
	typedef xr_map<shared_str,CSpaceRestrictionBridge*>	RESTRICTIONS;

private:
	enum {
		MAX_RESTRICTION_PER_TYPE_COUNT	= u32(128),
		dummy							= u32(-1),
	};

private:
	RESTRICTIONS					m_restrictions;
	shared_str						m_default_out_restrictions;
	shared_str						m_default_in_restrictions;

protected:
			shared_str				normalize_string				(shared_str space_restrictors);
	IC		void					collect_garbage					();
	virtual void					on_default_restrictions_changed	() = 0;
			void					clear							();

public:
	IC								CSpaceRestrictionHolder			();
	virtual							~CSpaceRestrictionHolder		();
			SpaceRestrictionHolder::CBaseRestrictionPtr	restriction	(shared_str space_restrictors);
			void					register_restrictor				(CSpaceRestrictor *space_restrictor, const RestrictionSpace::ERestrictorTypes &restrictor_type);
			void					unregister_restrictor			(CSpaceRestrictor *space_restrictor);
	IC		shared_str				default_out_restrictions		() const;
	IC		shared_str				default_in_restrictions			() const;
};

#include "space_restriction_holder_inline.h"