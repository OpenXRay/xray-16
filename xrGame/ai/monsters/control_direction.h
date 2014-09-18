#pragma once

#include "control_combase.h"

struct SControlDirectionData : public ControlCom::IComData {
	struct {
		float	target_angle;
		float	target_speed;
	} heading, pitch;

	bool		linear_dependency;
};

struct SRotationEventData : public ControlCom::IEventData {

	enum RotType {
		eHeading		= u32(1) << 0,
		ePitch			= u32(1) << 1,
	};
	u8 angle;
};

class CControlDirection : public CControl_ComPure<SControlDirectionData> {
	typedef CControl_ComPure<SControlDirectionData> inherited;

	struct {
		float	current_angle;
		float	current_speed;			// current speed
		float	current_acc;

		void	init	() {
			current_angle	= 0;
			current_speed	= 0;
			current_acc		= flt_max;
		}
	} m_heading, m_pitch;

public:

	virtual void	reinit				();
	virtual void	update_frame		();
	
	// services
			bool	is_face_target		(const Fvector &position,	float eps_angle);
			bool	is_face_target		(const CObject *obj,		float eps_angle);

			bool	is_from_right		(const Fvector &position);
			bool	is_from_right		(float yaw);

			bool	is_turning			(float eps_angle = EPS);

			void	get_heading			(float &current, float &target);
			float	get_heading_current	();	

			float	angle_to_target		(const Fvector &position);	
private:				
			void	pitch_correction	();
};
