////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_switch_manager_inline.h
//	Created 	: 25.12.2002
//  Modified 	: 12.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife Simulator switch manager inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CALifeSwitchManager::CALifeSwitchManager		(xrServer *server, LPCSTR section) :
	inherited		(server,section)
{
	m_switch_distance	= pSettings->r_float(section,"switch_distance");
	m_switch_factor		= pSettings->r_float(section,"switch_factor");
	set_switch_distance	(m_switch_distance);
	seed				(u32(CPU::QPC() & 0xffffffff));
}

IC	float CALifeSwitchManager::online_distance		() const
{
	return				(m_online_distance);
}

IC	float CALifeSwitchManager::offline_distance		() const
{
	return				(m_offline_distance);
}

IC	float CALifeSwitchManager::switch_distance		() const
{
	return				(m_switch_distance);
}

IC	void CALifeSwitchManager::set_switch_distance	(float switch_distance)
{
	m_switch_distance	= switch_distance;
	m_online_distance	= m_switch_distance*(1.f - m_switch_factor);
	m_offline_distance	= m_switch_distance*(1.f + m_switch_factor);
}

IC	void CALifeSwitchManager::set_switch_factor		(float switch_factor)
{
	m_switch_factor		= switch_factor;
	set_switch_distance	(switch_distance());
}
