////////////////////////////////////////////////////////////////////////////
//	Module 		: rat_state_manager.h
//	Created 	: 31.08.2007
//  Modified 	: 31.08.2007
//	Author		: Dmitriy Iassenev
//	Description : rat state manager class
////////////////////////////////////////////////////////////////////////////

#ifndef RAT_STATE_MANAGER_H_INCLUDED
#define RAT_STATE_MANAGER_H_INCLUDED

#include <boost/noncopyable.hpp>
#include "associative_vector.h"

class rat_state_base;
class CAI_Rat;

class rat_state_manager : private boost::noncopyable {
private:
	typedef	u32													state_id_type;
	typedef associative_vector<state_id_type, rat_state_base*>	States;
	typedef xr_stack<state_id_type>								Stack;

private:
	CAI_Rat			*m_object;
	States			m_states;
	Stack			m_stack;
	state_id_type	m_last_state_id;

private:
			rat_state_base *state				(state_id_type const &state_id);

public:
							rat_state_manager	();
							~rat_state_manager	();
			void			construct			(CAI_Rat *object);
	IC		void			change_state		(state_id_type const &state_id);
			void			push_state			(state_id_type const &state_id);
			void			pop_state			();
			void			add_state			(state_id_type const &state_id, rat_state_base *state);
			void			update				();
};

#include "rat_state_manager_inline.h"

#endif // RAT_STATE_MANAGER_H_INCLUDED