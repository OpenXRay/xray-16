#pragma once

#include "../control_animation_base.h"
#include "../ai_monster_defs.h"
#include "../../../../Include/xrRender/KinematicsAnimated.h"

class CController;

class CControllerAnimation : public CControlAnimationBase {
	typedef CControlAnimationBase inherited;

	CController	*m_controller;

public:
	enum ELegsActionType {
		eLegsTypeBase			= u32(1) << 15,

		// -----------------------------------------

		eLegsTypeStand			= eLegsTypeBase << 1,
		eLegsTypeSteal			= eLegsTypeBase << 2,
		eLegsTypeStealMotion	= eLegsTypeBase << 3,
		eLegsTypeWalk			= eLegsTypeBase << 4,
		eLegsTypeRun			= eLegsTypeBase << 5,

		// -----------------------------------------

		eLegsStand				= eLegsTypeStand | 1,
		eLegsSteal				= eLegsTypeSteal | 1,
		eLegsRun				= eLegsTypeRun	 | 1,
		eLegsWalk				= eLegsTypeWalk  | 1,
		eLegsBackRun			= eLegsTypeRun | 2,
		eLegsRunFwdLeft			= eLegsTypeRun | 3,
		eLegsRunFwdRight		= eLegsTypeRun | 4,
		eLegsRunBkwdLeft		= eLegsTypeRun | 5,
		eLegsRunBkwdRight		= eLegsTypeRun | 6,
		eLegsStealFwd			= eLegsTypeStealMotion | 1,
		eLegsStealBkwd			= eLegsTypeStealMotion | 2,
		eLegsStealFwdLeft		= eLegsTypeStealMotion | 3,
		eLegsStealFwdRight		= eLegsTypeStealMotion | 4,
		eLegsStealBkwdLeft		= eLegsTypeStealMotion | 5,
		eLegsStealBkwdRight		= eLegsTypeStealMotion | 6,


		eLegsStandDamaged		= eLegsTypeStand	| 2,
		eLegsRunDamaged			= eLegsTypeRun		| 7,
		eLegsWalkDamaged		= eLegsTypeWalk		| 2,
		eLegsBackRunDamaged		= eLegsTypeRun		| 8,
		eLegsRunStrafeLeftDamaged	= eLegsTypeRun	| 9,
		eLegsRunStrafeRightDamaged	= eLegsTypeRun	| 10,

		eLegsUndefined = u32(-1),
	};

	enum ETorsoActionType {
		eTorsoIdle,
		eTorsoSteal,
		eTorsoPsyAttack,
		eTorsoRun
	};

private:
	ELegsActionType		m_current_legs_action;
	ETorsoActionType	m_current_torso_action;

	DEFINE_MAP			(ELegsActionType,	MotionID, LEGS_MOTION_MAP,	LEGS_MOTION_MAP_IT);
	DEFINE_MAP			(ETorsoActionType,	MotionID, TORSO_MOTION_MAP, TORSO_MOTION_MAP_IT);
	
	LEGS_MOTION_MAP		m_legs;
	TORSO_MOTION_MAP	m_torso;

	struct SPathRotations{
		float			angle;
		ELegsActionType	legs_motion;
	};
	
	DEFINE_VECTOR		(SPathRotations,	PATH_ROTATIONS_VEC, PATH_ROTATIONS_VEC_IT);
	DEFINE_MAP			(ELegsActionType,	PATH_ROTATIONS_VEC, PATH_ROTATIONS_MAP, PATH_ROTATIONS_MAP_IT);
	PATH_ROTATIONS_MAP	m_path_rotations;

	bool				m_wait_torso_anim_end;

public:	
	virtual void		reinit				();
	virtual void		on_event			(ControlCom::EEventType, ControlCom::IEventData*);	
	virtual void		on_start_control	(ControlCom::EControlType type);
	virtual void		on_stop_control		(ControlCom::EControlType type);
	virtual void		update_frame		();

			// load
			void		load				();
			void		add_path_rotation	(ELegsActionType action, float angle, ELegsActionType type);
			
			void		set_body_state		(ETorsoActionType, ELegsActionType);

			void		set_path_params		();
			void		on_switch_controller	();
private:
			void		select_velocity		();
			void		set_path_direction	();

			void		select_torso_animation	();
			void		select_legs_animation	();

			SPathRotations	get_path_rotation	(float cur_yaw);

			bool		is_moving				();
			
};
