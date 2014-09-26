////////////////////////////////////////////////////////////////////////////
//	Module 		: base_monster_misc.cpp
//	Created 	: 26.05.2003
//  Modified 	: 26.05.2003
//	Author		: Serge Zhem
//	Description : Miscellanious functions for all the biting monsters
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "base_monster.h"
#include "../../../entitycondition.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////
// Входные воздействия 
// Зрение, слух, вероятность победы, выгодность противника
void CBaseMonster::UpdateMemory()
{
	// Обновить память
	EnemyMemory.update			();
	SoundMemory.UpdateHearing	();	
	CorpseMemory.update			();
	HitMemory.update			();
	
	// обновить менеджеры врагов и трупов
	EnemyMan.update				();	
	CorpseMan.update			();
		
	// remove hit info from objects that are corpses
	

	hear_dangerous_sound = hear_interesting_sound = false;
	SoundElem se;
	
	if (SoundMemory.IsRememberSound()) {
		SoundMemory.GetSound(se,hear_dangerous_sound);
		hear_interesting_sound = !hear_dangerous_sound;
	}

	// Setup is own additional flags
	m_bDamaged		= ((conditions().GetHealth() < db().m_fDamagedThreshold) ? true : false);
	
	m_bAggressive	=	hear_dangerous_sound || (EnemyMan.get_enemies_count() > 0) || 
						HitMemory.is_hit();

}


