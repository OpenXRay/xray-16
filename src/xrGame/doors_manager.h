////////////////////////////////////////////////////////////////////////////
//	Created		: 23.06.2009
//	Author		: Dmitriy Iassenev
//	Copyright (C) GSC Game World - 2009
////////////////////////////////////////////////////////////////////////////

#ifndef DOORS_MANAGER_H_INCLUDED
#define DOORS_MANAGER_H_INCLUDED

#include <boost/noncopyable.hpp>
#include "quadtree.h"
#include "doors.h"

class CPhysicObject;

namespace doors {

class actor;
class door;

class manager : private boost::noncopyable {
public:
				manager					( Fbox const& bounding_box );
				~manager				( );
	door*		register_door			( CPhysicObject& object );
	void		unregister_door			( door*& door );
	void		on_door_is_open			( door* door );
	void		on_door_is_closed		( door* door );
	bool		is_door_locked			( door const* door ) const;
	void		lock_door				( door* door );
	void		unlock_door				( door* door );
	bool		is_door_blocked			( door* door ) const;

public:
	bool		actualize_doors_state	( actor& npc, float average_speed );

private:
	friend class doors::actor;
	void		open_door				( door* door );
	void		close_door				( door* door );
//	void		check_bug_door			( ) const;

private:
	typedef CQuadTree<door>				doors_tree_type;

private:
	doors_tree_type	m_doors;
	doors_type		m_nearest_doors;
}; // class manager

} // namespace doors

#endif // #ifndef DOORS_MANAGER_H_INCLUDED