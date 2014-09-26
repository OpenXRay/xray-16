////////////////////////////////////////////////////////////////////////////
//	Module 		: FractionState.h
//	Created 	: 22.01.2008
//  Modified 	: 22.01.2008
//	Author		: Evgeniy Sokolov
//	Description : UI Fraction War state class
////////////////////////////////////////////////////////////////////////////

#pragma once
#include "UIWindow.h"

class FractionState
{
public:
				FractionState		();
				FractionState		( shared_str const& id );
				~FractionState		();

	IC int		get_actor_goodwill	()const;
	IC void		set_actor_goodwill	( int gw );

	IC LPCSTR	get_fraction_id		() const;
	IC void		set_fraction_id		( LPCSTR id );
	IC void		set_fraction_id2	( shared_str const& id );

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

		void	update_info			();

public:
	int				member_count;
	float			resource;
	float			power;
	int				state_vs;
	int				bonus;
	//shared_str		bonuses[6]; //int[6]

private:
	int				m_actor_goodwill;
	
	shared_str		m_id;
	shared_str		m_name;
	shared_str		m_icon;
	shared_str		m_icon_big;
	shared_str		m_target;
	shared_str		m_target_desc;
	shared_str		m_location;

	DECLARE_SCRIPT_REGISTER_FUNCTION
}; // struct FractionState

add_to_type_list(FractionState)
#undef script_type_list
#define script_type_list save_type_list(FractionState)

#include "FractionState_inline.h"
