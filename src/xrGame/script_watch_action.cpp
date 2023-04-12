////////////////////////////////////////////////////////////////////////////
//	Module 		: script_watch_action.cpp
//	Created 	: 30.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script watch action class
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "script_watch_action.h"
#include "script_game_object.h"

void CScriptWatchAction::SetWatchObject(CScriptGameObject* tpObjectToWatch)
{
    m_tpObjectToWatch = tpObjectToWatch->operator IGameObject*();
    m_tGoalType = eGoalTypeObject;
    m_bCompleted = false;
}
