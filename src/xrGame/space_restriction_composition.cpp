////////////////////////////////////////////////////////////////////////////
//	Module 		: space_restriction_composition.cpp
//	Created 	: 17.08.2004
//  Modified 	: 27.08.2004
//	Author		: Dmitriy Iassenev
//	Description : Space restriction composition
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "space_restriction_composition.h"
#include "space_restriction_holder.h"
#include "space_restriction_bridge.h"
#include "restriction_space.h"
#include "ai_space.h"
#include "level_graph.h"
#include "graph_engine.h"

#pragma warning(push)
#pragma warning(disable:4995)
#include <malloc.h>
#pragma warning(pop)

#ifdef DEBUG
#	include "level.h"
#	include "space_restrictor.h"
#endif // DEBUG

int g_restriction_checker = 0;

CSpaceRestrictionComposition::~CSpaceRestrictionComposition	()
{
	--g_restriction_checker;
}

struct CMergePredicate {
	CSpaceRestrictionComposition *m_restriction;

	IC	CMergePredicate	(CSpaceRestrictionComposition *restriction)
	{
		m_restriction	= restriction;
	}

	IC	bool operator()	(u32 level_vertex_id) const
	{
		return			(m_restriction->inside(level_vertex_id,false));
	}
};

IC	void CSpaceRestrictionComposition::merge	(CBaseRestrictionPtr restriction)
{
	m_restrictions.push_back	(restriction);
	m_border.insert				(m_border.begin(),restriction->border().begin(),restriction->border().end());
}

bool CSpaceRestrictionComposition::inside					(const Fsphere &sphere)
{
	if (!initialized()) {
		initialize				();
		if (!initialized())
			return				(true);
	}

	if (!m_sphere.intersect(sphere))
		return					(false);

	RESTRICTIONS::iterator		I = m_restrictions.begin();
	RESTRICTIONS::iterator		E = m_restrictions.end();
	for ( ; I != E; ++I)
		if ((*I)->inside(sphere))
			return				(true);

	return						(false);
}

void CSpaceRestrictionComposition::initialize	()
{
	u32							n = _GetItemCount(*m_space_restrictors);
	VERIFY						(n);
	if (n == 1) {
#ifdef DEBUG		
		m_correct				= true;
		check_restrictor_type	();
#endif
		return;
	}

	string256					element;

	for (u32 i=0; i<n ;++i)
		if (!m_space_restriction_holder->restriction(_GetItem(*m_space_restrictors,i,element))->initialized())
			return;

	Fsphere						*spheres = (Fsphere*)_alloca(n*sizeof(Fsphere));
	for (u32 i=0; i<n ;++i) {
		SpaceRestrictionHolder::CBaseRestrictionPtr	restriction = 
			m_space_restriction_holder->restriction(
				_GetItem(
					*m_space_restrictors,
					i,
					element
				)
			);

		merge					(restriction);

		spheres[i]				= restriction->sphere();
	}

	// computing almost minimum sphere which covers all the almost minimum spheres
	Fbox3						temp;
	temp.min.x					= spheres[0].P.x - spheres[0].R;
	temp.min.y					= spheres[0].P.y - spheres[0].R;
	temp.min.z					= spheres[0].P.z - spheres[0].R;
	temp.max.x					= spheres[0].P.x + spheres[0].R;
	temp.max.y					= spheres[0].P.y + spheres[0].R;
	temp.max.z					= spheres[0].P.z + spheres[0].R;

	for (u32 i=1; i<n; ++i) {
		temp.min.x				= _min(temp.min.x,spheres[i].P.x - spheres[i].R);
		temp.min.y				= _min(temp.min.y,spheres[i].P.y - spheres[i].R);
		temp.min.z				= _min(temp.min.z,spheres[i].P.z - spheres[i].R);
		temp.max.x				= _max(temp.max.x,spheres[i].P.x + spheres[i].R);
		temp.max.y				= _max(temp.max.y,spheres[i].P.y + spheres[i].R);
		temp.max.z				= _max(temp.max.z,spheres[i].P.z + spheres[i].R);
	}

	m_sphere.P.mad				(temp.min,temp.max,.5f);
	m_sphere.R					= m_sphere.P.distance_to(spheres[0].P) + spheres[0].R;

	for (u32 i=1; i<n ;++i)
		m_sphere.R				= _max(m_sphere.R,m_sphere.P.distance_to(spheres[i].P) + spheres[i].R);

	m_sphere.R					+= EPS_L;

	m_initialized				= true;

	m_border.erase				(
		std::remove_if(
			m_border.begin(),
			m_border.end(),
			CMergePredicate(this)
		),
		m_border.end()
	);

	process_borders				();

#ifdef DEBUG
	test_correctness			();
#endif
}

#ifdef DEBUG
void CSpaceRestrictionComposition::test_correctness()
{
	m_correct					= true;
	m_test_storage.clear		();

	{
		RESTRICTIONS::iterator		I = m_restrictions.begin();
		RESTRICTIONS::iterator		E = m_restrictions.end();
		for ( ; I != E; ++I)
			m_test_storage.insert	(m_test_storage.end(),(*I)->object().m_test_storage.begin(),(*I)->object().m_test_storage.end());
	}

	{
		std::sort					(m_test_storage.begin(),m_test_storage.end());
		m_test_storage.erase		(
			std::unique(	
				m_test_storage.begin(),
				m_test_storage.end()
			),
			m_test_storage.end()
		);
	}

	if (m_test_storage.empty()) {
		m_correct					= false;
		return;
	}

	xr_vector<u32>					nodes;
	{
		RESTRICTIONS::iterator		I = m_restrictions.begin();
		RESTRICTIONS::iterator		E = m_restrictions.end();
		for ( ; I != E; ++I) {
			VERIFY3							(!(*I)->object().m_test_storage.empty(),"Restrictor has no border",*(*I)->object().name());
			nodes.clear						();
			ai().level_graph().set_mask		(border());
			ai().graph_engine().search		(ai().level_graph(), (*I)->object().m_test_storage.back(), (*I)->object().m_test_storage.back(), &nodes, GraphEngineSpace::CFlooder());
			ai().level_graph().clear_mask	(border());
			
			if (nodes.size() == 65535)		
				m_correct = true;
			else
				m_correct						= (m_test_storage.size() <= nodes.size());
			
			if (!m_correct)
				break;
		}
	}
}
#endif

Fsphere CSpaceRestrictionComposition::sphere	() const
{
	NODEFAULT;
#ifdef DEBUG
	return							(m_sphere);
#endif
}

#ifdef DEBUG
void CSpaceRestrictionComposition::check_restrictor_type()
{
	if (_GetItemCount(*m_space_restrictors) == 1)
		return;

	if (!ai().get_alife())
		return;

	CObject							*object = Level().Objects.FindObjectByName(m_space_restrictors);
	if (!object)
		return;

	CSpaceRestrictor				*restrictor = smart_cast<CSpaceRestrictor*>(object);
	VERIFY3							(restrictor,"you are trying to use object as a restrictor",*m_space_restrictors);
	VERIFY2							(restrictor->restrictor_type() == RestrictionSpace::eRestrictorTypeNone,"you are trying to restrict yourself with restrictor with type eRestrictorTypeNone");
	VERIFY2							(restrictor->restrictor_type() != RestrictionSpace::eRestrictorTypeNone,"impossible situation: wrong net_Spawn branch used");
}
#endif // DEBUG