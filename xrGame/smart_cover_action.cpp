////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover_action.cpp
//	Created 	: 16.08.2007
//	Author		: Alexander Dudin
//	Description : Action class for smart cover
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "smart_cover_action.h"
#include "ai_monster_space.h"
#include "ai_space.h"
#include "level_graph.h"
#include "../xrServerEntities/object_broker.h"

using smart_cover::detail::parse_string;
using smart_cover::detail::parse_int;
using smart_cover::detail::parse_table;
using smart_cover::detail::parse_bool;
using smart_cover::detail::parse_fvector;

smart_cover::action::action(luabind::object const &description)
{
	luabind::object movement = description["movement"];
	if (movement.type() != LUA_TNIL && movement.type() == LUA_TBOOLEAN) {
		m_movement				= luabind::object_cast<bool>(movement);

		luabind::object position = description["position"];
		if (position.type() != LUA_TNIL)
			m_target_position	= luabind::object_cast<Fvector>(position);
	}
	else
		m_movement				= false;

	luabind::object animations;
	parse_table					(description, "animations", animations);
	typedef luabind::object::iterator	iterator;
	iterator		I = animations.begin();
	iterator		E = animations.end();
	for ( ; I != E; ++I) {
		VERIFY		(I.key().type() == LUA_TSTRING);
		LPCSTR		animation_type = luabind::object_cast<LPCSTR>(I.key());
		luabind::object	table = *I;
		if (table.type() != LUA_TTABLE) {
			VERIFY	(table.type() != LUA_TNIL);
			continue;
		}
		add_animation	(animation_type, *I);
	}
}

smart_cover::action::~action()
{
	delete_data(m_animations);
}

void smart_cover::action::add_animation(LPCSTR type, luabind::object const &table)
{	
	VERIFY						( table.type() == LUA_TTABLE );
	luabind::object::iterator I	= table.begin();
	luabind::object::iterator E	= table.end();
	Animations* animations		= xr_new<Animations>( );
	for ( ; I != E; ++I) {
		luabind::object	string	= *I;
		if (string.type() != LUA_TSTRING) {
			VERIFY				( string.type() != LUA_TNIL );
			continue;
		}

		shared_str animation	= luabind::object_cast<LPCSTR>(string);
		VERIFY2					(
			std::find(
				animations->begin(),
				animations->end(),
				animation
			) == 
			animations->end(),
			make_string(
				"duplicated_animation found: %s",
				animation.c_str()
			)
		);
		animations->push_back	( animation );
	}

	m_animations.insert			( std::make_pair( type, animations ) );
}