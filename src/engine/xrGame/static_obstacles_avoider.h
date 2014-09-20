////////////////////////////////////////////////////////////////////////////
//	Module 		: static_obstacles_avoider.h
//	Created 	: 16.05.2007
//  Modified 	: 16.05.2007
//	Author		: Dmitriy Iassenev
//	Description : static obstacles avoider
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "obstacles_query.h"
#include "refreshable_obstacles_query.h"

class CAI_Stalker;
class stalker_movement_manager_obstacles;

class static_obstacles_avoider {
private:
	stalker_movement_manager_obstacles		*m_movement_manager;
	const bool								*m_failed_to_build_path;
	bool									m_need_path_to_rebuild;

protected:
	obstacles_query							m_current_iteration;
	refreshable_obstacles_query				m_inactive_query;
	refreshable_obstacles_query				m_active_query;

private:
	obstacles_query							m_last_iteration;
	obstacles_query							m_temp_query;

private:
			bool							new_obstacles_found		() const;
			bool							refresh_objects			();

protected:
	virtual	void							query					();
	IC		const stalker_movement_manager_obstacles	&movement_manager		() const;
	IC		const CAI_Stalker				&object					() const;

public:
	IC										static_obstacles_avoider();
	IC		void							construct				(stalker_movement_manager_obstacles *movement_manager, const bool &failed_to_build_path);
	IC		const bool						&need_path_to_rebuild	() const;
	IC		obstacles_query					&active_query			();
	IC		obstacles_query					&inactive_query			();
	IC		obstacles_query					&current_iteration		();
	IC		void							clear					();

public:
			void							on_before_query			();
			void							query					(const Fvector &start_position, const Fvector &dest_position);
	virtual	bool							process_query			(const bool &change_path_state);
			void							update					();
			void							remove_links			(CObject *object);
};

#include "static_obstacles_avoider_inline.h"