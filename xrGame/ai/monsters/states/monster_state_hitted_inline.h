#pragma once

#include "monster_state_hitted_hide.h"
#include "monster_state_hitted_moveout.h"
#include "monster_state_home_point_danger.h"

#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>

#define CStateMonsterHittedAbstract CStateMonsterHitted<_Object>

TEMPLATE_SPECIALIZATION
CStateMonsterHittedAbstract::CStateMonsterHitted(_Object *obj) : inherited(obj)
{
	add_state	(eStateHitted_Hide,		xr_new<CStateMonsterHittedHide<_Object> >(obj));
	add_state	(eStateHitted_MoveOut,	xr_new<CStateMonsterHittedMoveOut<_Object> >(obj));
	add_state	(eStateHitted_Home,		xr_new<CStateMonsterDangerMoveToHomePoint<_Object> >(obj));
}

TEMPLATE_SPECIALIZATION
CStateMonsterHittedAbstract::~CStateMonsterHitted()
{
}

TEMPLATE_SPECIALIZATION
void CStateMonsterHittedAbstract::reselect_state()
{
	if (get_state(eStateHitted_Home)->check_start_conditions())	{
		select_state(eStateHitted_Home);
		return;
	}

	if (prev_substate == u32(-1)) {
		select_state(eStateHitted_Hide);
		return;
	}
	
	if (prev_substate == eStateHitted_Hide) { 
		select_state(eStateHitted_MoveOut);
		return;
	}

	select_state(eStateHitted_Hide);
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateMonsterHittedAbstract
