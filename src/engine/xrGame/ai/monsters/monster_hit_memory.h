#pragma once
#include "ai_monster_defs.h"

class CBaseMonster;

class CMonsterHitMemory {
	CBaseMonster			*monster;
	TTime					time_memory;

	MONSTER_HIT_VECTOR		m_hits;

public:
						CMonsterHitMemory		();
						~CMonsterHitMemory		();

	void				init_external			(CBaseMonster *M, TTime mem_time);
	void				update					();

	// -----------------------------------------------------
	bool				is_hit					() {return !m_hits.empty();}
	bool				is_hit					(CObject *pO);

	// Lain: added
	int                 get_num_hits            () {return m_hits.size(); }

	void				add_hit					(CObject *who, EHitSide side);
	
	Fvector				get_last_hit_dir		();
	TTime				get_last_hit_time		();
	CObject				*get_last_hit_object	();
	Fvector				get_last_hit_position	();
	
	void				clear					() {m_hits.clear();}

	void				remove_hit_info			(const CObject *obj);

private:
	void				remove_non_actual		();
};
