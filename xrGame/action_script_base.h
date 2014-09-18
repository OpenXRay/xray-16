////////////////////////////////////////////////////////////////////////////
//	Module 		: action_script_base.h
//	Created 	: 28.03.2004
//  Modified 	: 28.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Base action with script support
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "action_base.h"

class CScriptGameObject;

template <typename _object_type>
class CActionScriptBase : public CScriptActionBase {
protected:
	typedef CScriptActionBase inherited;

public:
	_object_type			*m_object;

public:
	IC						CActionScriptBase	(const xr_vector<COperatorCondition> &conditions, const xr_vector<COperatorCondition> &effects, _object_type *object = 0, LPCSTR action_name = "");
	IC						CActionScriptBase	(_object_type *object = 0, LPCSTR action_name = "");
	virtual					~CActionScriptBase	();
	virtual	void			setup				(_object_type *object, CPropertyStorage *storage);
	virtual	void			setup				(CScriptGameObject *object, CPropertyStorage *storage);
};

#include "action_script_base_inline.h"