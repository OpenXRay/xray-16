////////////////////////////////////////////////////////////////////////////
//	Module 		: script_action_wrapper.h
//	Created 	: 19.03.2004
//  Modified 	: 26.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Script action wrapper
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "action_base.h"

class CScriptActionWrapper : public CScriptActionBase, public luabind::wrap_base {
public:
	IC							CScriptActionWrapper(CScriptGameObject *object = 0, LPCSTR action_name = "");
	virtual void				setup				(CScriptGameObject *object, CPropertyStorage *storage);
	static	void				setup_static		(CScriptActionBase *action, CScriptGameObject *object, CPropertyStorage *storage);
	virtual void				initialize			();
	static	void				initialize_static	(CScriptActionBase *action);
	virtual void				execute				();
	static	void				execute_static		(CScriptActionBase *action);
	virtual void				finalize			();
	static	void				finalize_static		(CScriptActionBase *action);
//	virtual _edge_value_type	weight				(const CSConditionState &condition0, const CSConditionState &condition1) const;
//	static	_edge_value_type	weight_static		(CScriptActionBase *action, const CSConditionState &condition0, const CSConditionState &condition1);
};

#include "script_action_wrapper_inline.h"