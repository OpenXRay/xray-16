////////////////////////////////////////////////////////////////////////////
//	Module 		: script_abstract_action.cpp
//	Created 	: 30.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script abstract action
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "script_abstract_action.h"

CScriptAbstractAction::~CScriptAbstractAction() {}
bool CScriptAbstractAction::completed() { return (m_bCompleted); }
