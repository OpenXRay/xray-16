#pragma once

#include "chimera_state_hunting_move_to_cover.h"
#include "chimera_state_hunting_come_out.h"

#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>

#define CStateChimeraHuntingAbstract CStateChimeraHunting<_Object>

TEMPLATE_SPECIALIZATION
CStateChimeraHuntingAbstract::CStateChimeraHunting(_Object *obj) : inherited(obj)
{
	add_state(eStateMoveToCover,	xr_new<CStateChimeraHuntingMoveToCover<_Object> >	(obj));
	add_state(eStateComeOut,		xr_new<CStateChimeraHuntingComeOut<_Object> >		(obj));
}


TEMPLATE_SPECIALIZATION
bool CStateChimeraHuntingAbstract::check_start_conditions()
{
	return true;
}

TEMPLATE_SPECIALIZATION
bool CStateChimeraHuntingAbstract::check_completion()
{
	return false;
}

TEMPLATE_SPECIALIZATION
void CStateChimeraHuntingAbstract::reselect_state()
{
	if (prev_substate == u32(-1))					select_state(eStateMoveToCover);
	else if (prev_substate == eStateMoveToCover)	select_state(eStateComeOut);
	else											select_state(eStateMoveToCover);
}


#undef TEMPLATE_SPECIALIZATION
#undef CStateChimeraHuntingAbstract
