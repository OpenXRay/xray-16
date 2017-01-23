////////////////////////////////////////////////////////////////////////////
//	Module 		: script_action_condition_inline.h
//	Created 	: 30.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script action condition class inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CScriptActionCondition::CScriptActionCondition	()
{
	m_dwFlags			= 0;
	m_tLifeTime			= ALife::_TIME_ID(-1);
	m_tStartTime		= ALife::_TIME_ID(-1);
}

IC	CScriptActionCondition::CScriptActionCondition	(u32 dwFlags, double dTime)
{
	m_dwFlags			= dwFlags;
	m_tLifeTime			= ALife::_TIME_ID(dTime);
	m_tStartTime		= ALife::_TIME_ID(-1);
}

IC	void CScriptActionCondition::initialize			()
{
	m_tStartTime		= Device.dwTimeGlobal;
}
