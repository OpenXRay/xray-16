#pragma once

#include "../../actor_input_handler.h"

class CControlledActor : public CActorInputHandler {
	typedef CActorInputHandler inherited;
	
	Fvector m_target_point;
	
	bool	m_turned_yaw;
	bool	m_turned_pitch;

	bool	m_lock_run;
	u32		m_lock_run_started;
	u32		m_lock_run_period;

	bool	m_need_turn;

public:
	virtual void	reinit				();
	virtual	float	mouse_scale_factor	(){return flt_max;}
	virtual void	release				();
	virtual void	install				(CActor *);
	virtual void	install				();
	virtual bool	authorized			(int cmd);

			void	look_point			(const Fvector &point);
			bool	is_turning			();
			bool    is_installed		();

			void	frame_update		();
			bool	is_controlling		() {return m_actor != 0;}

			void	dont_need_turn		(){m_need_turn = false;}

private:
			void	reset				();
			void	update_turn			();
};	

