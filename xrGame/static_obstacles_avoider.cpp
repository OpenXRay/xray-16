////////////////////////////////////////////////////////////////////////////
//	Module 		: obstacles_avoider.cpp
//	Created 	: 16.05.2007
//  Modified 	: 16.05.2007
//	Author		: Dmitriy Iassenev
//	Description : obstacles avoider
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "static_obstacles_avoider.h"
#include "ai_space.h"
#include "moving_objects.h"
#include "stalker_movement_manager_obstacles.h"
#include "ai/stalker/ai_stalker.h"
#include "moving_object.h"

IC	const CAI_Stalker &static_obstacles_avoider::object	() const
{
	return							(movement_manager().object());
}

void static_obstacles_avoider::query						(const Fvector &start_position, const Fvector &dest_position)
{
	ai().moving_objects().query_action_static(
		object().get_moving_object(),
		start_position,
		dest_position
	);

	m_current_iteration.swap		(object().get_moving_object()->static_query());
}

void static_obstacles_avoider::query						()
{
	ai().moving_objects().query_action_static(
		object().get_moving_object()		
	);

	m_current_iteration.swap		(object().get_moving_object()->static_query());
}

bool static_obstacles_avoider::new_obstacles_found			() const
{
	if (m_current_iteration.obstacles().empty())
		return						(false);

	if (m_current_iteration.area().empty())
		return						(false);

	if (m_current_iteration != m_last_iteration)
		return						(true);

	if (!*m_failed_to_build_path)
		return						(false);

	return							(true);
}

bool static_obstacles_avoider::refresh_objects				()
{
	float							update_radius = m_active_query.refresh_radius();
	if (!m_active_query.objects_changed(object().Position(),update_radius))
		return						(true);

	m_temp_query.copy				(m_active_query);

	if (!m_active_query.refresh_objects())
		return						(true);

	if (!m_movement_manager->can_build_restricted_path(m_active_query)) {
		m_active_query.swap			(m_temp_query);
		
		if (m_inactive_query == m_active_query)
			m_inactive_query.refresh_objects	();

		return						(false);
	}

	m_inactive_query.update_objects	(object().Position(),update_radius);

	m_need_path_to_rebuild			= true;
	return							(true);
}

bool static_obstacles_avoider::process_query				(const bool &change_path_state)
{
	if (!new_obstacles_found())
		return						(change_path_state ? refresh_objects() : true);

	bool							active_query_actual = change_path_state ? (m_active_query == m_inactive_query) : true;
	if (
			!m_inactive_query.merge(
				object().Position(),
				change_path_state ? m_inactive_query.refresh_radius() : 0.f,
				m_current_iteration
			)
		)
	{
		if (active_query_actual)
			m_active_query.copy		(m_inactive_query);

		return						(change_path_state ? refresh_objects() : true);
	}

	if (!m_movement_manager->can_build_restricted_path(m_inactive_query))
		return						(change_path_state ? refresh_objects() : false);

	m_active_query.copy				(m_inactive_query);
	m_need_path_to_rebuild			= true;
	return							(true);
}

void static_obstacles_avoider::on_before_query				()
{
	m_current_iteration.swap		(m_last_iteration);
	m_current_iteration.clear		();
	m_need_path_to_rebuild			= false;
}

void static_obstacles_avoider::update						()
{
	on_before_query					();
	query							();
	process_query					(true);
}

void static_obstacles_avoider::remove_links					(CObject *object)
{
	m_current_iteration.remove_links(object);
	m_inactive_query.remove_links	(object);
	m_active_query.remove_links		(object);
	m_last_iteration.remove_links	(object);
}