#include "stdafx.h"
#include "ai_monster_squad_manager.h"
#include "ai_monster_squad.h"
#include "../../entity.h"

//////////////////////////////////////////////////////////////////////////
// SQUAD MANAGER Implementation
//////////////////////////////////////////////////////////////////////////
CMonsterSquadManager *g_monster_squad = 0;

CMonsterSquadManager::CMonsterSquadManager()
{
}
CMonsterSquadManager::~CMonsterSquadManager()
{
	for (u32 team_id=0; team_id<team.size();team_id++) {
		for (u32 squad_id=0; squad_id<team[team_id].size(); squad_id++) {
			for (u32 group_id=0; group_id<team[team_id][squad_id].size(); group_id++) {
				xr_delete(team[team_id][squad_id][group_id]);
			}
		}
	}
}

void CMonsterSquadManager::register_member(u8 team_id, u8 squad_id, u8 group_id, CEntity *e)
{
	CMonsterSquad *pSquad;

	// нет team - создать team, squad и group
	if (team_id >= team.size()) { 
		team.resize							(team_id + 1);
		team[team_id].resize				(squad_id + 1);
		team[team_id][squad_id].resize		(group_id + 1);
		
		for (u32 i=0; i<group_id; i++) 	
			team[team_id][squad_id][i]		= 0;

		pSquad								= xr_new<CMonsterSquad>();
		team[team_id][squad_id][group_id]	= pSquad;

	// есть team, нет squad - создать squad
	} else if (squad_id >= team[team_id].size()) { 
		
		team[team_id].resize				(squad_id + 1);
		team[team_id][squad_id].resize		(group_id + 1);
		
		for (u32 i=0; i<group_id; i++) 	
			team[team_id][squad_id][i]		= 0;

		pSquad								= xr_new<CMonsterSquad>();
		team[team_id][squad_id][group_id]	= pSquad;

	// есть team, squad, нет group 
	} else if (group_id >= team[team_id][squad_id].size()) { 
		
		u32 prev_size						= team[team_id][squad_id].size();
		team[team_id][squad_id].resize		(group_id + 1);

		for (u32 i = prev_size; i < group_id; i++)
			team[team_id][squad_id][i]		= 0;
		
		pSquad								= xr_new<CMonsterSquad>();
		team[team_id][squad_id][group_id]	= pSquad;
	} else {
		if (team[team_id][squad_id][group_id] == 0) {
			pSquad								= xr_new<CMonsterSquad>();
			team[team_id][squad_id][group_id]	= pSquad;
		} else {
			// TODO: Verify IT!
			pSquad = team[team_id][squad_id][group_id];
		}
	}

	pSquad->RegisterMember(e);
}

void CMonsterSquadManager::remove_member(u8 team_id, u8 squad_id, u8 group_id, CEntity *e)
{
	get_squad(team_id, squad_id, group_id)->RemoveMember(e);
}

CMonsterSquad *CMonsterSquadManager::get_squad(u8 team_id, u8 squad_id, u8 group_id)
{
	VERIFY((team_id < team.size()) && (squad_id < team[team_id].size()) && (group_id < team[team_id][squad_id].size()));
	return team[team_id][squad_id][group_id];
}

CMonsterSquad *CMonsterSquadManager::get_squad(const CEntity *entity)
{
	return get_squad((u8)entity->g_Team(),(u8)entity->g_Squad(),(u8)entity->g_Group());
}

void CMonsterSquadManager::update(CEntity *entity)
{
	CMonsterSquad	*squad = monster_squad().get_squad(entity);
	if (squad && squad->SquadActive() && (squad->GetLeader() == entity)) {
		squad->UpdateSquadCommands();
	}
}

void CMonsterSquadManager::remove_links(CObject *O)
{
	for (u32 team_id=0; team_id<team.size();team_id++) {
		for (u32 squad_id=0; squad_id<team[team_id].size(); squad_id++) {
			for (u32 group_id=0; group_id<team[team_id][squad_id].size(); group_id++) {
				CMonsterSquad *squad = team[team_id][squad_id][group_id];
				if (squad) squad->remove_links(O);
			}
		}
	}

}
