////////////////////////////////////////////////////////////////////////////
//	Module 		: script_abstract_action.cpp
//	Created 	: 30.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script abstract action
////////////////////////////////////////////////////////////////////////////

#include "script_abstract_action.h"
#include "stdafx.h"

CScriptAbstractAction::~CScriptAbstractAction()
{
}

bool CScriptAbstractAction::completed()
{
    return (m_bCompleted);
}
