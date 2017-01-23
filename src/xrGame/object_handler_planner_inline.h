////////////////////////////////////////////////////////////////////////////
//	Module 		: object_handler_planner_inline.h
//	Created 	: 11.03.2004
//  Modified 	: 01.12.2004
//	Author		: Dmitriy Iassenev
//	Description : Object handler action planner inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CAI_Stalker	&CObjectHandlerPlanner::object			() const
{
	VERIFY	(m_object);
	return	(*m_object);
}

IC	u32	CObjectHandlerPlanner::action_state_id			(_condition_type action_id) const
{
	return				(action_id & 0xffff);
}

IC	u32	CObjectHandlerPlanner::current_action_state_id	() const
{
	return				(action_state_id(current_action_id()));
}
