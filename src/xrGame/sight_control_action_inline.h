////////////////////////////////////////////////////////////////////////////
//	Module 		: sight_control_action_inline.h
//	Created 	: 05.04.2004
//  Modified 	: 05.04.2004
//	Author		: Dmitriy Iassenev
//	Description : Sight control action inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CSightControlAction::CSightControlAction	(float weight, u32 inertia_time, const CSightAction &sight_action) :
	m_weight			(weight),
	m_inertia_time		(inertia_time)
{
	(CSightAction&)*this	= sight_action;
}

IC	float CSightControlAction::weight			() const
{
	return				(m_weight);
}

IC	bool CSightControlAction::completed			() const
{
	return				(Device.dwTimeGlobal - m_start_time >= m_inertia_time);
}

IC	bool CSightControlAction::use_torso_look	() const
{
	return				(m_torso_look);
}

IC	const SightManager::ESightType &CSightControlAction::sight_type	() const
{
	return				(m_sight_type);
}

IC	const Fvector &CSightControlAction::vector3d	() const
{
	return				(m_vector3d);
}

IC	const CGameObject &CSightControlAction::object	() const
{
	VERIFY				(m_object_to_look);
	return				(*m_object_to_look);
}
