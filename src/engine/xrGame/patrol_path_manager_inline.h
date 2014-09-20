////////////////////////////////////////////////////////////////////////////
//	Module 		: patrol_path_manager_inline.h
//	Created 	: 03.12.2003
//  Modified 	: 03.12.2003
//	Author		: Dmitriy Iassenev
//	Description : Patrol path manager inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CPatrolPathManager::CPatrolPathManager			(CRestrictedObject *object, CGameObject *game_object)
{
	m_object				= object;
	VERIFY					(game_object);
	m_game_object			= game_object;
	m_path					= 0;
	m_path_name				= "";
	m_start_type			= ePatrolStartTypeDummy;
	m_route_type			= ePatrolRouteTypeDummy;
	m_actuality				= true;
	m_failed				= false;
	m_completed				= true;
	m_random				= false;
	m_curr_point_index		= u32(-1);
	m_prev_point_index		= u32(-1);
	m_start_point_index		= u32(-1);
	m_dest_position.set		(flt_max,flt_max,flt_max);
}

IC	bool CPatrolPathManager::actual					() const
{
	return					(m_actuality);
}

IC	bool CPatrolPathManager::failed					() const
{
	return					(m_failed);
}

IC	const CPatrolPath *CPatrolPathManager::get_path	() const
{
	return					(m_path);
}

IC	bool CPatrolPathManager::completed				() const
{
	return					(m_completed);
}

IC	bool CPatrolPathManager::random					() const
{
	return					(m_random);
}

IC	const Fvector &CPatrolPathManager::destination_position	() const
{
	VERIFY					(_valid(m_dest_position));
	return					(m_dest_position);
}

IC	void CPatrolPathManager::set_path				(const CPatrolPath *path, shared_str path_name)
{
	if (m_path == path)
		return;
	m_path					= path;
	m_path_name				= path_name;
	m_actuality				= false;
	m_completed				= false;
	reset					();
}

IC	void CPatrolPathManager::set_start_type			(const EPatrolStartType patrol_start_type)
{
	m_actuality				= m_actuality && (m_start_type == patrol_start_type);
	m_completed				= m_completed && m_actuality;
	m_start_type			= patrol_start_type;
}

IC	void CPatrolPathManager::set_route_type			(const EPatrolRouteType patrol_route_type)
{
	m_actuality				= m_actuality && (m_route_type == patrol_route_type);
	m_completed				= m_completed && m_actuality;
	m_route_type			= patrol_route_type;
}

IC	void CPatrolPathManager::set_random				(bool random)
{
	m_random				= random;
}

IC	void CPatrolPathManager::make_inactual			()
{
	m_actuality				= false;
	m_completed				= false;
}

IC	void CPatrolPathManager::set_path				(shared_str path_name)
{
	set_path				(ai().patrol_paths().path(path_name), path_name);
}

IC	void CPatrolPathManager::set_path				(shared_str path_name, const EPatrolStartType patrol_start_type, const EPatrolRouteType patrol_route_type, bool random)
{
	set_path				(ai().patrol_paths().path(path_name), path_name);
	set_start_type			(patrol_start_type);
	set_route_type			(patrol_route_type);
	set_random				(random);
}
IC	u32	CPatrolPathManager::get_current_point_index() const
{
	return					(m_curr_point_index);
}

IC	CRestrictedObject &CPatrolPathManager::object	() const
{
	VERIFY					(m_object);
	return					(*m_object);
}

IC	CPatrolPathManager::CExtrapolateCallback &CPatrolPathManager::extrapolate_callback	()
{
	return					(m_extrapolate_callback);
}
