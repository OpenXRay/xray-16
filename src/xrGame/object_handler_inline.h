////////////////////////////////////////////////////////////////////////////
//	Module 		: object_handler_inline.h
//	Created 	: 11.03.2004
//  Modified 	: 11.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Object handler inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	bool CObjectHandler::hammer_is_clutched			() const
{
	return				(m_hammer_is_clutched);
}

IC	CObjectHandlerPlanner &CObjectHandler::planner	() const
{
	VERIFY				(m_planner);
	return				(*m_planner);
}

IC	bool const& CObjectHandler::infinite_ammo		() const
{
	return				(m_infinite_ammo);
}
