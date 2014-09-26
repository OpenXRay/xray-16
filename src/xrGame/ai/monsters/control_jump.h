#pragma once
#include "control_combase.h"
#include "../../../Include/xrRender/KinematicsAnimated.h"

struct SControlJumpData : public ControlCom::IComData {
	CObject					*target_object;
 	Fvector					target_position;
	float					force_factor;

	enum EFlags {	
		eEnablePredictPosition		= u32(1) << 0,
		ePrepareSkip				= u32(1) << 1,	// do not use prepare state
		ePrepareInMove				= u32(1) << 2,
		eGlideOnPrepareFailed		= u32(1) << 3,  // if not set then cannot start jump
		eGlidePlayAnimOnce			= u32(1) << 4,
		eGroundSkip					= u32(1) << 5,
		eUseTargetPosition			= u32(1) << 6,
		eDontUseVelocityBounce		= u32(1) << 7,
		eUseAutoAim					= u32(1) << 8,
	};
	
	flags32					flags;

	struct	_prepare{
		MotionID	motion;
	} state_prepare;

	struct	_prepare_in_move{
		MotionID	motion;
		u32			velocity_mask;
	} state_prepare_in_move;


	struct	_glide{
		MotionID	motion;
	} state_glide;

	struct	_ground{
		MotionID	motion;
		u32			velocity_mask;
	} state_ground;
};

class CControlJump : public CControl_ComCustom<SControlJumpData> {
	typedef CControl_ComCustom<SControlJumpData> inherited;

	enum EStateAnimJump {
		eStatePrepare,
		eStatePrepareInMove,
		eStateGlide,
		eStateGround,
		eStateNone
	};


	// loadable parameters
	u32				m_delay_after_jump;
	float			m_jump_factor;
	float			m_trace_ground_range;
	float			m_hit_trace_range;
	float			m_build_line_distance;
	float			m_min_distance;
	float			m_max_distance;
	float			m_max_angle;
	float			m_max_height;
	float			m_auto_aim_factor;
	Fvector			m_jump_start_pos;

	// run-time params
	u32				m_time_next_allowed;
	u32				m_time_started;			// time jump started
	float			m_jump_time;			// physical-counted time of jump
	float			m_blend_speed;			// current anim blend speed
	Fvector			m_target_position;		// save target position for internal needs

	u32				m_last_saved_pos_time;
	Fvector			m_last_saved_pos;

	// state flags
	bool			m_object_hitted;
	bool			m_velocity_bounced;

	// animation
	EStateAnimJump	m_anim_state_prev;
	EStateAnimJump	m_anim_state_current;

	u32				m_last_time_added_impulse;

public:
	virtual void	load					(LPCSTR section);
	virtual void	reinit					();
	virtual bool	check_start_conditions	();
	virtual void	activate				();
	virtual void	on_release				();
	virtual void	on_event				(ControlCom::EEventType, ControlCom::IEventData*);


			float	relative_time			();
			bool	in_auto_aim				();
			float	get_auto_aim_factor		() const { return m_auto_aim_factor; }
			Fvector get_jump_start_pos		() const { return m_jump_start_pos; }
	// process jump
	virtual void	update_frame			();

	// check for distance and angle difference
	virtual	bool	can_jump				(CObject *target);

	bool			can_jump				(Fvector const& target, bool const aggressive_jump);
	bool			jump_intersect_geometry (Fvector const & target, CObject * ignored_object);

	// stop/break jump and all of jumping states
	virtual void	stop					();

	float			get_max_distance		() const { return m_max_distance; }
	float			get_min_distance		() const { return m_min_distance; }

SControlJumpData	&setup_data				() {return m_data;}

	void			remove_links			(CObject* object);

private:	
			void	calculate_jump_time (Fvector const & target, bool check_force_factor);
			// service routines		
			// build path after jump 
			void	grounding			();
			// get target position according to object center point
			Fvector get_target			(CObject *obj);
			// check for hit object
			void	hit_test			();

			// check current jump state		
			bool	is_on_the_ground	();

			// position prediction
			Fvector	predict_position	(CObject *obj, const Fvector &pos);

			void	start_jump			(const Fvector &point);

			// animation control method
			void	select_next_anim_state	();

	IC		bool	is_flag					(SControlJumpData::EFlags flag);
};


IC bool CControlJump::is_flag(SControlJumpData::EFlags flag) 
{
	return (m_data.flags.is(flag) == TRUE);
}
