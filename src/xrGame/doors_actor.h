////////////////////////////////////////////////////////////////////////////
//	Created		: 24.06.2009
//	Author		: Dmitriy Iassenev
//	Copyright (C) GSC Game World - 2009
////////////////////////////////////////////////////////////////////////////

#ifndef DOORS_ACTOR_H_INCLUDED
#define DOORS_ACTOR_H_INCLUDED

#include <boost/noncopyable.hpp>
#include "doors.h"

class CAI_Stalker;

namespace doors {

class actor : private boost::noncopyable {
public:
						actor			( CAI_Stalker const& object );
	virtual				~actor			( );
	Fvector const&		get_position	( ) const;
	bool				need_update		( ) const;
	bool				update_doors	( doors_type const& doors, float average_speed );
	void				on_door_destroy	( door& door );
	pcstr				get_name		( ) const;
#ifdef DEBUG
	void				render			( ) const;
#endif // #ifdef DEBUG

private:
	typedef buffer_vector<door*>		temp_doors_type;

private:
	bool				add_new_door	(
							float average_speed,
							door* door,
							doors_type const& processed_doors,
							doors_type& locked_doors,
							temp_doors_type& new_doors,
							door_state state
						);
	void				process_doors	(
							float average_speed,
							doors_type& processed_doors,
							temp_doors_type const& new_doors,
							door_state start_state,
							door_state stop_state
						);
	void				revert_states	( doors_type& doors, door_state const state );

private:
	doors_type			m_open_doors;
	doors_type			m_closed_doors;
#ifdef DEBUG
	doors_type			m_detected_doors;
#endif // #ifdef DEBUG
	CAI_Stalker const&	m_object;
}; // class actor

} // namespace doors

#endif // #ifndef DOORS_ACTOR_H_INCLUDED