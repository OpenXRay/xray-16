#pragma once

template<typename _Object>
class CScanningAbility {
	_Object		*object;

	// external members
	float		critical_value;
	float		scan_radius;
	float		velocity_threshold;
	float		decrease_value;
	float		scan_trace_time_freq;

	ref_sound	sound_scan;

	// internal members
	enum EScanState {
		eStateDisabled,
		eStateNotActive,
		eStateScanning
	} state;

	float		scan_value;

	SPPInfo		m_effector_info;
	float		m_effector_time;
	float		m_effector_time_attack;
	float		m_effector_time_release;

	u32			time_last_trace;

	bool		m_this_scan;

public:
			void	init_external			(_Object *obj) {object = obj;}
			void	on_destroy				();

			void	load					(LPCSTR section);
	virtual	void	reinit					();
		
			void	schedule_update			();
			void	frame_update			(u32 dt);

			void	enable					();
			void	disable					();

	virtual void	on_scan_success			() {}
	virtual void	on_scanning				() {}

private:
			float	get_velocity			(CObject *obj);

#ifdef DEBUG
public:
			float	get_scan_value			(){return scan_value;}
#endif

};

#include "scanning_ability_inline.h"