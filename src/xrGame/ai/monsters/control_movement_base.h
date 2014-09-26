#pragma once

#include "control_combase.h"
#include "ai_monster_defs.h"

class CControlMovementBase : public CControl_ComBase {
	typedef CControl_ComBase inherited;

	DEFINE_MAP			(u32, SVelocityParam, VELOCITY_MAP, VELOCITY_MAP_IT);
	VELOCITY_MAP		m_velocities;

	float				m_velocity;
	float				m_accel;

public:
	virtual void	load			(LPCSTR section);

	virtual void	reinit			();
	virtual void	update_frame	();

	void			load_velocity	(LPCSTR section, LPCSTR line, u32 param);
	SVelocityParam	&get_velocity	(u32 velocity_id);


			void	stop					();
			void	stop_accel				();
			void	set_velocity			(float val, bool max_acc = false); 
			void	set_accel				(float val) {m_accel = val;}
	
	// services
			float	get_velocity_from_path	();
};