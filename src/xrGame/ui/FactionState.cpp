////////////////////////////////////////////////////////////////////////////
//	Module 		: FactionState.cpp
//	Created 	: 22.01.2008
//	Author		: Evgeniy Sokolov
//	Description : UI Faction War state class implementation
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "StdAfx.h"
#include "FactionState.h"
#include "Actor.h"
#include "character_community.h"
#include "character_reputation.h"
#include "relation_registry.h"
#include "ai_space.h"
#include "xrScriptEngine/script_engine.hpp"

FactionState::FactionState():
	member_count(0),
	resource(0.0f),
	power(0.0f),
	m_actor_goodwill(0)
{
	m_id._set         (nullptr);
	m_name._set       (nullptr);
	m_icon._set       (nullptr);
	m_icon_big._set   (nullptr);
	m_target._set     (nullptr);
	m_target_desc._set(nullptr);
	m_location._set   (nullptr);

	ResetStates();
}

FactionState::FactionState(shared_str const& id) : FactionState()
{
    set_faction_id2(id);
}

FactionState::~FactionState()
{
}

void FactionState::ResetStates()
{
	for ( int i = 0; i < war_state_count ; ++i )
	{
		m_war_state_str[i]._set( NULL );
		m_war_state_hint_str[i]._set( NULL );
	}
	/*for ( int i = 0; i < bonuses_count ; ++i )
	{
	bonuses_vs[i]._set( NULL );
	}*/
}

SCRIPT_EXPORT(FactionState, (),
{
    using namespace luabind;

    module(luaState)
    [
		class_<FactionState>("FactionState")
            .def_readwrite("member_count",   &FactionState::member_count)
            .def_readwrite("resource",       &FactionState::resource)
            .def_readwrite("power",          &FactionState::power)
            .def_readwrite("bonus",          &FactionState::bonus)

            .property("faction_id",     &FactionState::get_faction_id,    &FactionState::set_faction_id)
            .property("actor_goodwill", &FactionState::get_actor_goodwill,&FactionState::set_actor_goodwill)

            .property("name",           &FactionState::get_name,          &FactionState::set_name)
            .property("icon",           &FactionState::get_icon,          &FactionState::set_icon)
            .property("icon_big",       &FactionState::get_icon_big,      &FactionState::set_icon_big)
            .property("target",         &FactionState::get_target,        &FactionState::set_target)
            .property("target_desc",    &FactionState::get_target_desc,   &FactionState::set_target_desc)
            .property("location",       &FactionState::get_location,      &FactionState::set_location)

            .property("war_state1",     &FactionState::get_war_state1,    &FactionState::set_war_state1)
            .property("war_state2",     &FactionState::get_war_state2,    &FactionState::set_war_state2)
            .property("war_state3",     &FactionState::get_war_state3,    &FactionState::set_war_state3)
            .property("war_state4",     &FactionState::get_war_state4,    &FactionState::set_war_state4)
            .property("war_state5",     &FactionState::get_war_state5,    &FactionState::set_war_state5)

            .property("war_state_hint1", &FactionState::get_war_state_hint1, &FactionState::set_war_state_hint1)
            .property("war_state_hint2", &FactionState::get_war_state_hint2, &FactionState::set_war_state_hint2)
            .property("war_state_hint3", &FactionState::get_war_state_hint3, &FactionState::set_war_state_hint3)
            .property("war_state_hint4", &FactionState::get_war_state_hint4, &FactionState::set_war_state_hint4)
            .property("war_state_hint5", &FactionState::get_war_state_hint5, &FactionState::set_war_state_hint5)
	];
})

void FactionState::update_info()
{
	if ( m_id.size() == 0 )
	{
		return;
	}
	m_actor_goodwill = 0;
	CActor* pActor = smart_cast<CActor*>( Level().CurrentEntity() );
	if ( pActor )
	{
		CHARACTER_COMMUNITY char_comm;
		char_comm.set( m_id );
		m_actor_goodwill = RELATION_REGISTRY().GetCommunityGoodwill( char_comm.index(), pActor->object_id() );
	}
	ResetStates();

    luabind::functor<void> m_functor;
    R_ASSERT(GEnv.ScriptEngine->functor( "pda.fill_faction_state", m_functor));
    m_functor(this);
}

