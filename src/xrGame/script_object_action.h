////////////////////////////////////////////////////////////////////////////
//	Module 		: script_object_action.h
//	Created 	: 30.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script object action class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "script_abstract_action.h"

#include "ai_monster_space.h"

class CScriptGameObject;

class CScriptObjectAction : public CScriptAbstractAction {
public:
	IGameObject								*m_tpObject;
	MonsterSpace::EObjectAction			m_tGoalType;
	u32									m_dwQueueSize;
	shared_str							m_caBoneName;

public:
	IC				CScriptObjectAction	();
	IC				CScriptObjectAction	(CScriptGameObject *tpLuaGameObject, MonsterSpace::EObjectAction tObjectActionType, u32 dwQueueSize = u32(-1));
	IC				CScriptObjectAction	(LPCSTR caBoneName, MonsterSpace::EObjectAction tObjectActionType);
	IC				CScriptObjectAction	(MonsterSpace::EObjectAction tObjectActionType);
	virtual			~CScriptObjectAction();
			void	SetObject			(CScriptGameObject *tpLuaGameObject);
	IC		void	SetObject			(LPCSTR	caBoneName);
	IC		void	SetObjectAction		(MonsterSpace::EObjectAction tObjectActionType);
	IC		void	SetQueueSize		(u32 dwQueueSize);
	IC		void	initialize			();
};

#include "script_object_action_inline.h"
