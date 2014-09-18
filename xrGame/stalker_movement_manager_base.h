////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_movement_manager_base.h
//	Created 	: 27.12.2003
//	Modified	: 13.02.2008
//	Author		: Dmitriy Iassenev
//	Description : stalker movement manager base class
////////////////////////////////////////////////////////////////////////////

#ifndef STALKER_MOVEMENT_MANAGER_BASE_H_INCLUDED
#define STALKER_MOVEMENT_MANAGER_BASE_H_INCLUDED

#include "movement_manager.h"
#include "ai_monster_space.h"
#include "stalker_movement_params.h"

#ifdef DEBUG
#	include "detail_path_manager.h"
#	include "ai/stalker/ai_stalker.h"
#else // #ifdef DEBUG
#	include "detail_path_manager_space.h"
#endif // #ifdef DEBUG

using namespace MonsterSpace;

class CAI_Stalker;
class CStalkerVelocityCollection;
class CGameObject;

class stalker_movement_manager_base : public CMovementManager {
private:
	typedef CMovementManager						inherited;

public:
	typedef MonsterSpace::SBoneRotation				SBoneRotation;
	typedef MonsterSpace::EMovementType				EMovementType;
	typedef MonsterSpace::EBodyState				EBodyState;
	typedef MonsterSpace::EMentalState				EMentalState;
	typedef DetailPathManager::EDetailPathType		EDetailPathType;

public:
	using inherited::speed;

public:
					stalker_movement_manager_base	(CAI_Stalker *object);
	virtual			~stalker_movement_manager_base	();
	virtual	void	Load							(LPCSTR section);
	virtual	void	reinit							();
	virtual	void	reload							(LPCSTR section);
	virtual	void	update							(u32 time_delta);
	virtual void	on_travel_point_change			(const u32 &previous_travel_point_index);
	virtual	void	on_restrictions_change			();
	virtual	void	set_level_dest_vertex			(u32 const& level_vertex_id);
	virtual	void	remove_links					(CObject *object);
			void	initialize						();
	IC		float	path_direction_angle			();
	IC		bool	turn_in_place					() const;

	IC		void	set_head_orientation			(const MonsterSpace::SBoneRotation &orientation);
			void	set_desired_position			(const Fvector *desired_position);
	IC		void	set_desired_direction			(const Fvector *desired_direction);
	IC		void	set_body_state					(EBodyState body_state);
	IC		void	set_movement_type				(EMovementType movement_type);
	IC		void	set_mental_state				(EMentalState mental_state);
	IC		void	set_path_type					(EPathType path_type);
	IC		void	set_detail_path_type			(EDetailPathType detail_path_type);
			void	set_nearest_accessible_position	();
			void	set_nearest_accessible_position	(Fvector desired_position, u32 level_vertex_id);
			float	speed							(const EMovementDirection &movement_direction);
			void	setup_speed_from_animation		(const float &speed);

public:
	IC		SBoneRotation const&	head_orientation		() const;
	IC		Fvector const*			desired_position		() const;
	IC		Fvector const*			desired_direction		() const;
	IC		EBodyState const&		body_state				() const;
	IC		EBodyState const&		target_body_state		() const;
	IC		EMovementType const&	movement_type			() const;
	IC		EMentalState const&		mental_state			() const;
	IC		EMentalState const&		target_mental_state		() const;
	IC		EPathType const&		path_type				() const;
	IC		EDetailPathType const&	detail_path_type		() const;
	IC		bool					use_desired_position	() const;
	IC		bool					use_desired_direction	() const;
	IC		EMovementType const&	target_movement_type	() const;
	IC		CAI_Stalker&			object					() const;
	IC stalker_movement_params const& current_params		() const;
	IC stalker_movement_params&		target_params			();

public:
	virtual void	on_build_path					();
			void	update_object_on_the_way		(const CGameObject *object, const float &distance);
			bool	is_object_on_the_way			(const CGameObject *object, const float &distance);
			void	force_update					(const bool &force_update);
	IC		void	danger_head_speed				(const float &speed);

private:
	IC		void	setup_head_speed				(stalker_movement_params& movement_params);
	IC		void	add_velocity					(int mask, float linear, float compute_angular, float angular);
	IC		void	add_velocity					(int mask, float linear, float compute_angular);
	IC		void	setup_body_orientation			();
			void	init_velocity_masks				();
			void	setup_movement_params			(stalker_movement_params& movement_params);
			bool	script_control					();
			void	setup_velocities				(stalker_movement_params& movement_params);
			void	parse_velocity_mask				(stalker_movement_params& movement_params);
			void	check_for_bad_path				(stalker_movement_params& movement_params);

protected:
			void	update							(stalker_movement_params& movement_params);

private:
	typedef CStalkerVelocityCollection	velocities_type;

private:
	velocities_type const*	m_velocities;
	float					m_danger_head_speed;
	u32						m_last_turn_index;
	CAI_Stalker*			m_object;

protected:
	stalker_movement_params	m_current;
	stalker_movement_params	m_target;

public:
	SBoneRotation			m_head;

private:
	const CGameObject*		m_last_query_object;
	Fvector					m_last_query_position;
	Fvector					m_last_query_object_position;
	float					m_last_query_distance;
	bool					m_last_query_result;
	bool					m_force_update;
}; // class stalker_movement_manager_base

#include "stalker_movement_manager_base_inline.h"

#endif // #ifndef STALKER_MOVEMENT_MANAGER_BASE_H_INCLUDED