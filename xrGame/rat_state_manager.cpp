////////////////////////////////////////////////////////////////////////////
//	Module 		: rat_state_manager.cpp
//	Created 	: 31.08.2007
//  Modified 	: 31.08.2007
//	Author		: Dmitriy Iassenev
//	Description : rat state manager class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "rat_state_manager.h"
#include "object_broker.h"
#include "rat_state_base.h"

rat_state_manager::rat_state_manager	() :
	m_object				(0),
	m_last_state_id			(u32(-1))
{
}

rat_state_manager::~rat_state_manager	()
{
	delete_data				(m_states);
}

void rat_state_manager::construct		(CAI_Rat *object)
{
	VERIFY					(object);
	m_object				= object;
}

rat_state_base *rat_state_manager::state(state_id_type const &state_id)
{
	States::iterator		I = m_states.find(state_id);
	if (I == m_states.end())
		return				(0);

	return					((*I).second);
}

void rat_state_manager::push_state		(state_id_type const &state_id)
{
	VERIFY					(state(state_id));
	m_stack.push			(state_id);
}

void rat_state_manager::pop_state		()
{
	VERIFY					(!m_stack.empty());
	m_stack.pop				();
}

void rat_state_manager::add_state		(state_id_type const &state_id, rat_state_base *state)
{
	VERIFY					(!this->state(state_id));
	state->construct		(m_object);
	m_states.insert			(std::make_pair(state_id, state));
}

void rat_state_manager::update			()
{
	VERIFY					(!m_stack.empty());
	state_id_type			new_state_id = m_stack.top();
	if (new_state_id == m_last_state_id) {
		rat_state_base		*current = state(m_last_state_id);
		VERIFY				(current);
		current->execute	();
		return;
	}

	rat_state_base			*old_state = state(m_last_state_id);
	if (old_state)
		old_state->finalize	();

	m_last_state_id			= new_state_id;
	rat_state_base			*new_state = state(new_state_id);
	VERIFY					(new_state);
	new_state->initialize	();
	new_state->execute		();
}