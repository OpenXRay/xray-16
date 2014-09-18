////////////////////////////////////////////////////////////////////////////
//	Module 		: script_animation_action.h
//	Created 	: 30.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script animation action class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "script_abstract_action.h"
#include "script_export_space.h"
#include "ai_monster_space.h"

class CScriptAnimationAction : public CScriptAbstractAction {
public:
	enum EGoalType {
		eGoalTypeAnimation = u32(0),
		eGoalTypeMental,
		eGoalTypeDummy = u32(-1),
	};

public:
	shared_str								m_caAnimationToPlay;
	MonsterSpace::EMentalState				m_tMentalState;
	EGoalType								m_tGoalType;
	bool									m_use_animation_movement_controller;
	MonsterSpace::EScriptMonsterAnimAction	m_tAnimAction;
	int										anim_index;

public:
	IC				CScriptAnimationAction	();
	IC				CScriptAnimationAction	(LPCSTR caAnimationToPlay, bool use_animation_movement_controller = false);
	IC				CScriptAnimationAction	(MonsterSpace::EMentalState tMentalState);
	// -------------------------------------------------------------------------------------------------
	// Monster
	// -------------------------------------------------------------------------------------------------
	IC				CScriptAnimationAction	(MonsterSpace::EScriptMonsterAnimAction tAnimAction, int index);
	virtual			~CScriptAnimationAction	();
	IC		void	SetAnimation			(LPCSTR caAnimationToPlay);
	IC		void	SetMentalState			(MonsterSpace::EMentalState tMentalState);
	IC		void	initialize				();

	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CScriptAnimationAction)
#undef script_type_list
#define script_type_list save_type_list(CScriptAnimationAction)

#include "script_animation_action_inline.h"