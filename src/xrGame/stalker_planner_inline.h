////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_planner_inline.h
//	Created 	: 26.03.2004
//  Modified 	: 26.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Stalker planner class inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	void CStalkerPlanner::affect_cover	(bool value)
{
	m_affect_cover	= value;
}

IC	bool CStalkerPlanner::affect_cover	() const
{
	return			(m_affect_cover);
}
