////////////////////////////////////////////////////////////////////////////
//	Module 		: space_restriction.h
//	Created 	: 17.08.2004
//  Modified 	: 27.08.2004
//	Author		: Dmitriy Iassenev
//	Description : Space restriction
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "restriction_space.h"
#include "space_restriction_holder.h"
#include "space_restriction_bridge.h"
#include "space_restriction_abstract.h"

//#define USE_FREE_IN_RESTRICTIONS

class CSpaceRestrictionManager;

class CSpaceRestriction : 
	public RestrictionSpace::CTimeIntrusiveBase,
	public CSpaceRestrictionAbstract
{
private:
	typedef CSpaceRestrictionAbstract	inherited;

	friend struct CRemoveMergedFreeInRestrictions;
#ifdef DEBUG
	friend class CLevelGraph;
#endif
private:
	typedef SpaceRestrictionHolder::CBaseRestrictionPtr CBaseRestrictionPtr;

private:
	struct CFreeInRestriction {
		CBaseRestrictionPtr		m_restriction;
		bool					m_enabled;

		IC	CFreeInRestriction	(CBaseRestrictionPtr restriction, bool enabled)
		{
			m_restriction		= restriction;
			m_enabled			= enabled;
		}
	};

private:
	typedef xr_vector<CBaseRestrictionPtr>	RESTRICTIONS;
	typedef xr_vector<CFreeInRestriction>	FREE_IN_RESTRICTIONS;

protected:
	bool							m_applied;
	shared_str						m_out_restrictions;
	shared_str						m_in_restrictions;
	xr_vector<u32>					m_temp;
	CSpaceRestrictionManager		*m_space_restriction_manager;
	CBaseRestrictionPtr				m_out_space_restriction;
	CBaseRestrictionPtr				m_in_space_restriction;
#ifdef USE_FREE_IN_RESTRICTIONS
	FREE_IN_RESTRICTIONS			m_free_in_restrictions;
#endif

private:
	IC		bool					intersects					(CBaseRestrictionPtr bridge);
	IC		bool					intersects					(SpaceRestrictionHolder::CBaseRestrictionPtr bridge0, SpaceRestrictionHolder::CBaseRestrictionPtr bridge1);
			CBaseRestrictionPtr		merge						(CBaseRestrictionPtr bridge, const RESTRICTIONS &temp_restrictions) const;
			void					merge_in_out_restrictions	();
			void					merge_free_in_retrictions	();

protected:
	IC		bool					initialized					() const;
			bool					affect						(CBaseRestrictionPtr bridge, const Fsphere &sphere) const;
			bool					affect						(CBaseRestrictionPtr bridge, u32 start_vertex_id, float radius) const;
			bool					affect						(CBaseRestrictionPtr bridge, const Fvector &start_position, const Fvector &dest_position) const;
			bool					affect						(CBaseRestrictionPtr bridge, u32 start_vertex_id, u32 dest_vertex_id) const;

public:
									CSpaceRestriction			(CSpaceRestrictionManager *space_restriction_manager, shared_str out_restrictions, shared_str in_restrictions);
			void					initialize					();
			void					remove_border				();
	template <typename T1, typename T2>
	IC		void					add_border					(T1 p1, T2 p2);
			u32						accessible_nearest			(const Fvector &position, Fvector &result);
			bool					accessible					(const Fsphere &sphere);
			bool					accessible					(u32 level_vertex_id, float radius);
	IC		shared_str				out_restrictions			() const;
	IC		shared_str				in_restrictions				() const;
	IC		bool					applied						() const;
	IC		bool					inside						(const Fsphere &sphere);
	IC		bool					inside						(u32 level_vertex_id, bool partially_inside);
	virtual shared_str				name						() const;
};

#include "space_restriction_inline.h"