#pragma once

#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>

#define CControlledEntityAbstract CControlledEntity<_Object>

TEMPLATE_SPECIALIZATION
void CControlledEntityAbstract::on_reinit()
{
	m_data.m_object = 0;
	m_controller	= 0; 
}

TEMPLATE_SPECIALIZATION
void CControlledEntityAbstract::set_task_follow(const CEntity *e)
{
	m_data.m_object = e;
	m_data.m_task	= eTaskFollow;
}
TEMPLATE_SPECIALIZATION
void CControlledEntityAbstract::set_task_attack(const CEntity *e)
{
	m_data.m_object = e;
	m_data.m_task	= eTaskAttack;
}

TEMPLATE_SPECIALIZATION
void CControlledEntityAbstract::set_under_control(CController *controller)
{
	m_controller		= controller;
	
	saved_id.team_id	= m_object->g_Team	();
	saved_id.squad_id	= m_object->g_Squad	();
	saved_id.group_id	= m_object->g_Group	();

	m_object->ChangeTeam(m_controller->g_Team(), m_controller->g_Squad(), m_controller->g_Group());
}

TEMPLATE_SPECIALIZATION
void CControlledEntityAbstract::free_from_control()
{
	m_object->ChangeTeam			(saved_id.team_id, saved_id.squad_id, saved_id.group_id);
	m_controller					= 0;
}	

TEMPLATE_SPECIALIZATION
void CControlledEntityAbstract::on_die()
{
	if (!is_under_control())			return;

	m_controller->OnFreedFromControl	(m_object);
	m_controller						= 0;
}
TEMPLATE_SPECIALIZATION
void CControlledEntityAbstract::on_destroy()
{
	if (!is_under_control())			return;

	m_object->ChangeTeam				(saved_id.team_id, saved_id.squad_id, saved_id.group_id);

	m_controller->OnFreedFromControl	(m_object);
	m_controller						= 0;
}



#undef TEMPLATE_SPECIALIZATION
#undef CControlledEntityAbstract
