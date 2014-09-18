#pragma once
#include "ai_monster_defs.h"

class CBaseMonster;

class CMonsterCorpseManager {
	CBaseMonster			*monster;
	
	const CEntityAlive	*corpse;

	Fvector				position;
	u32					vertex;
	TTime				time_last_seen;

	bool				forced;

public:
						CMonsterCorpseManager	(); 
						~CMonsterCorpseManager	();
	void				init_external			(CBaseMonster *M);

	void				update					();

	void				force_corpse			(const CEntityAlive *corpse);
	void				unforce_corpse			();

	const CEntityAlive *get_corpse				() {return corpse;}
	const Fvector		&get_corpse_position	() {return position;}
	u32					get_corpse_vertex		() {return vertex;}
	TTime				get_corpse_time_last_seen() {return time_last_seen;}

	void				reinit					();
	void				remove_links			(CObject* O);

};
