////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_animation_manager.h
//	Created 	: 25.02.2003
//  Modified 	: 19.11.2004
//	Author		: Dmitriy Iassenev
//	Description : Stalker animation manager
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "stalker_animation_pair.h"
#include "stalker_animation_script.h"
#include "ai_monster_space.h"
#include "graph_engine_space.h"

class CMotionDef;
class CBlend;
class CAI_Stalker;
class CWeapon;
class CMissile;
class CPropertyStorage;
class CStalkerAnimationData;

class CStalkerAnimationManager {
public:
	typedef xr_deque<CStalkerAnimationScript>					SCRIPT_ANIMATIONS;
	typedef MonsterSpace::EMovementDirection					EMovementDirection;
	typedef MonsterSpace::EBodyState							EBodyState;
	typedef GraphEngineSpace::_solver_value_type				_value_type;
	typedef GraphEngineSpace::_solver_condition_type			_condition_type;
	typedef CStalkerAnimationPair::BLEND_ID						BLEND_ID;
	typedef BLEND_ID											ANIMATION_ID;

private:
	const CStalkerAnimationData		*m_data_storage;
	SCRIPT_ANIMATIONS				m_script_animations;

private:
	CStalkerAnimationPair			m_global;
	CStalkerAnimationPair			m_head;
	CStalkerAnimationPair			m_torso;
	CStalkerAnimationPair			m_legs;
	CStalkerAnimationPair			m_script;

private:
	u32								m_direction_start;
	EMovementDirection				m_current_direction;
	EMovementDirection				m_target_direction;

private:
	EMovementDirection				m_previous_speed_direction;
	u32								m_change_direction_time;
	mutable int						m_looking_back;

private:
	int								m_crouch_state_config;
	int								m_crouch_state;
	bool							m_no_move_actual;

private:
	CAI_Stalker						*m_object;
	IRenderVisual					*m_visual;
	IKinematicsAnimated				*m_skeleton_animated;

private:
	CWeapon							*m_weapon;
	CMissile						*m_missile;

private:
	bool							m_call_script_callback;
	bool							m_call_global_callback;
	bool							m_start_new_script_animation;

#ifdef USE_HEAD_BONE_PART_FAKE
private:
	u32								m_script_bone_part_mask;
#endif // USE_HEAD_BONE_PART_FAKE

private:
	float							m_previous_speed;
	float							m_target_speed;
	float							m_last_non_zero_speed;
	bool							m_special_danger_move;

public:
	typedef fastdelegate::FastDelegate<MotionID (bool&)>	AnimationSelector;
	typedef fastdelegate::FastDelegate<void ()>				AnimationCallback;
	typedef fastdelegate::FastDelegate<void (CBlend*)>		AnimationModifier;

private:
	AnimationSelector				m_global_selector;
	AnimationCallback				m_global_callback;
	AnimationModifier				m_global_modifier;

public:
	struct callback_params {
		Fmatrix const*				m_rotation;
		CAI_Stalker const*			m_object;
		CBlend const* const*		m_blend;
		bool						m_forward;

							callback_params	()
		{
			invalidate				();
		}

		inline	void		invalidate		()
		{
			m_rotation				= 0;
			m_object				= 0;
			m_blend					= 0;
			m_forward				= false;
		}
	}; // struct callback_params

private:
	callback_params					m_spine_params;
	callback_params					m_shoulder_params;
	callback_params					m_head_params;
	
private:
	IC		bool					strapped				() const;

public:
	IC		bool					standing				() const;

private:
	IC		void					fill_object_info		();
	IC		u32						object_slot				() const;
	IC		EBodyState				body_state				() const;

private:
			bool					need_look_back			() const;

private:
			MotionID				aim_animation			(const u32 &slot, const xr_vector<CAniVector> &animation, const u32 &index) const;
			MotionID				no_object_animation		(const EBodyState &body_state) const;
			MotionID				unknown_object_animation(u32 slot, const EBodyState &body_state) const;
			MotionID				weapon_animation		(u32 slot, const EBodyState &body_state);
			MotionID				missile_animation		(u32 slot, const EBodyState &body_state);

private:
	IC		float					legs_switch_factor		() const;
			void					legs_assign_direction	(float switch_factor, const EMovementDirection &direction);
			void					legs_process_direction	(float yaw);
			MotionID				legs_move_animation		();
			MotionID				legs_no_move_animation	();

private:
			MotionID				global_critical_hit		();

private:
			MotionID				assign_head_animation	();
			MotionID				assign_torso_animation	();
			MotionID				assign_legs_animation	();
	const CStalkerAnimationScript	&assign_script_animation();
			void					clear_unsafe_callbacks	();

public:
			MotionID				assign_global_animation	(bool &animation_movement_controller);
	IC		bool					non_script_need_update	() const;

private:
	IC		bool 					script_callback			() const;
	IC		bool					need_update				() const;
	IC		void 					update_tracks			();

private:
	IC		void					play_script_impl		();
			bool 					play_script				();

private:
	IC		void					play_global_impl		(const MotionID &animation, bool const &animation_movement_controller);
			bool 					play_global				();

private:
	IC		void 					play_head				();
	IC		void 					play_torso				();
			void 					play_legs				();
			void 					update_impl				();

private:
	static	void					global_play_callback	(CBlend *blend);
	static	void					head_play_callback		(CBlend *blend);
	static	void					torso_play_callback		(CBlend *blend);
	static	void					legs_play_callback		(CBlend *blend);
	static	void					script_play_callback	(CBlend *blend);

public:
									CStalkerAnimationManager(CAI_Stalker *object);
	virtual	void					reinit					();
	virtual	void					reload					();
	virtual void					update					();
			void					play_fx					(float power_factor, int fx_index);
			void 					play_delayed_callbacks	();

public:
			void					add_script_animation	(LPCSTR animation, bool hand_usage = false, bool use_movement_controller = false);
			void					add_script_animation	(LPCSTR animation, bool hand_usage, Fvector position, Fvector rotation, bool local_animation);
	IC		void					clear_script_animations	();
	IC		void					pop_script_animation	();
	IC		const SCRIPT_ANIMATIONS	&script_animations		() const;
	IC		void					special_danger_move		(const bool &value);

public:
	IC		CStalkerAnimationPair	&global					();
	IC		CStalkerAnimationPair	&head					();
	IC		CStalkerAnimationPair	&torso					();
	IC		CStalkerAnimationPair	&legs					();
	IC		CStalkerAnimationPair	&script					();
	IC		CAI_Stalker				&object					() const;
	IC		const float				&target_speed			() const;
	IC		const bool				&special_danger_move	() const;

#ifdef DEBUG
private:
			void					add_animation_stats		(const ANIMATION_ID &animation_id, const BLEND_ID *blend_id, bool just_started);
public:
			void					add_animation_stats		();
#endif // DEBUG

public:
	IC		AnimationSelector const	&global_selector		() const;
	IC		void					global_selector			(AnimationSelector const &selector);
	IC		AnimationCallback const	&global_callback		() const;
	IC		void					global_callback			(AnimationCallback const &callback);
	IC		AnimationModifier const	&global_modifier		() const;
	IC		void					global_modifier			(AnimationModifier const &modifier);
	IC	CStalkerAnimationData const &data_storage			() const;

public:
			void					remove_bone_callbacks		();
			void					assign_bone_callbacks		();
			void					assign_bone_blend_callbacks	(bool const& forward_direction);
			bool					forward_blend_callbacks		() const;
			bool					backward_blend_callbacks	() const;
};

#include "stalker_animation_manager_inline.h"
