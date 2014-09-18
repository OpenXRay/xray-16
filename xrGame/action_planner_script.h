////////////////////////////////////////////////////////////////////////////
//	Module 		: action_planner_script.h
//	Created 	: 28.03.2004
//  Modified 	: 28.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Action planner with script support
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "action_planner.h"

class CScriptGameObject;

template <typename _object_type>
class CActionPlannerScript : public CScriptActionPlanner {
protected:
	typedef CScriptActionPlanner inherited;

public:
	_object_type			*m_object;

public:
	IC						CActionPlannerScript	();
	virtual	void			setup					(_object_type *object);
	IC		_object_type	&object					() const;
};

#include "action_planner_script_inline.h"