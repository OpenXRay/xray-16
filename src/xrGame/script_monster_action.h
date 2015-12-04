////////////////////////////////////////////////////////////////////////////
//	Module 		: script_monster_action.h
//	Created 	: 30.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script monster action class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "script_abstract_action.h"
#include "ai_monster_space.h"

class CScriptGameObject;

class CScriptMonsterAction : public CScriptAbstractAction {
public: 
	MonsterSpace::EScriptMonsterGlobalAction	m_tAction;
	IGameObject										*m_tObject;

public:
	IC				CScriptMonsterAction	();
	IC				CScriptMonsterAction	(MonsterSpace::EScriptMonsterGlobalAction action);
	IC				CScriptMonsterAction	(MonsterSpace::EScriptMonsterGlobalAction action, CScriptGameObject *tObj);
	virtual			~CScriptMonsterAction	();
			void	SetObject				(CScriptGameObject *tObj);
};

#include "script_monster_action_inline.h"
