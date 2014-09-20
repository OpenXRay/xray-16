////////////////////////////////////////////////////////////////////////////
//	Module 		: rat_state_manager_inline.h
//	Created 	: 31.08.2007
//  Modified 	: 31.08.2007
//	Author		: Dmitriy Iassenev
//	Description : rat state manager class inline functions
////////////////////////////////////////////////////////////////////////////

#ifndef RAT_STATE_MANAGER_INLINE_H_INCLUDED
#define RAT_STATE_MANAGER_INLINE_H_INCLUDED

IC	void rat_state_manager::change_state	(state_id_type const &state_id)
{
	pop_state	();
	push_state	(state_id);
}

#endif // RAT_STATE_MANAGER_INLINE_H_INCLUDED