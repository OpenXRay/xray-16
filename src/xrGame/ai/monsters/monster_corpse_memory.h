#pragma once
#include "ai_monster_defs.h"

class CBaseMonster;

class CMonsterCorpseMemory {
	CBaseMonster		*monster;
	TTime				time_memory;

	CORPSE_MAP			m_objects;

public:
						CMonsterCorpseMemory	();
						~CMonsterCorpseMemory	();

	void				init_external			(CBaseMonster *M, TTime mem_time);
	void				update					();

	// -----------------------------------------------------
	const CEntityAlive	*get_corpse				();

	SMonsterCorpse		get_corpse_info			();
	u32					get_corpse_count		() {return m_objects.size();}

	void				clear					() {m_objects.clear();}
	void				remove_links			(CObject *O);

	void				add_corpse				(const CEntityAlive *corpse);
	bool                is_valid_corpse			(const CEntityAlive *corpse);
	
	

private:
	void				remove_non_actual		();

	CORPSE_MAP_IT		find_best_corpse		();

};
