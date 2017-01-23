////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_movement_params_inline.h
//	Created 	: 23.12.2005
//  Modified 	: 23.12.2005
//	Author		: Dmitriy Iassenev
//	Description : Stalker movement parameters class inline functions
////////////////////////////////////////////////////////////////////////////

#ifndef STALKER_MOVEMENT_PARAMS_INLINE_H_INCLUDED
#define STALKER_MOVEMENT_PARAMS_INLINE_H_INCLUDED

IC	void stalker_movement_params::construct								(stalker_movement_manager_smart_cover* manager)
{
	VERIFY								(!m_manager);
	VERIFY								(manager);
	m_manager							= manager;
}

IC	void stalker_movement_params::desired_position						(Fvector const* position)
{
	if (!position) {
		m_desired_position_impl.set		(flt_max, flt_max, flt_max);
		m_desired_position				= 0;
		return;
	}

	cover_id							("");

	m_desired_position_impl				= *position;
	m_desired_position					= &m_desired_position_impl;
}

IC	Fvector const* stalker_movement_params::desired_position			() const
{
	return								(m_desired_position);
}

IC	void stalker_movement_params::desired_direction						(Fvector const* direction)
{
	if (!direction) {
		m_desired_direction_impl.set	(flt_max, flt_max, flt_max);
		m_desired_direction				= 0;
		return;
	}

	cover_id							("");

	m_desired_direction_impl			= *direction;
	VERIFY								(fsimilar(m_desired_direction_impl.magnitude(), 1.f));
	m_desired_direction					= &m_desired_direction_impl;
}

IC	Fvector const* stalker_movement_params::desired_direction			() const
{
	return								(m_desired_direction);
}

IC	shared_str const& stalker_movement_params::cover_id					() const
{
	return								(m_cover_id);
}

IC	smart_cover::cover const* stalker_movement_params::cover			() const
{
	return								(m_cover);
}

IC	void stalker_movement_params::cover_fire_object						(CGameObject const* object)
{
	m_cover_fire_object					= object;
	if (!object)
		return;

	m_cover_fire_position				= 0;
	m_cover_fire_position_impl.set		(flt_max, flt_max, flt_max);
}

IC	CGameObject const* stalker_movement_params::cover_fire_object		() const
{
	return								(m_cover_fire_object);
}

IC	void stalker_movement_params::cover_fire_position					(Fvector const* position)
{
	if (!position) {
		m_cover_fire_position			= 0;
		m_cover_fire_position_impl.set	(flt_max, flt_max, flt_max);
		return;
	}

	m_cover_fire_object					= 0;
	m_cover_fire_position_impl			= *position;
	m_cover_fire_position				= &m_cover_fire_position_impl;
}

IC	Fvector const* stalker_movement_params::cover_fire_position			() const
{
	return								(m_cover_fire_position);
}

#endif // #ifndef STALKER_MOVEMENT_PARAMS_INLINE_H_INCLUDED