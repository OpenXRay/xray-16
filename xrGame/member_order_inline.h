////////////////////////////////////////////////////////////////////////////
//	Module 		: member_order_inline.h
//	Created 	: 26.05.2004
//  Modified 	: 26.05.2004
//	Author		: Dmitriy Iassenev
//	Description : Member order inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CMemberOrder::CMemberOrder					(CAI_Stalker *object) :
	m_object		(object),
	m_initialized	(true)
{
	VERIFY					(m_object);
	m_cover					= 0;
	m_probability			= 1.f;
	m_processed				= false;
	m_selected_enemy		= 0;
	m_detour				= false;
}

IC	bool CMemberOrder::initialized				() const
{
	return			(m_initialized);
}

IC	CAI_Stalker &CMemberOrder::object			() const
{
	VERIFY			(m_object);
	return			(*m_object);
}

IC	float CMemberOrder::probability				() const
{
	return			(m_probability);
}

IC	void CMemberOrder::probability				(float probability)
{
	m_probability	= probability;
}

IC	xr_vector<u32> &CMemberOrder::enemies		()
{
	return			(m_enemies);
}

IC	bool CMemberOrder::processed				() const
{
	return			(m_processed);
}

IC	void CMemberOrder::processed				(bool processed)
{
	m_processed		= processed;
}

IC	u32	 CMemberOrder::selected_enemy			() const
{
	return			(m_selected_enemy);
}

IC	void CMemberOrder::selected_enemy			(u32 selected_enemy)
{
	m_selected_enemy = selected_enemy;
}

IC	void CMemberOrder::cover					(const CCoverPoint *object_cover) const
{
	m_cover			= object_cover;
}

IC	const CCoverPoint *CMemberOrder::cover		() const
{
	return			(m_cover);
}

IC	CMemberOrder::CMemberDeathReaction &CMemberOrder::member_death_reaction	()
{
	return			(m_member_death_reaction);
}

IC	CMemberOrder::CGrenadeReaction &CMemberOrder::grenade_reaction			()
{
	return			(m_grenade_reaction);
}

IC	bool CMemberOrder::detour					() const
{
	return			(m_detour);
}

IC	void CMemberOrder::detour					(const bool &value)
{
	m_detour		= value;
}
