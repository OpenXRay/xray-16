////////////////////////////////////////////////////////////////////////////
//	Module 		: FactionState.h
//	Created 	: 22.01.2008
//	Author		: Evgeniy Sokolov
//	Description : UI Faction War state class
////////////////////////////////////////////////////////////////////////////

#pragma once
#include "xrUICore/Windows/UIWindow.h"

class FactionState
{
public:
				FactionState		();
				FactionState		( shared_str const& id );
				~FactionState		();

	IC int		get_actor_goodwill	()const;
	IC void		set_actor_goodwill	( int gw );

	IC LPCSTR	get_faction_id		() const;
	IC shared_str const& get_faction_id2() const;
	IC void		set_faction_id		( LPCSTR id );
	IC void		set_faction_id2		( shared_str const& id );

	IC LPCSTR	get_name			() const;
	IC void		set_name			( LPCSTR name );

	IC LPCSTR	get_icon			() const;
	IC void		set_icon			( LPCSTR icon );

	IC LPCSTR	get_icon_big		() const;
	IC void		set_icon_big		( LPCSTR icon_big );

	IC LPCSTR	get_target			() const;
	IC void		set_target			( LPCSTR target );

	IC LPCSTR	get_target_desc		() const;
	IC void		set_target_desc		( LPCSTR target_desc );

	IC LPCSTR	get_location		() const;
	IC void		set_location		( LPCSTR location );

	IC LPCSTR	get_war_state		( u8 index ) const;
	IC LPCSTR	get_war_state_hint	( u8 index ) const;

	IC LPCSTR	get_war_state1		() const;
	IC void		set_war_state1		( LPCSTR icon );
	IC LPCSTR	get_war_state2		() const;
	IC void		set_war_state2		( LPCSTR icon );
	IC LPCSTR	get_war_state3		() const;
	IC void		set_war_state3		( LPCSTR icon );
	IC LPCSTR	get_war_state4		() const;
	IC void		set_war_state4		( LPCSTR icon );
	IC LPCSTR	get_war_state5		() const;
	IC void		set_war_state5		( LPCSTR icon );

	IC LPCSTR	get_war_state_hint1		() const;
	IC void		set_war_state_hint1		( LPCSTR icon );
	IC LPCSTR	get_war_state_hint2		() const;
	IC void		set_war_state_hint2		( LPCSTR icon );
	IC LPCSTR	get_war_state_hint3		() const;
	IC void		set_war_state_hint3		( LPCSTR icon );
	IC LPCSTR	get_war_state_hint4		() const;
	IC void		set_war_state_hint4		( LPCSTR icon );
	IC LPCSTR	get_war_state_hint5		() const;
	IC void		set_war_state_hint5		( LPCSTR icon );

		void	update_info			();
		void	ResetStates			();

public:
	enum 		{ war_state_count = 5 };
	enum		{ bonuses_count = 6 };

	int				member_count;
	float			resource;
	float			power;

	int				bonus;
	//shared_str		bonuses[6];

private:
	int				m_actor_goodwill;
	
	shared_str		m_id;
	shared_str		m_name;
	shared_str		m_icon;
	shared_str		m_icon_big;
	shared_str		m_target;
	shared_str		m_target_desc;
	shared_str		m_location;

	//	int			state_vs;
	shared_str		m_war_state_str[war_state_count];
	shared_str		m_war_state_hint_str[war_state_count];

}; // struct FactionState

#include "FactionState_inline.h"
