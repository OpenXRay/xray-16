////////////////////////////////////////////////////////////////////////////
//	Module 		: agent_member_manager.h
//	Created 	: 24.05.2004
//  Modified 	: 14.01.2005
//	Author		: Dmitriy Iassenev
//	Description : Agent member manager
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "member_order.h"
#include "memory_space.h"

class CAgentManager;
class CEntity;

class CAgentMemberManager {
public:
	typedef xr_vector<CMemberOrder*>		MEMBER_STORAGE;
	typedef MEMBER_STORAGE::iterator		iterator;
	typedef MEMBER_STORAGE::const_iterator	const_iterator;
	typedef MemorySpace::squad_mask_type	squad_mask_type;

private:
	CAgentManager					*m_object;
	MEMBER_STORAGE					m_members;
	MEMBER_STORAGE					m_combat_members;
	bool							m_actuality;
	squad_mask_type					m_combat_mask;
	u32								m_last_throw_time;
	u32								m_throw_time_interval;

protected:
	IC		CAgentManager			&object					() const;

public:
	IC								CAgentMemberManager		(CAgentManager *object);
	virtual							~CAgentMemberManager	();
			void					update					();
			void					add						(CEntity *member);
			void					remove					(CEntity *member);
	IC		CMemberOrder			&member					(const CAI_Stalker *object);
			CMemberOrder			*get_member				(const ALife::_OBJECT_ID &id);
	IC		const MEMBER_STORAGE	&members				() const;
	IC		MEMBER_STORAGE			&members				();
	IC		squad_mask_type			mask					(const CAI_Stalker *object) const;
			squad_mask_type			mask					(const ALife::_OBJECT_ID &id) const;
	IC		bool					group_behaviour			() const;
	IC		iterator				member					(squad_mask_type mask);
			void					remove_links			(CObject *object);
			void					register_in_combat		(const CAI_Stalker *object);
			void					unregister_in_combat	(const CAI_Stalker *object);
			bool					registered_in_combat	(const CAI_Stalker *object) const;
	IC		const squad_mask_type	&combat_mask			() const;
			squad_mask_type			non_combat_members_mask	() const;
			MEMBER_STORAGE			&combat_members			();
			u32						in_detour				() const;
			bool					can_detour				() const;
			bool					cover_detouring			() const;
			bool					can_cry_noninfo_phrase	() const;
			bool					can_throw_grenade		(const Fvector &location) const;
			void					on_throw_completed		();
	IC		const u32				&throw_time_interval	() const;
	IC		void					throw_time_interval		(const u32 &value);
};

#include "agent_member_manager_inline.h"