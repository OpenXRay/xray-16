////////////////////////////////////////////////////////////////////////////
//	Module 		: group_hierarchy_holder.h
//	Created 	: 12.11.2001
//  Modified 	: 03.09.2004
//	Author		: Dmitriy Iassenev, Oles Shishkovtsov, Aleksandr Maksimchuk
//	Description : Group hierarchy holder
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "seniority_hierarchy_space.h"

class CEntity;
class CAgentManager;
class CSquadHierarchyHolder;

namespace MemorySpace {
	struct CVisibleObject;
	struct CSoundObject;
	struct CHitObject;
}

namespace GroupHierarchyHolder {
	typedef xr_vector<MemorySpace::CVisibleObject>	VISIBLE_OBJECTS;
	typedef xr_vector<MemorySpace::CSoundObject>	SOUND_OBJECTS;
	typedef xr_vector<MemorySpace::CHitObject>		HIT_OBJECTS;
	typedef xr_vector<CEntity*>						MEMBER_REGISTRY;
}

class CGroupHierarchyHolder {
private:
	typedef GroupHierarchyHolder::VISIBLE_OBJECTS	VISIBLE_OBJECTS;
	typedef GroupHierarchyHolder::SOUND_OBJECTS		SOUND_OBJECTS;
	typedef GroupHierarchyHolder::HIT_OBJECTS		HIT_OBJECTS;
	typedef GroupHierarchyHolder::MEMBER_REGISTRY	MEMBER_REGISTRY;

#ifdef SQUAD_HIERARCHY_HOLDER_USE_LEADER
private:
	CEntity							*m_leader;
#endif // SQUAD_HIERARCHY_HOLDER_USE_LEADER

private:
	CSquadHierarchyHolder			*m_squad;
	MEMBER_REGISTRY					m_members;
	VISIBLE_OBJECTS					*m_visible_objects;
	SOUND_OBJECTS					*m_sound_objects;
	HIT_OBJECTS						*m_hit_objects;

	// TODO: for stalker only, should be removed
private:
	CAgentManager					*m_agent_manager;

	// TODO: for rats only, should be removed
public:
	u32								m_dwLastActionTime;
	u32								m_dwLastAction;
	u32								m_dwActiveCount;
	u32								m_dwAliveCount;
	u32								m_dwStandingCount;

private:
	IC		CAgentManager			*get_agent_manager			() const;
private:
	IC		VISIBLE_OBJECTS			&visible_objects			() const;
	IC		SOUND_OBJECTS			&sound_objects				() const;
	IC		HIT_OBJECTS				&hit_objects				() const;
private:
			void					register_in_group			(CEntity *member);
			void					register_in_squad			(CEntity *member);
			void					register_in_agent_manager	(CEntity *member);
			void					register_in_group_senses	(CEntity *member);
private:
			void					unregister_in_group			(CEntity *member);
			void					unregister_in_squad			(CEntity *member);
			void					unregister_in_agent_manager	(CEntity *member);
			void					unregister_in_group_senses	(CEntity *member);

public:
	IC								CGroupHierarchyHolder		(CSquadHierarchyHolder *squad);
	virtual							~CGroupHierarchyHolder		();
	IC		CAgentManager			&agent_manager				() const;
	IC		const MEMBER_REGISTRY	&members					() const;
			void					register_member				(CEntity *member);
			void					unregister_member			(CEntity *member);
	IC		CSquadHierarchyHolder	&squad						() const;

#ifdef SQUAD_HIERARCHY_HOLDER_USE_LEADER
public:
			void					update_leader				();
	IC		CEntity					*leader						() const;
#endif // SQUAD_HIERARCHY_HOLDER_USE_LEADER
};

#include "group_hierarchy_holder_inline.h"