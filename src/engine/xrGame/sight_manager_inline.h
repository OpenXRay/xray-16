////////////////////////////////////////////////////////////////////////////
//	Module 		: sight_manager_inline.h
//	Created 	: 27.12.2003
//  Modified 	: 08.04.2008
//	Author		: Dmitriy Iassenev
//	Description : Sight manager inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	bool CSightManager::use_torso_look				() const
{
	if (!m_actions.empty())
		return			(current_action().use_torso_look());
	else
		return			(true);
}

IC	bool CSightManager::turning_in_place			() const
{
	return				(m_turning_in_place);
}

IC	bool CSightManager::enabled						() const
{
	return				(m_enabled);
}

template <typename T1, typename T2, typename T3>
IC	void CSightManager::setup						(T1 _1, T2 _2, T3 _3)
{
	setup				(CSightAction(_1,_2,_3));
}

template <typename T1, typename T2>
IC	void CSightManager::setup						(T1 _1, T2 _2)
{
	setup				(CSightAction(_1,_2));
}

template <typename T1>
IC	void CSightManager::setup						(T1 _1)
{
	setup				(CSightAction(_1));
}

IC	Fmatrix	const& CSightManager::current_spine_rotation	() const
{
	return				(m_current.m_spine.m_rotation);
}

IC	Fmatrix	const& CSightManager::current_shoulder_rotation	() const
{
	return				(m_current.m_shoulder.m_rotation);
}

IC	Fmatrix	const& CSightManager::current_head_rotation		() const
{
	return				(m_current.m_head.m_rotation);
}

IC	void CSightManager::bone_aiming					()
{
	m_animation_id		= "";
	m_aiming_type		= aiming_none;
}

IC	void CSightManager::bone_aiming					(
		shared_str const& animation_id,
		animation_frame_type const animation_frame,
		aiming_type const aiming_type
	)
{
	VERIFY				(aiming_none != aiming_type);

	m_animation_id		= animation_id;
	m_animation_frame	= animation_frame;
	m_aiming_type		= aiming_type;
}