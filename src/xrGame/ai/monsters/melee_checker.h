#pragma once
class CBaseMonster;
class CEntityAlive;

#define HIT_STACK_SIZE	2

class CMeleeChecker {
private:
	collide::rq_results	r_res;

private:
	CBaseMonster	*m_object;

	// ltx parameters
	float			m_min_attack_distance;
	float			m_max_attack_distance;
	float			m_as_min_dist;			
	float			m_as_step;				

	bool			m_hit_stack[HIT_STACK_SIZE];

	float			m_current_min_distance;	
	
public:
			void	init_external			(CBaseMonster *obj) {m_object = obj;}
	IC		void	load					(LPCSTR section);
			
	// инициализировано состо€ние атаки
	IC		void	init_attack				();
			void	on_hit_attempt			(bool hit_success);

			// ѕолучить рассто€ние от fire_bone до врага
			// ¬ыполнить RayQuery от fire_bone в enemy.center
			float	distance_to_enemy		(const CEntityAlive *enemy);

	IC		float	get_min_distance		();
	IC		float	get_max_distance		();

			bool	can_start_melee			(const CEntityAlive *enemy);
			bool	should_stop_melee		(const CEntityAlive *enemy);

#ifdef DEBUG
	IC		float	dbg_as_min_dist			(){return m_as_min_dist;}
	IC		float	dbg_as_step				(){return m_as_step;}
#endif

};

#include "melee_checker_inline.h"