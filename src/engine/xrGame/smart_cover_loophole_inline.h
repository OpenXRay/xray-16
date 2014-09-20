////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover_loophole_inline.h
//	Created 	: 29.08.2007
//	Author		: Alexander Dudin
//	Description : Loophole class inline functions
////////////////////////////////////////////////////////////////////////////

#ifndef SMART_COVER_LOOPHOLE_INLINE_H_INCLUDED
#define SMART_COVER_LOOPHOLE_INLINE_H_INCLUDED

namespace smart_cover {

IC	shared_str const &loophole::id					() const
{
	return		(m_id);
}

IC	float const	&loophole::fov						() const
{
	return		(m_fov);
}

IC	float const	&loophole::danger_fov				() const
{
	return		(m_danger_fov);
}

IC	float const	&loophole::range					() const
{
	return		(m_range);
}

IC	Fvector const &loophole::fov_position			() const
{
	return		(m_fov_position);
}

IC	Fvector	const &loophole::fov_direction			() const
{
	return		(m_fov_direction);
}

IC	Fvector	const &loophole::danger_fov_direction	() const
{
	return		(m_danger_fov_direction);
}

IC	Fvector	const &loophole::enter_direction		() const
{
	return		(m_enter_direction);
}

IC	loophole::ActionList const &loophole::actions	() const
{
	return		(m_actions);
}

IC	bool const &loophole::enterable					() const
{
	return		(m_enterable);
}

IC	void loophole::enterable						(bool value)
{
	m_enterable	= value;
}

IC	bool const &loophole::usable					() const
{
	return		(m_usable);
}

IC	bool loophole::is_action_available				(shared_str const &action_id) const
{
	return		(m_actions.find(action_id) != m_actions.end());
}

IC	bool const &loophole::exitable					() const
{
	return		(m_exitable);
}

IC	void loophole::exitable							(bool value)
{
	m_exitable	= value;
}

} // namespace smart_cover

#endif //SMART_COVER_LOOPHOLE_INLINE_H_INCLUDED