////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover_animation_planner_inline.h
//	Created 	: 04.09.2007
//	Author		: Alexander Dudin
//	Description : Smart cover animation planner class inline functions
////////////////////////////////////////////////////////////////////////////

#ifndef SMART_COVER_ANIMATION_PLANNER_INLINE_H_INCLUDED
#define SMART_COVER_ANIMATION_PLANNER_INLINE_H_INCLUDED

namespace smart_cover {

IC u32 const &animation_planner::time_object_hit		() const
{
	return				(m_time_object_hit);
}

IC u32 const &animation_planner::loophole_value			() const
{
	return				(m_loophole_value);
}

IC void animation_planner::decrease_loophole_value		(u32 const &value)
{
	m_loophole_value	-= value;
}

IC u32 const &animation_planner::last_transition_time	() const
{
	return				(m_last_transition_time);
}

IC void animation_planner::last_transition_time			(u32 const &value)
{
	m_last_transition_time = value;
}

IC CPropertyStorage *animation_planner::property_storage()
{
	return				(&m_storage);
}

IC shared_str animation_planner::cName					() const
{
	return				("animation planner");
}

IC u32 animation_planner::default_idle_interval		()
{
	return				(u32(1000 * m_random.randF(m_idle_min_time, m_idle_max_time)));
}

IC u32 animation_planner::default_lookout_interval()
{
	return				(u32(1000 * m_random.randF(m_lookout_min_time, m_lookout_max_time)));
}

IC float const &animation_planner::idle_min_time		() const
{
	return				(m_idle_min_time);
}

IC void	animation_planner::idle_min_time				(float const &value)
{
	m_idle_min_time		= value;
}

IC float const &animation_planner::idle_max_time		() const
{
	return				(m_idle_max_time);
}

IC void	animation_planner::idle_max_time				(float const &value)
{
	m_idle_max_time		= value;
}

IC float const &animation_planner::lookout_min_time		() const
{
	return				(m_lookout_min_time);
}

IC void	animation_planner::lookout_min_time				(float const &value)
{
	m_lookout_min_time	= value;
}

IC float const &animation_planner::lookout_max_time		() const
{
	return				(m_lookout_max_time);
}

IC void	animation_planner::lookout_max_time				(float const &value)
{
	m_lookout_max_time	= value;
}

IC bool const &animation_planner::stay_idle				() const
{
	return				(m_stay_idle);
}

IC void animation_planner::stay_idle					(bool const &value)
{
	m_stay_idle			= value;
}

IC u32 const &animation_planner::last_idle_time			() const
{
	return				(m_last_idle_time);
}

IC void animation_planner::last_idle_time				(u32 const &value)
{
	m_last_idle_time	= value;
}

IC u32 const &animation_planner::last_lookout_time		() const
{
	return				(m_last_lookout_time);
}

IC void animation_planner::last_lookout_time			(u32 const &value)
{
	m_last_lookout_time	= value;
}

} // namespace smart_cover

#endif // SMART_COVER_ANIMATION_PLANNER_INLINE_H_INCLUDED