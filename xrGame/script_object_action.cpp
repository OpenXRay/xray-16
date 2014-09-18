////////////////////////////////////////////////////////////////////////////
//	Module 		: script_object_action.h
//	Created 	: 30.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script object action class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "script_object_action.h"
#include "script_game_object.h"

CScriptObjectAction::~CScriptObjectAction	()
{
}

void CScriptObjectAction::SetObject			(CScriptGameObject *tpLuaGameObject)
{
	m_tpObject			= tpLuaGameObject->operator CObject*();
	m_bCompleted		= false;
}
