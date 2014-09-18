////////////////////////////////////////////////////////////////////////////
//	Module 		: group_hierarchy_holder.cpp
//	Created 	: 12.11.2001
//  Modified 	: 03.09.2004
//	Author		: Dmitriy Iassenev, Oles Shishkovtsov, Aleksandr Maksimchuk
//	Description : Group hierarchy holder
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "group_hierarchy_holder.h"
#include "squad_hierarchy_holder.h"
#include "entity.h"
#include "agent_manager.h"
#include "agent_member_manager.h"
#include "agent_memory_manager.h"
#include "ai/stalker/ai_stalker.h"
#include "memory_manager.h"
#include "visual_memory_manager.h"
#include "sound_memory_manager.h"
#include "hit_memory_manager.h"

CGroupHierarchyHolder::~CGroupHierarchyHolder			()
{
	VERIFY						(m_members.empty());
	VERIFY						(!m_visible_objects);
	VERIFY						(!m_sound_objects);
	VERIFY						(!m_hit_objects);
	VERIFY						(!m_agent_manager);
}

#ifdef SQUAD_HIERARCHY_HOLDER_USE_LEADER
void CGroupHierarchyHolder::update_leader				()
{
	m_leader					= 0;
	MEMBER_REGISTRY::iterator	I = m_members.begin();
	MEMBER_REGISTRY::iterator	E = m_members.end();
	for ( ; I != E; ++I)
		if ((*I)->g_Alive()) {
			m_leader			= *I;
			break;
		}
}
#endif // SQUAD_HIERARCHY_HOLDER_USE_LEADER

void CGroupHierarchyHolder::register_in_group			(CEntity *member)
{
	VERIFY						(member);
	MEMBER_REGISTRY::iterator	I = std::find(m_members.begin(),m_members.end(),member);
	VERIFY3						(I == m_members.end(),"Specified group member has already been found",*member->cName());

	if (m_members.empty()) {
		m_visible_objects		= xr_new<VISIBLE_OBJECTS>();
		m_sound_objects			= xr_new<SOUND_OBJECTS>();
		m_hit_objects			= xr_new<HIT_OBJECTS>();

//		m_visible_objects->reserve	(128);
//		m_sound_objects->reserve	(128);
//		m_hit_objects->reserve		(128);
	}

	m_members.push_back			(member);
}

void CGroupHierarchyHolder::register_in_squad			(CEntity *member)
{
#ifdef SQUAD_HIERARCHY_HOLDER_USE_LEADER
	if (!leader() && member->g_Alive()) {
		m_leader				= member;
		if (!squad().leader())
			squad().leader		(member);
	}
#endif // SQUAD_HIERARCHY_HOLDER_USE_LEADER
}

void CGroupHierarchyHolder::register_in_agent_manager	(CEntity *member)
{
	if (!get_agent_manager() && smart_cast<CAI_Stalker*>(member)) {
		m_agent_manager								= xr_new<CAgentManager>();
		agent_manager().memory().set_squad_objects	(&visible_objects());
		agent_manager().memory().set_squad_objects	(&sound_objects());
		agent_manager().memory().set_squad_objects	(&hit_objects());
	}

	if (get_agent_manager())
		agent_manager().member().add				(member);
}

void CGroupHierarchyHolder::register_in_group_senses	(CEntity *member)
{
	CCustomMonster			*monster = smart_cast<CCustomMonster*>(member);
	if (monster) {
		monster->memory().visual().set_squad_objects(&visible_objects());
		monster->memory().sound().set_squad_objects	(&sound_objects());
		monster->memory().hit().set_squad_objects	(&hit_objects());
	}
}

void CGroupHierarchyHolder::unregister_in_group			(CEntity *member)
{
	VERIFY						(member);
	MEMBER_REGISTRY::iterator	I = std::find(m_members.begin(),m_members.end(),member);
	VERIFY3						(I != m_members.end(),"Specified group member cannot be found",*member->cName());
	m_members.erase				(I);
}

void CGroupHierarchyHolder::unregister_in_squad			(CEntity *member)
{
#ifdef SQUAD_HIERARCHY_HOLDER_USE_LEADER
	if (leader() && (leader()->ID() == member->ID())) {
		update_leader					();
		if (squad().leader()->ID() == member->ID())
			if (leader())
				squad().leader			(leader());
			else
				squad().update_leader	();
	}
#endif // SQUAD_HIERARCHY_HOLDER_USE_LEADER
}

void CGroupHierarchyHolder::unregister_in_agent_manager	(CEntity *member)
{
	if (get_agent_manager()) {
		agent_manager().member().remove	(member);
		if (agent_manager().member().members().empty())
			xr_delete					(m_agent_manager);
	}

	if (m_members.empty()) {
		xr_delete						(m_visible_objects);
		xr_delete						(m_sound_objects);
		xr_delete						(m_hit_objects);
	}
}

void CGroupHierarchyHolder::unregister_in_group_senses	(CEntity *member)
{
	CCustomMonster			*monster = smart_cast<CCustomMonster*>(member);
	if (monster) {
		monster->memory().visual().set_squad_objects(0);
		monster->memory().sound().set_squad_objects	(0);
		monster->memory().hit().set_squad_objects	(0);
	}
}

void CGroupHierarchyHolder::register_member				(CEntity *member)
{
	register_in_group			(member);
	register_in_squad			(member);
	register_in_agent_manager	(member);
	register_in_group_senses	(member);
}

void CGroupHierarchyHolder::unregister_member			(CEntity *member)
{
	unregister_in_group			(member);
	unregister_in_squad			(member);
	unregister_in_agent_manager	(member);
	unregister_in_group_senses	(member);
}
