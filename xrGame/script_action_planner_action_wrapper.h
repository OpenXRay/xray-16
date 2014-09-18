////////////////////////////////////////////////////////////////////////////
//	Module 		: script_action_planner_action_wrapper.h
//	Created 	: 29.03.2004
//  Modified 	: 29.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Script action planner action wrapper
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "action_planner_action.h"

class CScriptGameObject;

class CScriptActionPlannerActionWrapper : public CScriptActionPlannerAction, public luabind::wrap_base {
	typedef CScriptActionPlannerAction inherited;
public:
	IC			 CScriptActionPlannerActionWrapper	(CScriptGameObject *object = 0, LPCSTR action_name = "");
	virtual void				setup				(CScriptGameObject *object, CPropertyStorage *storage);
	static	void				setup_static		(CScriptActionPlannerAction *action, CScriptGameObject *object, CPropertyStorage *storage);
	virtual void				initialize			();
	static	void				initialize_static	(CScriptActionPlannerAction *action);
	virtual void				execute				();
	static	void				execute_static		(CScriptActionPlannerAction *action);
	virtual void				finalize			();
	static	void				finalize_static		(CScriptActionPlannerAction *action);
	virtual _edge_value_type	weight				(const CSConditionState &condition0, const CSConditionState &condition1) const;
	static	_edge_value_type	weight_static		(CScriptActionPlannerAction *action, const CSConditionState &condition0, const CSConditionState &condition1);
};

#include "script_action_planner_action_wrapper_inline.h"