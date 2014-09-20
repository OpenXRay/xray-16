////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_movement_manager_smart_cover.h
//	Created 	: 27.12.2003
//	Modified	: 13.02.2008
//	Author		: Dmitriy Iassenev
//	Description : stalker movement manager class with smart covers
////////////////////////////////////////////////////////////////////////////

#ifndef STALKER_MOVEMENT_MANAGER_SMART_COVER_H_INCLUDED
#define STALKER_MOVEMENT_MANAGER_SMART_COVER_H_INCLUDED

#include "stalker_movement_manager_obstacles.h"
#include "../include/xrrender/animation_motion.h"

class CCoverPoint;
class CPropertyStorage;
class CBlend;

namespace smart_cover {
	class cover;
	class animation_planner;
	class target_selector;
	class animation_selector;

	namespace transitions {
		class action;
		class animation_action;
	} // namespace transitions
} // namespace smart_cover

template <typename _return_type>
class CScriptCallbackEx;

class CEntityAlive;

class stalker_movement_manager_smart_cover : public stalker_movement_manager_obstacles {
private:
	typedef stalker_movement_manager_obstacles			inherited;
	typedef smart_cover::animation_selector				animation_selector_type;
	typedef smart_cover::animation_planner				animation_planner_type;
	typedef smart_cover::cover							cover_type;
	typedef smart_cover::loophole						loophole_type;
	typedef smart_cover::transitions::action			transition_action;
	typedef smart_cover::transitions::animation_action	animation_action;
	typedef smart_cover::target_selector				target_selector_type;

public:
				 stalker_movement_manager_smart_cover	(CAI_Stalker *object);
	virtual		 ~stalker_movement_manager_smart_cover	();
	virtual	void				reinit					();
	virtual	void				update					(u32 time_delta);
	virtual	void				on_frame				(CPHMovementControl *movement_control, Fvector &dest_position);
	virtual	void				remove_links			(CObject *object);
			void		cleanup_after_animation_selector();
			bool				in_smart_cover			() const;
	inline	bool				check_can_kill_enemy	() const;
	inline	void				check_can_kill_enemy	(bool value);

public:
			bool				enemy_in_fov			() const;
			bool				in_fov					(shared_str const &cover_id, shared_str const &loophole_id, Fvector const &position) const;
			bool				in_range				(shared_str const &cover_id, shared_str const &loophole_id, Fvector const &position) const;
			bool				in_current_loophole_fov	(Fvector const &position) const;
			bool			in_current_loophole_range	(Fvector const &position) const;
	IC	float const&apply_loophole_direction_distance	() const;
	IC	void		apply_loophole_direction_distance	(float const &value);

// forced to be public
public:
		transition_action const	&current_transition		();
			bool				exit_transition			();
			void				go_next_loophole		();
			void	start_non_animated_loophole_change	();
			void	stop_non_animated_loophole_change	();
			Fvector				position_to_cover_from	() const;

public:
	IC	target_selector_type&	target_selector			();
			void				target_selector			(CScriptCallbackEx<void> const &callback);
			void				target_idle				();
			void				target_lookout			();
			void				target_fire				();
			void				target_fire_no_lookout	();
			void				target_default			(bool const& value);

public:
	IC	animation_selector_type&animation_selector		() const;
	IC		void				property_storage		(CPropertyStorage *storage);
	IC		bool	entering_smart_cover_with_animation	() const;
			bool				default_behaviour		() const;
	IC		void				combat_behaviour		(bool value);
	IC		bool				combat_behaviour		() const;

public:
			float const&		idle_min_time			() const;
			void				idle_min_time			(float const &value);
			float const&		idle_max_time			() const;
			void				idle_max_time			(float const &value);
			float const&		lookout_min_time		() const;
			void				lookout_min_time		(float const &value);
			float const&		lookout_max_time		() const;
			void				lookout_max_time		(float const &value);

private:
			bool				target_approached		(float const &distance) const;
			void				enter_smart_cover		(u32 const& time_delta);
			void				on_smart_cover_enter	();
			void				on_smart_cover_exit		();

// fov range checks
private:
			bool				in_min_acceptable_range		(shared_str const& cover_id, shared_str const& loophole_id, Fvector const& position, float const& min_range) const;

// loopholes stuff
private:
		loophole_type const& nearest_enterable_loophole	();
		loophole_type const&	loophole				(cover_type const& cover, shared_str const &loophole_id) const;
			void				bind_global_selector	();
			void				unbind_global_selector	();

private:
			shared_str const&	next_loophole_id		();

private:
	IC		void			non_animated_loophole_change(bool const &value);

private:
	IC	animation_action const	&current_transition_animation	() const;

private:
	typedef	xr_vector<shared_str>						LoopholePath;

private:
			void				loophole_path			(smart_cover::cover const &cover, shared_str const &source, shared_str const &target, LoopholePath &path) const;
			void				build_enter_path		();
			void				build_exit_path			();
			void				build_exit_path_to_cover();
			void				actualize_path			();
			void				try_actualize_path		();
		transition_action const &action					(
									smart_cover::cover const &cover,
									shared_str const &loophole_id0,
									shared_str const &loophole_id1
								) const;
		transition_action const &nearest_action			(
									smart_cover::cover const &cover,
									shared_str const &loophole_id0,
									shared_str const &loophole_id1,
									Fvector const& position,
									Fvector& result_position,
									u32& result_vertex_id,
									EBodyState* target_body_state
								) const;
			bool				fill_enemy_position		(Fvector &position) const;
			bool				update_script_cover		();
			float				exit_path_weight		(
									u32 const &source_node,
									Fvector const &source_position,
									u32 const &target_node,
									Fvector const &target_position
								) const;
			float				enter_path				(
									LoopholePath* result,
									Fvector const& position,
									u32 const level_vertex_id,
									smart_cover::cover const& cover,
									shared_str const& target_loophole_id
								);

private:
			void				setup_movement_params	();
			void			non_animated_change_loophole();
			void				enter_smart_cover		();
			void				reach_enter_location	(u32 const& time_delta);

private:
			MotionID xr_stdcall	select_animation		(bool& animation_movement_controller);
			void xr_stdcall		on_animation_end		();
			void xr_stdcall		modify_animation		(CBlend* blend);
			bool				test_pick				(Fvector source, Fvector destination) const;

private:
	LoopholePath				m_path;
	LoopholePath				m_temp_loophole_path;
	target_selector_type*		m_target_selector;
	animation_selector_type*	m_animation_selector;
	transition_action const*	m_current_transition;
	animation_action const*		m_current_transition_animation;
	CPropertyStorage*			m_property_storage;
	float						m_apply_loophole_direction_distance;
	MotionID					m_enter_animation;
	shared_str					m_enter_cover_id;
	shared_str					m_enter_loophole_id;
	mutable collide::rq_results	m_ray_query_storage;
	bool						m_entering_smart_cover_with_animation;
	bool						m_non_animated_loophole_change;
	bool						m_default_behaviour;
	bool						m_check_can_kill_enemy;
	bool						m_combat_behaviour;
}; // class stalker_movement_manager_smart_cover

#include "stalker_movement_manager_smart_cover_inline.h"

#endif // #ifndef STALKER_MOVEMENT_MANAGER_SMART_COVER_H_INCLUDED