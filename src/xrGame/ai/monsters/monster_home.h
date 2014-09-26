#pragma once

class CBaseMonster;
class CPatrolPath;

class CMonsterHome 
{
	CBaseMonster		*m_object;
	const CPatrolPath	*m_path;
	u32					m_level_vertex_id;
	float				m_radius_min;
	float				m_radius_max;
	float				m_radius_middle;
	u32					min_move_dist;
	u32					max_move_dist;

	bool				m_aggressive;

public:
	CMonsterHome	            (CBaseMonster *obj) : m_object(obj) {}

	void	load				(LPCSTR line);
	void	setup				(LPCSTR path_name, float min_radius, float max_radius, bool aggressive = false, float middle_radius = 0.f);
	void	setup				(u32 lv_ID, float min_radius, float max_radius, bool aggressive = false, float middle_radius = 0.f);
	void	remove_home			();
	void	set_move_dists		(u32 min_dist, u32 max_dist);

	u32		get_place_in_min_home();
	u32		get_place_in_mid_home();
	u32		get_place_in_max_home_to_direction(Fvector to_direction);
	u32		get_place_in_max_home();
	u32		get_place			();
	u32		get_place_in_cover	();
	bool	at_home				();
	bool	at_home				(const Fvector &pos);
	bool	at_home				(const Fvector &pos, float radius);
	bool	at_min_home			(const Fvector &pos);
	bool	at_mid_home			(const Fvector &pos);
	Fvector	get_home_point		();
	float	get_min_radius		() { return m_radius_min;    }
	float	get_mid_radius		() { return m_radius_middle; }
	float	get_max_radius		() { return m_radius_max;    };
IC	bool	has_home			() { return (m_path != 0)&&(m_level_vertex_id != u32(-1));   }
IC	bool	is_aggressive		() { return m_aggressive;    }
};