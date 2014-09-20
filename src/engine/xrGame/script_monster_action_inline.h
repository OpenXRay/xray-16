////////////////////////////////////////////////////////////////////////////
//	Module 		: script_monster_action_inline.h
//	Created 	: 30.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script monster action class inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CScriptMonsterAction::CScriptMonsterAction	()
{
	m_tAction		= MonsterSpace::eGA_None;
	m_bCompleted	= true;
}

IC	CScriptMonsterAction::CScriptMonsterAction	(MonsterSpace::EScriptMonsterGlobalAction action)
{
	m_tAction		= action;
	m_bCompleted	= false;
}

IC	CScriptMonsterAction::CScriptMonsterAction	(MonsterSpace::EScriptMonsterGlobalAction action, CScriptGameObject *tObj)
{
	m_tAction		= action;
	m_bCompleted	= false;
	SetObject		(tObj);
}
