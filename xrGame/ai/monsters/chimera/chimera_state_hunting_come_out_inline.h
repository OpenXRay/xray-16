#pragma once

#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>

#define CStateChimeraHuntingMoveToCoverAbstract CStateChimeraHuntingMoveToCover<_Object>

TEMPLATE_SPECIALIZATION
CStateChimeraHuntingMoveToCoverAbstract::CStateChimeraHuntingMoveToCover(_Object *obj) : inherited(obj)
{
}


TEMPLATE_SPECIALIZATION
bool CStateChimeraHuntingMoveToCoverAbstract::check_start_conditions()
{
	return true;
}

TEMPLATE_SPECIALIZATION
bool CStateChimeraHuntingMoveToCoverAbstract::check_completion()
{
	return false;
}

TEMPLATE_SPECIALIZATION
void CStateChimeraHuntingMoveToCoverAbstract::reselect_state()
{
	if (prev_substate == u32(-1))					select_state(eStateMoveToCover);
	else if (prev_substate == eStateMoveToCover)	select_state(eStateComeOut);
	else											select_state(eStateMoveToCover);
}


#undef TEMPLATE_SPECIALIZATION
#undef CStateChimeraHuntingMoveToCoverAbstract
