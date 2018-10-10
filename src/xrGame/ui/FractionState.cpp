////////////////////////////////////////////////////////////////////////////
//	Module 		: FractionState.cpp
//	Created 	: 22.01.2008
//  Modified 	: 23.01.2008
//	Author		: Evgeniy Sokolov
//	Description : UI Fraction War state class implementation
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "StdAfx.h"
#include "FractionState.h"

#include "../Actor.h"
#include "../character_community.h"
#include "../character_reputation.h"
#include "../relation_registry.h"
#include "../ai_space.h"
#include "xrScriptEngine/script_engine.hpp"

using namespace luabind;

FractionState::FractionState():
	member_count(0),
	resource(0.0f),
	power(0.0f),
	state_vs(0),
	m_actor_goodwill(0)
{
	m_id._set         ( NULL );
	m_name._set       ( NULL );
	m_icon._set       ( NULL );
	m_icon_big._set   ( NULL );
	m_target._set     ( NULL );
	m_target_desc._set( NULL );
	m_location._set   ( NULL );
}

FractionState::FractionState( shared_str const& id )
{
	FractionState();
	set_fraction_id2( id );
}

FractionState::~FractionState()
{
}

SCRIPT_EXPORT(FractionState, (),
{
	module(luaState)
    [
		class_<FractionState>("FractionState")
            .def_readwrite("member_count",   &FractionState::member_count)
            .def_readwrite("resource",       &FractionState::resource)
            .def_readwrite("power",          &FractionState::power)
            .def_readwrite("state_vs",       &FractionState::state_vs)
            .def_readwrite("bonus",          &FractionState::bonus)

            .property("fraction_id",    &FractionState::get_fraction_id,   &FractionState::set_fraction_id)
            .property("actor_goodwill", &FractionState::get_actor_goodwill,&FractionState::set_actor_goodwill)

            .property("name",           &FractionState::get_name,          &FractionState::set_name)
            .property("icon",           &FractionState::get_icon,          &FractionState::set_icon)
            .property("icon_big",       &FractionState::get_icon_big,      &FractionState::set_icon_big)
            .property("target",         &FractionState::get_target,        &FractionState::set_target)
            .property("target_desc",    &FractionState::get_target_desc,   &FractionState::set_target_desc)
            .property("location",       &FractionState::get_location,      &FractionState::set_location)
    ];
})

void FractionState::update_info()
{
	if ( m_id.size() == 0 )
	{
		return;
	}
	m_actor_goodwill = 0;
	CActor* pActor = smart_cast<CActor*>( Level().CurrentEntity() );
	if ( pActor )
	{
		CHARACTER_COMMUNITY char_cmm;
		char_cmm.set( m_id );
		m_actor_goodwill = RELATION_REGISTRY().GetCommunityGoodwill( char_cmm.index(), pActor->object_id() );
	}

	luabind::functor<void> m_functor;
	R_ASSERT(GEnv.ScriptEngine->functor("pda.fill_fraction_state", m_functor));
	m_functor(this);
}
