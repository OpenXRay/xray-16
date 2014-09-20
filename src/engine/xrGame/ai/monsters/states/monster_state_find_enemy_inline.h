#pragma once

#include "monster_state_find_enemy_run.h"
#include "monster_state_find_enemy_angry.h"
#include "monster_state_find_enemy_walk.h"
#include "monster_state_find_enemy_look.h"

#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>

#define CStateMonsterFindEnemyAbstract CStateMonsterFindEnemy<_Object>

TEMPLATE_SPECIALIZATION
CStateMonsterFindEnemyAbstract::CStateMonsterFindEnemy(_Object *obj) : inherited(obj)
{
	add_state	(eStateFindEnemy_Run,			xr_new<CStateMonsterFindEnemyRun<_Object> >			(obj));
	add_state	(eStateFindEnemy_LookAround,	xr_new<CStateMonsterFindEnemyLook<_Object> >		(obj));
	add_state	(eStateFindEnemy_Angry,			xr_new<CStateMonsterFindEnemyAngry<_Object> >		(obj));
	add_state	(eStateFindEnemy_WalkAround,	xr_new<CStateMonsterFindEnemyWalkAround<_Object> >	(obj));
}

TEMPLATE_SPECIALIZATION
CStateMonsterFindEnemyAbstract::~CStateMonsterFindEnemy()
{
}

TEMPLATE_SPECIALIZATION
void CStateMonsterFindEnemyAbstract::reselect_state()
{
	if (prev_substate == u32(-1)) {
		select_state(eStateFindEnemy_Run);
		return;
	}
	
	switch (prev_substate)	{
		case eStateFindEnemy_Run:			select_state(eStateFindEnemy_LookAround);	break;
		case eStateFindEnemy_LookAround:	select_state(eStateFindEnemy_Angry);		break;
		case eStateFindEnemy_Angry:			select_state(eStateFindEnemy_WalkAround);	break;
		case eStateFindEnemy_WalkAround:	select_state(eStateFindEnemy_WalkAround);	break;
	}
}

