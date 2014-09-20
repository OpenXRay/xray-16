////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_movement_manager_obstacles.h
//	Created 	: 27.12.2003
//	Modified	: 13.02.2008
//	Author		: Dmitriy Iassenev
//	Description : stalker movement manager class with obstacles avoiding
////////////////////////////////////////////////////////////////////////////

#ifndef STALKER_MOVEMENT_MANAGER_OBSTACLES_H_INCLUDED
#define STALKER_MOVEMENT_MANAGER_OBSTACLES_H_INCLUDED

#include "stalker_movement_manager_base.h"
#include "static_obstacles_avoider.h"
#include "dynamic_obstacles_avoider.h"

namespace doors {
	class actor;
} // namespace doors

class CRestrictedObjectObstacle;

class stalker_movement_manager_obstacles : public stalker_movement_manager_base {
private:
	typedef stalker_movement_manager_base					inherited;

public:
						stalker_movement_manager_obstacles	(CAI_Stalker *object);
	virtual				~stalker_movement_manager_obstacles	();
	virtual	void				move_along_path				(CPHMovementControl *movement_control, Fvector &dest_position, float time_delta);
	virtual	void				remove_links				(CObject *object);
	virtual	void				Load						( LPCSTR section );
			float				is_going_through			( Fmatrix const& matrix, Fvector const& vector, float max_distance ) const;
			void				on_death					( );

private:
	using CMovementManager::move_along_path;

private:
	typedef obstacles_query::AREA							AREA;

private:
	virtual	CRestrictedObject	*create_restricted_object	();

public:
	IC	CRestrictedObjectObstacle	&restricted_object		() const;

private:
	IC		void				remove_query_objects		(const Fvector &position, const float &radius);
			bool				apply_border				(const obstacles_query &query);
			void				remove_border				(const obstacles_query &query);
			void				move_along_path_no_changes	();
			void				rebuild_path				();
			void				process_query_impl			();
			void				process_query				(CPHMovementControl *movement_control, Fvector &dest_position, float time_delta);
			void				move_along_path_impl		(CPHMovementControl *movement_control, Fvector &dest_position, float time_delta);

public:
			bool				can_build_restricted_path	(const obstacles_query &query);

private:
	typedef DetailPathManager::STravelPathPoint				STravelPathPoint;

private:
			void				save_current_state			();
			void				restore_current_state		();
			bool				simulate_path_navigation	();

public:
	virtual	void				build_level_path			();
	virtual	const float			&prediction_speed			() const;
#ifdef DEBUG
	inline	doors::actor const&	get_doors_actor				() const { return *m_doors_actor; }
#endif // #ifdef DEBUG

#ifdef DEBUG
private:
	typedef CDetailPathManager::STravelPoint				STravelPoint;
	typedef xr_vector<STravelPoint>							KEY_POINTS;

private:
	KEY_POINTS					m_detail_key_points;
#endif // #ifdef DEBUG

private:
	bool						m_saved_state;
	xr_vector<u32>				m_level_path;
	u32							m_detail_current_index;
	xr_vector<STravelPathPoint>	m_detail_path;
	u32							m_detail_last_patrol_point;
	obstacles_query				m_saved_current_iteration;
	const CGameObject*			m_last_query_object;
	doors::actor*				m_doors_actor;

private:
	CRestrictedObjectObstacle	*m_restricted_object;
	static_obstacles_avoider	m_static_obstacles;
	dynamic_obstacles_avoider	m_dynamic_obstacles;
	u32							m_last_dest_vertex_id;
	u32							m_last_fail_time;
	xr_vector<u32>				m_temp_path;
	bool						m_failed_to_build_path;
}; // class stalker_movement_manager_obstacles

#include "stalker_movement_manager_obstacles_inline.h"

#endif // #ifndef STALKER_MOVEMENT_MANAGER_OBSTACLES_H_INCLUDED