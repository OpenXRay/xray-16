////////////////////////////////////////////////////////////////////////////
//	Module 		: script_object_action.h
//	Created 	: 30.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script object action class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "script_abstract_action.h"
#include "script_export_space.h"
#include "ai_monster_space.h"

class CScriptGameObject;

class CScriptObjectAction : public CScriptAbstractAction {
public:
	CObject								*m_tpObject;
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

	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CScriptObjectAction)
#undef script_type_list
#define script_type_list save_type_list(CScriptObjectAction)

#include "script_object_action_inline.h"