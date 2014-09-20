////////////////////////////////////////////////////////////////////////////
//	Module 		: script_entity_action_inline.h
//	Created 	: 30.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script entity action class inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CScriptEntityAction::CScriptEntityAction						()
{
	m_user_data					= 0;
	m_started					= false;
}

IC	CScriptEntityAction::CScriptEntityAction						(const CScriptEntityAction *entity_action)
{
	*this						= *entity_action;
}

template<typename T>
IC	void CScriptEntityAction::SetAction						(const T &t, T &tt)
{
	tt					= t;
}

IC	void CScriptEntityAction::SetAction						(CScriptMovementAction &tMovementAction)
{
	SetAction			(tMovementAction,m_tMovementAction);
}

IC	void CScriptEntityAction::SetAction						(CScriptWatchAction &tWatchAction)
{
	SetAction			(tWatchAction,m_tWatchAction);
}

IC	void CScriptEntityAction::SetAction						(CScriptAnimationAction &tAnimationAction)
{
	SetAction			(tAnimationAction,m_tAnimationAction);
}

IC	void CScriptEntityAction::SetAction						(CScriptSoundAction &tSoundAction)
{
	SetAction			(tSoundAction,m_tSoundAction);
}

IC	void CScriptEntityAction::SetAction						(CScriptParticleAction &tParticleAction)
{
	SetAction			(tParticleAction,m_tParticleAction);
}

IC	void CScriptEntityAction::SetAction						(CScriptObjectAction &tObjectAction)
{
	SetAction			(tObjectAction,m_tObjectAction);
}

IC	void CScriptEntityAction::SetAction						(CScriptActionCondition &tActionCondition)
{
	SetAction			(tActionCondition,m_tActionCondition);
}

IC	void CScriptEntityAction::SetAction						(CScriptMonsterAction &tMonsterAction)
{
	SetAction			(tMonsterAction,m_tMonsterAction);
}

IC	void CScriptEntityAction::SetAction						(void *user_data)
{
	m_user_data			= user_data;
}

IC	bool CScriptEntityAction::CheckIfActionCompleted			(const CScriptAbstractAction &tAbstractAction) const
{
	return				(tAbstractAction.m_bCompleted);
}

IC	bool CScriptEntityAction::CheckIfMovementCompleted		() const
{
	return				(CheckIfActionCompleted(m_tMovementAction));
}

IC	bool CScriptEntityAction::CheckIfWatchCompleted			() const
{
	return				(CheckIfActionCompleted(m_tWatchAction));
}

IC	bool CScriptEntityAction::CheckIfAnimationCompleted		() const
{
	return				(CheckIfActionCompleted(m_tAnimationAction));
}

IC	bool CScriptEntityAction::CheckIfSoundCompleted			() const
{
	return				(CheckIfActionCompleted(m_tSoundAction));
}

IC	bool CScriptEntityAction::CheckIfParticleCompleted		() const
{
	return				(CheckIfActionCompleted(m_tParticleAction));
}

IC	bool CScriptEntityAction::CheckIfObjectCompleted			() const
{
	return				(CheckIfActionCompleted(m_tObjectAction));
}

IC	bool CScriptEntityAction::CheckIfMonsterActionCompleted	() const
{
	return				(CheckIfActionCompleted(m_tMonsterAction));
}

IC	bool CScriptEntityAction::CheckIfTimeOver					()
{
	return((m_tActionCondition.m_tLifeTime >= 0) && ((m_tActionCondition.m_tStartTime + m_tActionCondition.m_tLifeTime) < Device.dwTimeGlobal));
}

IC	bool CScriptEntityAction::CheckIfActionCompleted			()
{
	bool				started = m_started;
	m_started			= true;
	if (!started)
		return			(false);

	u32					l_dwFlags = m_tActionCondition.m_dwFlags;
	if ((CScriptActionCondition::MOVEMENT_FLAG	& m_tActionCondition.m_dwFlags)	&& CheckIfMovementCompleted	())
		l_dwFlags		^= CScriptActionCondition::MOVEMENT_FLAG;
	if ((CScriptActionCondition::WATCH_FLAG		& m_tActionCondition.m_dwFlags)	&& CheckIfWatchCompleted	())
		l_dwFlags		^= CScriptActionCondition::WATCH_FLAG;
	if ((CScriptActionCondition::ANIMATION_FLAG	& m_tActionCondition.m_dwFlags)	&& CheckIfAnimationCompleted())
		l_dwFlags		^= CScriptActionCondition::ANIMATION_FLAG;
	if ((CScriptActionCondition::SOUND_FLAG		& m_tActionCondition.m_dwFlags)	&& CheckIfSoundCompleted	())
		l_dwFlags		^= CScriptActionCondition::SOUND_FLAG;
	if ((CScriptActionCondition::PARTICLE_FLAG	& m_tActionCondition.m_dwFlags)	&& CheckIfParticleCompleted	())
		l_dwFlags		^= CScriptActionCondition::PARTICLE_FLAG;
	if ((CScriptActionCondition::OBJECT_FLAG		& m_tActionCondition.m_dwFlags)	&& CheckIfObjectCompleted	())
		l_dwFlags		^= CScriptActionCondition::OBJECT_FLAG;
	if ((CScriptActionCondition::TIME_FLAG		& m_tActionCondition.m_dwFlags)	&& CheckIfTimeOver			())
		l_dwFlags		^= CScriptActionCondition::TIME_FLAG;
	if ((CScriptActionCondition::ACT_FLAG			& m_tActionCondition.m_dwFlags)	&& CheckIfMonsterActionCompleted())
		l_dwFlags		^= CScriptActionCondition::ACT_FLAG;

	if (!m_tActionCondition.m_dwFlags && (m_tActionCondition.m_tLifeTime < 0) 
		&& CheckIfMovementCompleted()
		&& CheckIfWatchCompleted()
		&& CheckIfAnimationCompleted()
		&& CheckIfSoundCompleted()
		&& CheckIfParticleCompleted()
		&& CheckIfObjectCompleted()
		&& CheckIfMonsterActionCompleted()
		)
		return			(true);
	else
		return			(!l_dwFlags);
}

IC	void CScriptEntityAction::initialize						()
{
	m_started						= false;
	m_tMovementAction.initialize	();
	m_tWatchAction.initialize		();
	m_tAnimationAction.initialize	();
	m_tSoundAction.initialize		();
	m_tParticleAction.initialize	();
	m_tObjectAction.initialize		();
	m_tActionCondition.initialize	();
}

IC	const CScriptMovementAction	&CScriptEntityAction::move	()
{
	return				(m_tMovementAction);
}

IC	const CScriptWatchAction &CScriptEntityAction::look		()
{
	return				(m_tWatchAction);
}

IC	const CScriptAnimationAction &CScriptEntityAction::anim	()
{
	return				(m_tAnimationAction);
}

IC	const CScriptParticleAction	&CScriptEntityAction::particle()
{
	return				(m_tParticleAction);
}

IC	const CScriptObjectAction &CScriptEntityAction::object	()
{
	return				(m_tObjectAction);
}

IC	const CScriptActionCondition &CScriptEntityAction::cond			()
{
	return				(m_tActionCondition);
}

IC	void *CScriptEntityAction::data							()
{
	return				(m_user_data);
}
