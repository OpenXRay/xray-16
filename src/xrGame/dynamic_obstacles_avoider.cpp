////////////////////////////////////////////////////////////////////////////
//	Module 		: dynamic_obstacles_avoider.cpp
//	Created 	: 16.05.2007
//  Modified 	: 16.05.2007
//	Author		: Dmitriy Iassenev
//	Description : dynamic obstacles avoider
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "dynamic_obstacles_avoider.h"
#include "ai_space.h"
#include "moving_objects.h"
#include "stalker_movement_manager_smart_cover.h"
#include "ai/stalker/ai_stalker.h"
#include "moving_object.h"

void dynamic_obstacles_avoider::query()
{
    ai().get_moving_objects().query_action_dynamic(object().get_moving_object());

    m_current_iteration.swap(object().get_moving_object()->dynamic_query());
}

bool dynamic_obstacles_avoider::movement_enabled() const
{
    switch (object().get_moving_object()->action())
    {
    case moving_object::action_wait: { return (false);
    }
    case moving_object::action_move: { return (true);
    }
    default: NODEFAULT;
    }
#ifdef DEBUG
    return (false);
#endif // DEBUG
}

bool dynamic_obstacles_avoider::process_query(const bool& change_path_state)
{
    if (!movement_enabled())
        return (true);

    m_inactive_query.set_intersection(m_current_iteration);
    m_active_query.set_intersection(m_current_iteration);

    return (inherited::process_query(change_path_state));
}
