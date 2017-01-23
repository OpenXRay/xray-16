////////////////////////////////////////////////////////////////////////////
//	Module 		: script_action_planner_action_wrapper.cpp
//	Created 	: 29.03.2004
//  Modified 	: 29.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Script action planner action wrapper
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_action_planner_action_wrapper.h"
#include "script_game_object.h"

void CScriptActionPlannerActionWrapper::setup		(CScriptGameObject *object, CPropertyStorage *storage)
{
	luabind::call_member<void>			(this,"setup",object,storage);
}

void CScriptActionPlannerActionWrapper::setup_static(CScriptActionPlannerAction *planner, CScriptGameObject *object, CPropertyStorage *storage)
{
	planner->CScriptActionPlannerAction::setup	(object,storage);
}

void CScriptActionPlannerActionWrapper::initialize			()
{
	luabind::call_member<void>			(this,"initialize");
}

void CScriptActionPlannerActionWrapper::initialize_static	(CScriptActionPlannerAction *action)
{
	action->CScriptActionPlannerAction::initialize	();
}

void CScriptActionPlannerActionWrapper::execute				()
{
	luabind::call_member<void>			(this,"execute");
}

void CScriptActionPlannerActionWrapper::execute_static		(CScriptActionPlannerAction *action)
{
	action->CScriptActionPlannerAction::execute		();
}

void CScriptActionPlannerActionWrapper::finalize				()
{
	luabind::call_member<void>			(this,"finalize");
}

void CScriptActionPlannerActionWrapper::finalize_static		(CScriptActionPlannerAction *action)
{
	action->CScriptActionPlannerAction::finalize		();
}

CScriptActionPlannerActionWrapper::_edge_value_type CScriptActionPlannerActionWrapper::weight	(const CSConditionState &condition0, const CSConditionState &condition1) const
{
	return								(luabind::call_member<_edge_value_type>(const_cast<CScriptActionPlannerActionWrapper*>(this),"weight",condition0,condition1));
}

CScriptActionPlannerActionWrapper::_edge_value_type CScriptActionPlannerActionWrapper::weight_static	(CScriptActionPlannerAction *action, const CSConditionState &condition0, const CSConditionState &condition1)
{
	return								(((const CScriptActionPlannerActionWrapper*)action)->CScriptActionPlannerAction::weight(condition0,condition1));
}
