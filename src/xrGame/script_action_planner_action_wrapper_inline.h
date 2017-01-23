////////////////////////////////////////////////////////////////////////////
//	Module 		: script_action_planner_action_wrapper_inline.h
//	Created 	: 29.03.2004
//  Modified 	: 29.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Script action planner action wrapper inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CScriptActionPlannerActionWrapper::CScriptActionPlannerActionWrapper	(
	CScriptGameObject *object,
	LPCSTR action_name
) : 
	inherited(
		object,
		action_name
	)
{
}
