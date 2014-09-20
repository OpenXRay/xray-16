////////////////////////////////////////////////////////////////////////////
//	Module 		: script_object_action_inline.h
//	Created 	: 30.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script object action class inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CScriptObjectAction::CScriptObjectAction	()
{
	m_tpObject			= 0;
	m_tGoalType			= MonsterSpace::eObjectActionIdle;
	m_bCompleted		= true;
}

IC	CScriptObjectAction::CScriptObjectAction	(CScriptGameObject *tpLuaGameObject, MonsterSpace::EObjectAction tObjectActionType, u32 dwQueueSize)
{
	SetObject			(tpLuaGameObject);
	SetObjectAction		(tObjectActionType);
	SetQueueSize		(dwQueueSize);
}

IC	CScriptObjectAction::CScriptObjectAction	(LPCSTR caBoneName, MonsterSpace::EObjectAction tObjectActionType)
{
	SetObject			(caBoneName);
	SetObjectAction		(tObjectActionType);
}

IC	CScriptObjectAction::CScriptObjectAction	(MonsterSpace::EObjectAction tObjectActionType)
{
	SetObjectAction		(tObjectActionType);
}

IC	void CScriptObjectAction::SetObject			(LPCSTR	caBoneName)
{
	m_caBoneName		= caBoneName;
	m_bCompleted		= false;
}

IC	void CScriptObjectAction::SetObjectAction	(MonsterSpace::EObjectAction tObjectActionType)
{
	m_tGoalType			= tObjectActionType;
	m_bCompleted		= false;
}

IC	void CScriptObjectAction::SetQueueSize		(u32 dwQueueSize)
{
	m_dwQueueSize		= dwQueueSize;
	m_bCompleted		= false;
}

IC	void CScriptObjectAction::initialize		()
{
}
