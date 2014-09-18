#pragma once

#include "ai_monster_defs.h"
#include "control_combase.h"
#include "../../movement_manager_space.h"

class CMotionStats;
class CCoverEvaluatorCloseToEnemy;

class CControlPathBuilderBase : public CControl_ComBase {
	typedef CControl_ComBase inherited;

	// -----------------------------------------------------------
	// external setup
	bool						m_try_min_time;
	bool						m_enable;
	bool						m_use_dest_orient;
	Fvector						m_dest_dir;
	MovementManager::EPathType	m_path_type;
	bool						m_extrapolate;
	u32							m_velocity_mask;
	u32							m_desirable_mask;
	bool						m_reset_actuality;
	u32							m_game_graph_target_vertex;

	// -----------------------------------------------------------
    // build path members
	// -----------------------------------------------------------

	class STarget {
		Fvector		_position;
		u32			_node;
	public:
					STarget()
		{
			_position.set( -FLT_MAX, -FLT_MAX, -FLT_MAX );
			_node  =u32(-1);
		}
		void		init		() {
			_position.set	(0.f,0.f,0.f);
			_node			= u32(-1);
		}

		void		set			(const Fvector &pos, u32 vertex) {
			_position.set	(pos);
			_node			= vertex;
		}
		IC	const Fvector	&position	()const				{ return _position; }
		//IC		  Fvector	&position	()					{ return _position; }
		IC	u32				node		()const				{ return _node;		}
		IC	void			set_node	( u32 node_ )		{ _node = node_ ;	}
		IC	void			set_position( const Fvector	&p ){ _position.set(p);	}
	} m_target_set, m_target_found;

	u32			m_time;					// время перестроения пути
	u32			m_last_time_target_set;
	float		m_distance_to_path_end;
	bool		m_failed;
	u32			m_last_time_dir_set;

	bool		m_target_actual;		// устанавливаемый таргет соответствует предыдущему

	struct {
		bool	use_covers;
		float	min_dist;
		float	max_dist;
		float	deviation;
		float	radius;
	} m_cover_info;

	enum {
		eMoveToTarget,
		eRetreatFromTarget,
	} m_target_type;

	CCoverEvaluatorCloseToEnemy	*m_cover_approach;

	// -----------------------------------------------------------

	enum {
		eStatePathValid			= u32(1) << 0,		
		eStateWaitNewPath		= u32(1) << 1,
		eStatePathEnd			= u32(1) << 2,
		eStateNoPath			= u32(1) << 3,
		eStatePathFailed		= u32(1) << 4
	};
	u32							m_state;

	bool						m_path_end;

	// состояние, в котором path_builder работает независимо
	u32							m_time_global_failed_started;
	u32							m_time_path_updated_external;
	
public:
						CControlPathBuilderBase				();
	virtual				~CControlPathBuilderBase			();
	
	// -------------------------------------------------------------------
	// Control Interface
	virtual void		reinit				();
	virtual void		update_frame		();
	virtual void		on_event			(ControlCom::EEventType, ControlCom::IEventData*);	
	virtual void		on_start_control	(ControlCom::EControlType type);
	virtual void		on_stop_control		(ControlCom::EControlType type);

	// -------------------------------------------------------------------

			void		pre_update			();
			

	// -------------------------------------------------------------------
	IC	void	set_try_min_time		(bool new_val) {m_try_min_time		= new_val;}
	IC	void	set_use_dest_orient		(bool new_val) {m_use_dest_orient	= new_val;}
	IC	void	disable_path			() {m_enable = false;}
	IC	void	enable_path				() {m_enable = true;}
	IC	void	extrapolate_path		(bool val) {m_extrapolate = val;}
	IC	void	set_level_path_type		() {m_path_type = MovementManager::ePathTypeLevelPath;}
	IC	void	set_game_path_type		() {m_path_type = MovementManager::ePathTypeGamePath;}
	IC	void	set_patrol_path_type	() {m_path_type = MovementManager::ePathTypePatrolPath;}
	IC	void	set_velocity_mask		(u32 mask) {m_velocity_mask = mask;}
	IC	void	set_desirable_mask		(u32 mask) {m_desirable_mask = mask;}
		void	set_dest_direction		(const Fvector &dir);

	IC	bool	enabled					() {return m_enable;}
	// -------------------------------------------------------------------
	// Set methods
		void		set_target_point		(const Fvector &position, u32 node = u32(-1));
		void		set_target_point		(u32 node);
		void		set_retreat_from_point	(const Fvector &position);

	IC	void		set_rebuild_time		(u32 time);
	IC	void		set_cover_params		(float min, float max, float dev, float radius);
	IC	void		set_use_covers			(bool val = true);
	IC	void		set_distance_to_end		(float dist);

		void		prepare_builder			();
		void		detour_graph_points		(u32 game_graph_vertex_id = u32(-1));
	IC	void		set_generic_parameters	();
	

		bool		is_target_actual		() const {return m_target_actual;}
		Fvector		get_target_found		() {return m_target_found.position();}
		u32			get_target_found_node	() const {return m_target_found.node();}
		Fvector		get_target_set			() {return m_target_set.position();}

		// -------------------------------------------------------------------
		// Services
		void		set_target_accessible	(STarget &target, const Fvector &position);

private:
		// functional
		void		update_path_builder_state	();
		void		update_target_point			();
		void		check_failure				();

		bool		target_point_need_update	();
		void		find_target_point_set		();
		void		find_target_point_failed	();

		void		select_target				();		// выбрать 

		void		set_path_builder_params		();		// set params to control
		
		void		reset						();

		void		travel_point_changed		();
		void		on_path_built				();
		void		on_path_end					();
		void		on_path_updated				();

		// нашли позицию, найти ноду
		void		find_node					();

		bool		global_failed				();
};

#include "control_path_builder_base_inline.h"
