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
#include "sight_manager_space.h"

CScriptWatchAction::CScriptWatchAction()
{
    m_tpObjectToWatch = 0;
    m_tWatchType = SightManager::eSightTypeCurrentDirection;
    m_tWatchVector.set(0, 0, 0);
    m_tGoalType = eGoalTypeCurrent;
    m_bCompleted = true;
}

CScriptWatchAction::~CScriptWatchAction() {}
void CScriptWatchAction::SetWatchObject(CScriptGameObject* tpObjectToWatch)
{
    m_tpObjectToWatch = tpObjectToWatch->operator IGameObject*();
    m_tGoalType = eGoalTypeObject;
    m_bCompleted = false;
}
