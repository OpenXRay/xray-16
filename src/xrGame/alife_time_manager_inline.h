////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_time_manager_inline.h
//	Created 	: 05.01.2003
//  Modified 	: 12.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife time manager class inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	void			CALifeTimeManager::set_time_factor		(float time_factor)
{
	m_game_time					= game_time();
	m_start_time				= Device.dwTimeGlobal;
	m_time_factor				= time_factor;
};

IC	ALife::_TIME_ID	CALifeTimeManager::start_game_time		() const
{
	return						m_start_game_time;
}

IC	ALife::_TIME_ID	CALifeTimeManager::game_time			() const
{
	return						(m_game_time + ALife::_TIME_ID(m_time_factor*float(Device.dwTimeGlobal - m_start_time)));
};

IC	float			CALifeTimeManager::time_factor	() const
{
	return						(m_time_factor);
}

IC	float			CALifeTimeManager::normal_time_factor	() const
{
	return						(m_normal_time_factor);
}

IC	void			CALifeTimeManager::change_game_time		(u32 value)
{
	m_game_time		+= value;
}

