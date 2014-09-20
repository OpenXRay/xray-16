////////////////////////////////////////////////////////////////////////////
//	Module 		: member_order.h
//	Created 	: 26.05.2004
//  Modified 	: 26.05.2004
//	Author		: Dmitriy Iassenev
//	Description : Member order
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "agent_manager_space.h"
#include "stalker_decision_space.h"
#include "graph_engine_space.h"
#include "condition_state.h"

class CAI_Stalker;
class CCoverPoint;
class CExplosive;
class CGameObject;

class CMemberOrder {
public:
	struct CMemberDeathReaction {
		CAI_Stalker				*m_member;
		u32						m_time;
		bool					m_processing;

		IC			CMemberDeathReaction()
		{
			clear				();
		}

		IC	void	clear				()
		{
			m_member			= 0;
			m_time				= 0;
			m_processing		= false;
		}
	};

	struct CGrenadeReaction {
		const CExplosive		*m_grenade;
		const CGameObject		*m_game_object;
		u32						m_time;
		bool					m_processing;

		IC			CGrenadeReaction	()
		{
			clear				();
		}

		IC	void	clear				()
		{
			m_grenade			= 0;
			m_game_object		= 0;
			m_time				= 0;
			m_processing		= false;
		}
	};

protected:
	CAI_Stalker					*m_object;
	mutable const CCoverPoint	*m_cover;
	bool						m_initialized;
	float						m_probability;
	xr_vector<u32>				m_enemies;
	bool						m_processed;
	u32							m_selected_enemy;
	CMemberDeathReaction		m_member_death_reaction;
	CGrenadeReaction			m_grenade_reaction;
	bool						m_detour;

public:
	IC							CMemberOrder			(CAI_Stalker *object);
	IC		bool				initialized				() const;
	IC		CAI_Stalker			&object					() const;
	IC		float				probability				() const;
	IC		bool				processed				() const;
	IC		u32					selected_enemy			() const;
	IC		const CCoverPoint	*cover					() const;
	IC		CMemberDeathReaction&member_death_reaction	();
	IC		CGrenadeReaction	&grenade_reaction		();
	IC		xr_vector<u32>		&enemies				();
	IC		void				cover					(const CCoverPoint *object_cover) const;
	IC		void				probability				(float probability);
	IC		void				processed				(bool processed);
	IC		void				selected_enemy			(u32 selected_enemy);
	IC		bool				detour					() const;
	IC		void				detour					(const bool &value);
};

#include "member_order_inline.h"