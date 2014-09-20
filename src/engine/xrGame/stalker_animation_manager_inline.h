////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_animation_manager_inline.h
//	Created 	: 25.02.2003
//  Modified 	: 19.11.2004
//	Author		: Dmitriy Iassenev
//	Description : Stalker animation manager inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	const CStalkerAnimationManager::SCRIPT_ANIMATIONS &CStalkerAnimationManager::script_animations	() const
{
	return							(m_script_animations);
}

IC	CStalkerAnimationPair &CStalkerAnimationManager::global		()
{
	return							(m_global);
}

IC	CStalkerAnimationPair &CStalkerAnimationManager::head		()
{
	return							(m_head);
}

IC	CStalkerAnimationPair &CStalkerAnimationManager::torso		()
{
	return							(m_torso);
}

IC	CStalkerAnimationPair &CStalkerAnimationManager::legs		()
{
	return							(m_legs);
}

IC	CStalkerAnimationPair &CStalkerAnimationManager::script		()
{
	return							(m_script);
}

IC	CAI_Stalker	&CStalkerAnimationManager::object				() const
{
	VERIFY							(m_object);
	return							(*m_object);
}

IC	void CStalkerAnimationManager::pop_script_animation			()
{
	VERIFY							(!script_animations().empty());
	m_script_animations.pop_front	();
	script().reset					();
}

IC	void CStalkerAnimationManager::clear_script_animations		()
{
	m_script_animations.clear		();
	script().reset					();
}

IC	bool CStalkerAnimationManager::non_script_need_update		() const
{
	return							(
		m_global_selector ||
		m_global_callback ||
		m_global.need_update() ||
		m_head.need_update() ||
		m_torso.need_update() ||
		m_legs.need_update()
	);
}

IC	const float &CStalkerAnimationManager::target_speed			() const
{
	return							(m_last_non_zero_speed);
}

IC	void CStalkerAnimationManager::special_danger_move			(const bool &value)
{
	m_special_danger_move			= value;
}

IC	const bool &CStalkerAnimationManager::special_danger_move	() const
{
	return							(m_special_danger_move);
}

IC	CStalkerAnimationManager::AnimationSelector const &CStalkerAnimationManager::global_selector() const
{
	return							(m_global_selector);
}

IC	void CStalkerAnimationManager::global_selector				(AnimationSelector const &selector)
{
	m_global_selector				= selector;
}

IC	CStalkerAnimationManager::AnimationCallback const &CStalkerAnimationManager::global_callback() const
{
	return							(m_global_callback);
}

IC	void CStalkerAnimationManager::global_callback	(AnimationCallback const &callback)
{
	m_global_callback				= callback;
}

IC	CStalkerAnimationManager::AnimationModifier const &CStalkerAnimationManager::global_modifier() const
{
	return							(m_global_modifier);
}

IC	void CStalkerAnimationManager::global_modifier				(AnimationModifier const &modifier)
{
	m_global_modifier				= modifier;
}

IC CStalkerAnimationData const &CStalkerAnimationManager::data_storage	() const
{
	VERIFY							(m_data_storage);
	return							(*m_data_storage);
}