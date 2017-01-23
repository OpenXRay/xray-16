////////////////////////////////////////////////////////////////////////////
//	Module 		: script_effector_wrapper.cpp
//	Created 	: 06.02.2004
//  Modified 	: 06.02.2004
//	Author		: Dmitriy Iassenev
//	Description : XRay Script effector wrapper class
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_effector_wrapper.h"

CScriptEffectorWrapper::~CScriptEffectorWrapper	()
{
}

bool CScriptEffectorWrapper::process		(SPPInfo *pp)
{
	return		(luabind::call_member<bool>(this,"process",pp));
}

bool CScriptEffectorWrapper::process_static	(CScriptEffector *tpLuaEffector, SPPInfo *pp)
{
	return		(!!tpLuaEffector->CScriptEffector::process(pp));
}
