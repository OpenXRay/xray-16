////////////////////////////////////////////////////////////////////////////
//	Module 		: action_planner_action_script.h
//	Created 	: 07.07.2004
//  Modified 	: 07.07.2004
//	Author		: Dmitriy Iassenev
//	Description : Action planner action with script support
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "action_planner_action.h"

class CScriptGameObject;

template <typename _object_type>
class CActionPlannerActionScript : public CScriptActionPlannerAction {
protected:
	typedef CScriptActionPlannerAction inherited;

public:
	_object_type	*m_object;

public:
	IC				CActionPlannerActionScript	(const xr_vector<COperatorCondition> &conditions, const xr_vector<COperatorCondition> &effects, _object_type *object = 0, LPCSTR action_name = "");
	IC				CActionPlannerActionScript	(_object_type *object = 0, LPCSTR action_name = "");
	virtual			~CActionPlannerActionScript	();
	virtual	void	setup						(_object_type *object, CPropertyStorage *storage);
	virtual	void	setup						(CScriptGameObject *object, CPropertyStorage *storage);
	IC		_object_type &object				() const;
};

#include "action_planner_action_script_inline.h"