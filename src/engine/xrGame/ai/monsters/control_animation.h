#pragma once

#include "control_combase.h"
//#include "../../../Include/xrRender/KinematicsAnimated.h"
#include "../../../Include/xrRender/RenderVisual.h"
#include "../../../Include/xrRender/KinematicsAnimated.h"

struct SAnimationPart {
	CBlend			*blend;
	bool			actual;
	u32				time_started;
	
	void	init	() {
		motion.invalidate	();
		blend				= 0;
		actual				= true;
		time_started		= 0;
	}

	void			set_motion (MotionID const& m);
	MotionID const&	get_motion () const { return motion; }

private:
	MotionID		motion;
};

struct SControlAnimationData : public ControlCom::IComData {
	float				_speed;
	IC void				set_speed	(float v)		{_speed=v; VERIFY2(_abs(_speed)<1000,"SControlAnimationData::set_speed too big");};
	IC float			get_speed	()				{return _speed;};
	SAnimationPart		global;
	SAnimationPart		legs;
	SAnimationPart		torso;
};

struct SAnimationSignalEventData : public ControlCom::IEventData {
	MotionID	motion;
	float		time_perc;
	u32			event_id;
	IC			SAnimationSignalEventData(MotionID m, float perc, u32 id) : time_perc(perc), event_id(id), motion(m) {}
};


class CControlAnimation : public CControl_ComPure<SControlAnimationData> {
	typedef CControl_ComPure<SControlAnimationData> inherited;

	IKinematicsAnimated		*m_skeleton_animated;
	
	// animation events
	struct SAnimationEvent{
		float	time_perc;
		u32		event_id;
		bool	handled;
	};

	DEFINE_VECTOR			(SAnimationEvent, ANIMATION_EVENT_VEC, ANIMATION_EVENT_VEC_IT);
	DEFINE_MAP				(MotionID, ANIMATION_EVENT_VEC, ANIMATION_EVENT_MAP, ANIMATION_EVENT_MAP_IT);
	ANIMATION_EVENT_MAP		m_anim_events;

	bool					m_freeze;
	float					m_saved_global_speed;
	float					m_saved_legs_speed;
	float					m_saved_torso_speed;

public:
	
	bool					m_global_animation_end;
	bool					m_legs_animation_end;
	bool					m_torso_animation_end;

public:
	virtual void	reinit					();
	virtual void	update_frame			();
	virtual	void	reset_data				();
				
			void	add_anim_event			(MotionID, float, u32);

			CBlend	*current_blend			() {return m_data.global.blend;}

			void	restart					();
			
			void	freeze					();
			void	unfreeze				();

		// Services
		IC	float	motion_time				(MotionID motion_id, IRenderVisual *visual);


private:
	void	play				();
	void	play_part			(SAnimationPart &part, PlayCallback callback);
	void	check_events		(SAnimationPart &part);
	void	check_callbacks		();

	void	restart				(SAnimationPart &part, PlayCallback callback);

public:
	enum EAnimationEventType {
		eAnimationHit		= u32(0),
		eAnimationCustom
	};
};

// get motion time, when just MotionID available
IC float CControlAnimation::motion_time(MotionID motion_id, IRenderVisual *visual)
{
	IKinematicsAnimated	*skeleton_animated	= smart_cast<IKinematicsAnimated*>(visual);
	VERIFY				(skeleton_animated);
	CMotionDef			*motion_def			= skeleton_animated->LL_GetMotionDef(motion_id);
	VERIFY				(motion_def);
	CMotion*			motion				= skeleton_animated->LL_GetRootMotion(motion_id);
	VERIFY				(motion);
	return				(motion->GetLength() / motion_def->Speed());
}

