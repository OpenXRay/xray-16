////////////////////////////////////////////////////////////////////////////
//	Module 		: script_monster_action.h
//	Created 	: 30.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script monster action class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "script_monster_action.h"
#include "script_game_object.h"

CScriptMonsterAction::~CScriptMonsterAction	()
{
}

void CScriptMonsterAction::SetObject	(CScriptGameObject *tObj)
{
	m_tObject	= tObj->operator CObject*();
}
