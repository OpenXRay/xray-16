#ifndef MONSTER_AURA_H_INCLUDED
#define MONSTER_AURA_H_INCLUDED

class CBaseMonster;

class monster_aura
{
private:
	CBaseMonster*		m_object;

	char				m_name[64];
	float				m_linear_factor;
	float				m_quadratic_factor;
	float				m_max_power;
	float				m_max_distance;
	bool				m_enable_for_dead;
	
	float				m_pp_highest_at;
	pcstr				m_pp_effector_name;
	u32					m_pp_index;

	ref_sound			m_sound;
	ref_sound			m_detect_sound;
	float				m_detect_snd_time;

	bool				m_enabled;
public:
						monster_aura				(CBaseMonster* object, pcstr name);
						~monster_aura				();

	void				load_from_ini				(CInifile const * ini, pcstr section, bool enable_for_dead_default = false);
	float				calculate					() const;
	void				update_schedule				();
	void				play_detector_sound			();
	void				on_monster_death			();

private:
	bool				check_work_condition		() const;
	void				remove_pp_effector			();
	float				override_if_debug			(pcstr var_name, float value) const;
	float   xr_stdcall	get_post_process_factor		() const;
};


#endif // MONSTER_AURA_H_INCLUDED