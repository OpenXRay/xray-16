////////////////////////////////////////////////////////////////////////////
//	Module 		: script_effector_wrapper.h
//	Created 	: 06.02.2004
//  Modified 	: 06.02.2004
//	Author		: Dmitriy Iassenev
//	Description : XRay Script effector wrapper class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "script_effector.h"

class CScriptEffectorWrapper : public CScriptEffector, public luabind::wrap_base {
public:
	IC				CScriptEffectorWrapper	(int iType, float fTime);
	virtual			~CScriptEffectorWrapper	();
	virtual bool	process					(SPPInfo *pp);
	static	bool	process_static			(CScriptEffector *tpLuaEffector, SPPInfo *pp);
};

#include "script_effector_wrapper_inline.h"