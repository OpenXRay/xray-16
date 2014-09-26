////////////////////////////////////////////////////////////////////////////
//	Module 		: script_entity_action.h
//	Created 	: 30.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script entity action class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "script_export_space.h"
#include "script_movement_action.h"
#include "script_watch_action.h"
#include "script_animation_action.h"
#include "script_sound_action.h"
#include "script_particle_action.h"
#include "script_object_action.h"
#include "script_action_condition.h"
#include "script_monster_action.h"

class CScriptEntityAction {
public:
	CScriptMovementAction	m_tMovementAction;
	CScriptWatchAction		m_tWatchAction;
	CScriptAnimationAction	m_tAnimationAction;
	CScriptSoundAction		m_tSoundAction;
	CScriptParticleAction	m_tParticleAction;
	CScriptObjectAction		m_tObjectAction;
	CScriptActionCondition	m_tActionCondition;
	CScriptMonsterAction	m_tMonsterAction;
	void					*m_user_data;
	bool					m_started;

public:
	IC				CScriptEntityAction					();
	IC				CScriptEntityAction					(const CScriptEntityAction *entity_action);
	virtual			~CScriptEntityAction				();
	template<typename T>
	IC		void	SetAction							(const T &t, T &tt);
	IC		void	SetAction							(CScriptMovementAction &tMovementAction);
	IC		void	SetAction							(CScriptWatchAction &tWatchAction);
	IC		void	SetAction							(CScriptAnimationAction &tAnimationAction);
	IC		void	SetAction							(CScriptSoundAction &tSoundAction);
	IC		void	SetAction							(CScriptParticleAction &tParticleAction);
	IC		void	SetAction							(CScriptObjectAction &tObjectAction);
	IC		void	SetAction							(CScriptActionCondition &tActionCondition);
	IC		void	SetAction							(CScriptMonsterAction &tMonsterAction);
	IC		void	SetAction							(void *user_data);
	IC		bool	CheckIfActionCompleted				(const CScriptAbstractAction &tAbstractAction) const;
	IC		bool	CheckIfMovementCompleted			() const;
	IC		bool	CheckIfWatchCompleted				() const;
	IC		bool	CheckIfAnimationCompleted			() const;
	IC		bool	CheckIfSoundCompleted				() const;
	IC		bool	CheckIfParticleCompleted			() const;
	IC		bool	CheckIfObjectCompleted				() const;
	IC		bool	CheckIfMonsterActionCompleted		() const;
	IC		bool	CheckIfTimeOver						();
	IC		bool	CheckIfActionCompleted				();
	IC		void	initialize							();

public:
	IC		const CScriptMovementAction		&move		();
	IC		const CScriptWatchAction		&look		();
	IC		const CScriptAnimationAction	&anim		();
	IC		const CScriptParticleAction		&particle	();
	IC		const CScriptObjectAction		&object		();
	IC		const CScriptActionCondition	&cond		();
	IC		void							*data		();

	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CScriptEntityAction)
#undef script_type_list
#define script_type_list save_type_list(CScriptEntityAction)

#include "script_entity_action_inline.h"