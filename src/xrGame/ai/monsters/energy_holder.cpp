#include "stdafx.h"
#include "energy_holder.h"
#include "../../gameobject.h"

CEnergyHolder::CEnergyHolder()
{
	m_auto_activate				= false;
	m_auto_deactivate			= false;
	m_enable					= true;
}

CEnergyHolder::~CEnergyHolder()
{
}

void CEnergyHolder::reinit()
{
	m_active					= true;
	m_value						= 1.0f;
	m_time_last_update			= Device.dwTimeGlobal;
}

void CEnergyHolder::reload(LPCSTR section, LPCSTR prefix, LPCSTR suffix) 
{
	string128 line_name;
	
	m_restore_vel				= pSettings->r_float(section, strconcat(sizeof(line_name),line_name,prefix,"Energy_Restore_Velocity",suffix));
	m_decline_vel				= pSettings->r_float(section, strconcat(sizeof(line_name),line_name,prefix,"Energy_Decline_Velocity",suffix));
	m_critical_value			= pSettings->r_float(section, strconcat(sizeof(line_name),line_name,prefix,"Energy_Critical_Value",suffix)); 
	m_activate_value			= pSettings->r_float(section, strconcat(sizeof(line_name),line_name,prefix,"Energy_Activate_Value",suffix));
	m_aggressive_restore_vel	= pSettings->r_float(section, strconcat(sizeof(line_name),line_name,prefix,"Energy_Aggressive_Restore_Velocity",suffix));

	m_aggressive				= false;
}

void CEnergyHolder::activate()
{
	if (!is_active()) on_activate();		
	m_active = true;
}	

void CEnergyHolder::deactivate()
{
	if (is_active()) on_deactivate();	
	m_active = false;
}

void CEnergyHolder::schedule_update()
{
	if (!m_enable) return;
	
	// Обновить значение энергии
	u32		cur_time	= Device.dwTimeGlobal;
	float	dt			= float(cur_time - m_time_last_update) / 1000.f;

	if (!is_active()) 
		m_value += m_aggressive ? m_aggressive_restore_vel * dt : m_restore_vel * dt;
	else 
		m_value -= m_decline_vel * dt;

	clamp(m_value, 0.f, 1.f);

	// сохранить время последнего обновления
	m_time_last_update = cur_time;

	// проверка на автоматическое включение/выключение поля
	if (is_active() && should_deactivate() && m_auto_deactivate)	deactivate	();
	if (!is_active() && can_activate() && m_auto_activate)			activate	();
}

void CEnergyHolder::enable()
{
	m_enable			= true;
	m_time_last_update	= Device.dwTimeGlobal;
}
