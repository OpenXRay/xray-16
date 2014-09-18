//////////////////////////////////////////////////////////////////////////
// relation_registry_fights.cpp:	реестр для хранения данных об отношении персонажа к 
//									другим персонажам
//////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "relation_registry.h"



//////////////////////////////////////////////////////////////////////////

RELATION_REGISTRY::FIGHT_DATA::FIGHT_DATA			()
{
	attacker = defender = 0xffff;
	total_hit = 0;
	time = 0;
	time_old = 0;
	attack_time = 0;
	defender_to_attacker = ALife::eRelationTypeDummy;
}

//////////////////////////////////////////////////////////////////////////
extern bool	IsGameTypeSingle();
void RELATION_REGISTRY::FightRegister (u16 attacker, u16 defender, ALife::ERelationType defender_to_attacker, float hit_amount)
{
	UpdateFightRegister();

	FIGHT_VECTOR& fights = fight_registry();
	for(FIGHT_VECTOR_IT it = fights.begin(); it != fights.end(); it++)
	{
		FIGHT_DATA& fight_data = *it;
		if(attacker == fight_data.attacker && defender == fight_data.defender)
		{
			fight_data.time_old = fight_data.time;
			fight_data.time = Device.dwTimeGlobal;
			fight_data.total_hit += hit_amount;
			break;
		}
	}

	if(it == fights.end())
	{
		FIGHT_DATA fight_data;
		fight_data.attacker = attacker;
		fight_data.defender = defender;
		fight_data.total_hit = hit_amount;
		fight_data.time = Device.dwTimeGlobal;
		fight_data.defender_to_attacker = defender_to_attacker;
		fights.push_back(fight_data);
	}
}

RELATION_REGISTRY::FIGHT_DATA* RELATION_REGISTRY::FindFight(u16 object_id, bool by_attacker)
{
	FIGHT_VECTOR& fights = fight_registry();
	for(FIGHT_VECTOR_IT it = fights.begin(); it != fights.end(); it++)
	{
		FIGHT_DATA& fight_data = *it;
		u16 id_to_find = by_attacker?fight_data.attacker:fight_data.defender;
		if(object_id == id_to_find)
		{
			return &fight_data;
		}
	}

	return NULL;
}


bool fight_time_pred(RELATION_REGISTRY::FIGHT_DATA& fight_data)
{
	//(c) время которое про драку помнит реестр (иначе считать неактуальным)
	static u32 fight_remember_time	= u32(1000.f * pSettings->r_float(ACTIONS_POINTS_SECT, "fight_remember_time"));	

	u32 time_delta =  Device.dwTimeGlobal - fight_data.time;
	if( time_delta > fight_remember_time)
		return true;

	return false;
}

void RELATION_REGISTRY::UpdateFightRegister ()
{
	FIGHT_VECTOR& fights = fight_registry();
	FIGHT_VECTOR_IT it = std::remove_if(fights.begin(), fights.end(), fight_time_pred);
	fights.erase(it, fights.end());
}