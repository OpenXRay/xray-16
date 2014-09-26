////////////////////////////////////////////////////////////////////////////
//	Module 		: movement_manager_inline.h
//	Created 	: 02.10.2001
//  Modified 	: 12.11.2003
//	Author		: Dmitriy Iassenev
//	Description : Movement manager inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	bool CMovementManager::actual() const
{
	return					(m_path_actuality);
}

IC	void CMovementManager::set_path_type(EPathType path_type)
{
	m_path_actuality		= m_path_actuality && (m_path_type == path_type);
	m_path_type				= path_type;
}

IC	bool CMovementManager::enabled() const
{
	return					(m_enabled);
}

IC	float CMovementManager::speed() const
{
	return					(m_speed);
}

IC	bool CMovementManager::path_completed() const
{
	return					((m_path_state == ePathStatePathCompleted) && actual());
}

IC	const float &CMovementManager::old_desirable_speed	() const
{
	return					(m_old_desirable_speed);
}

IC	void CMovementManager::set_desirable_speed		(float speed)
{
	m_old_desirable_speed	= speed;
}

IC	void CMovementManager::set_body_orientation(const CBoneRotation &orientation)
{
	m_body					= orientation;
}

IC	const CMovementManager::CBoneRotation &CMovementManager::body_orientation() const
{
	return					(m_body);
}

template <typename T>
IC	bool CMovementManager::accessible			(T position_or_vertex_id, float radius) const
{
	return					(restrictions().accessible(position_or_vertex_id,radius));
}

IC	void CMovementManager::extrapolate_path		(bool value)
{
	m_path_actuality		= m_path_actuality && (value == m_extrapolate_path);
	m_extrapolate_path		= value;
}

IC	bool CMovementManager::extrapolate_path		() const
{
	return					(m_extrapolate_path);
}

IC	void CMovementManager::set_build_path_at_once()
{
	m_build_at_once			= true;
}

IC	CMovementManager::CGameVertexParams	*CMovementManager::base_game_params() const
{
	return					(m_base_game_selector);
}

IC	CMovementManager::CBaseParameters	*CMovementManager::base_level_params() const
{
	return					(m_base_level_selector);
}

IC	CMovementManager::CGameLocationSelector		&CMovementManager::game_selector	() const
{
	VERIFY					(m_game_location_selector);
	return					(*m_game_location_selector);
}

IC	CMovementManager::CGamePathManager			&CMovementManager::game_path		() const
{
	VERIFY					(m_game_path_manager);
	return					(*m_game_path_manager);
}

IC	CMovementManager::CLevelPathManager			&CMovementManager::level_path		() const
{
	VERIFY					(m_level_path_manager);
	return					(*m_level_path_manager);
}

IC	CDetailPathManager		&CMovementManager::detail	() const
{
	VERIFY					(m_detail_path_manager);
	return					(*m_detail_path_manager);
}

IC	CPatrolPathManager		&CMovementManager::patrol		() const
{
	VERIFY					(m_patrol_path_manager);
	return					(*m_patrol_path_manager);
}

IC	CRestrictedObject &CMovementManager::restrictions					() const
{
	VERIFY					(m_restricted_object);
	return					(*m_restricted_object);
}

IC	CLocationManager &CMovementManager::locations						() const
{
	VERIFY					(m_location_manager);
	return					(*m_location_manager);
}

IC	CCustomMonster &CMovementManager::object							() const
{
	VERIFY					(m_object);
	return					(*m_object);
}

IC	CLevelPathBuilder &CMovementManager::level_path_builder				() const
{
	VERIFY					(m_level_path_builder);
	return					(*m_level_path_builder);
}

IC	CDetailPathBuilder &CMovementManager::detail_path_builder			() const
{
	VERIFY					(m_detail_path_builder);
	return					(*m_detail_path_builder);
}

IC	bool CMovementManager::wait_for_distributed_computation				() const
{
	return					(m_wait_for_distributed_computation);
}
