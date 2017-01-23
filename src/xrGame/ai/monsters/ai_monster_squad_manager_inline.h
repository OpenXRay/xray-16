#pragma once

IC	CMonsterSquadManager &monster_squad()
{
	if (!g_monster_squad)
		g_monster_squad = new CMonsterSquadManager();
	return (*g_monster_squad);
}

