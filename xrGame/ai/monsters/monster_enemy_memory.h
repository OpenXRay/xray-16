#pragma once

#include "ai_monster_defs.h"

class CBaseMonster;

class CMonsterEnemyMemory {
	CBaseMonster	*monster;
	TTime			time_memory;

	ENEMIES_MAP		m_objects;

public:
						CMonsterEnemyMemory		();
						~CMonsterEnemyMemory	();

	void				init_external			(CBaseMonster *M, TTime mem_time);
	void				update					();

	// -----------------------------------------------------
	const CEntityAlive	*get_enemy				();
	SMonsterEnemy		get_enemy_info			();
	u32					get_enemies_count		() {return m_objects.size();}

	const ENEMIES_MAP	&get_memory				() {return m_objects;}

	void				clear					() {m_objects.clear();}
	void				remove_links			(CObject *O);
	
	void				add_enemy				(const CEntityAlive *enemy);
	void				add_enemy				(const CEntityAlive *enemy, const Fvector &pos, u32 vertex, u32 time);

private:

	void				remove_non_actual		();

	ENEMIES_MAP_IT		find_best_enemy			();

};

