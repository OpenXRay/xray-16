////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_movement_manager_smart_cover_inline.h
//	Created 	: 27.12.2003
//	Modified	: 13.02.2008
//	Author		: Dmitriy Iassenev
//	Description : stalker movement manager class with smart covers inline functions
////////////////////////////////////////////////////////////////////////////

#ifndef STALKER_MOVEMENT_MANAGER_SMART_COVER_INLINE_H_INCLUDED
#define STALKER_MOVEMENT_MANAGER_SMART_COVER_INLINE_H_INCLUDED

IC stalker_movement_manager_smart_cover::animation_selector_type& stalker_movement_manager_smart_cover::animation_selector	() const
{
	VERIFY							(m_animation_selector);
	return							(*m_animation_selector);
}

IC	void stalker_movement_manager_smart_cover::property_storage								(CPropertyStorage *storage)
{
	m_property_storage				= storage;
}

IC	stalker_movement_manager_smart_cover::animation_action const	&stalker_movement_manager_smart_cover::current_transition_animation	() const
{
	VERIFY							(m_current_transition_animation);
	return							(*m_current_transition_animation);
}

IC	void stalker_movement_manager_smart_cover::non_animated_loophole_change					(bool const &value)
{
	m_non_animated_loophole_change	= value;
}

IC	float const &stalker_movement_manager_smart_cover::apply_loophole_direction_distance	() const
{
	return							(m_apply_loophole_direction_distance);
}

IC	void stalker_movement_manager_smart_cover::apply_loophole_direction_distance			(float const &value)
{
	m_apply_loophole_direction_distance	= value;
}

IC	stalker_movement_manager_smart_cover::target_selector_type& stalker_movement_manager_smart_cover::target_selector	()
{
	VERIFY							(m_target_selector);
	return							(*m_target_selector);
}

IC	bool stalker_movement_manager_smart_cover::entering_smart_cover_with_animation			() const
{
	return							(m_entering_smart_cover_with_animation);
}

IC	bool stalker_movement_manager_smart_cover::check_can_kill_enemy							() const
{
	return							(m_check_can_kill_enemy);
}

IC	void stalker_movement_manager_smart_cover::check_can_kill_enemy							(bool const value)
{
	m_check_can_kill_enemy			= value;
}

IC	void stalker_movement_manager_smart_cover::combat_behaviour								(bool const value)
{
	m_combat_behaviour				= value;
}

IC	bool stalker_movement_manager_smart_cover::combat_behaviour								() const
{
	return							(m_combat_behaviour);
}

#endif // #ifndef STALKER_MOVEMENT_MANAGER_SMART_COVER_INLINE_H_INCLUDED