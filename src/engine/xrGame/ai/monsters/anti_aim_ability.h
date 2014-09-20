#ifndef ANTI_AIM_ABILITY_H_INCLUDED
#define ANTI_AIM_ABILITY_H_INCLUDED

#include "ai_monster_defs.h"
#include "control_combase.h"

class CBaseMonster;

class anti_aim_ability : public CControl_ComCustom<>
{
public:
	typedef						fastdelegate::FastDelegate< void () >	hit_callback;

private:
	typedef CControl_ComCustom<> inherited;

private:
	CBaseMonster*				m_object;

// settings
	float						m_timeout;
	xr_vector<shared_str>		m_effectors;
	float						m_freeze_time;
	float						m_max_angle;
	float						m_detection_gain_speed;
	float						m_detection_loose_speed;

// state
	TTime						m_last_activated_tick;
	TTime						m_last_detection_tick;
	TTime						m_animation_hit_tick;
	TTime						m_animation_end_tick;
	TTime						m_camera_effector_end_tick;
	u32							m_effector_id;
	float						m_last_angle;
	float						m_detection_level;
	//@bool						m_is_activated;

	hit_callback				m_callback;
	
public:
								anti_aim_ability		(CBaseMonster* object);
								~anti_aim_ability		();

	void						load_from_ini			(CInifile const * ini, pcstr section);
	void						update_schedule			();

	void						set_callback			(hit_callback callback) { m_callback = callback; }
	void						on_monster_death		();
	virtual bool				check_start_condition	(); 
//@	bool						is_active				() const { return m_is_activated; }

private:
	bool						can_detect				();
	void						do_deactivate			();
	void						activate				();
	void						deactivate				();
	bool						check_update_condition	() const;
	float						calculate_angle			() const;
	void						start_camera_effector	();
};


#endif // ANTI_AIM_ABILITY_H_INCLUDED