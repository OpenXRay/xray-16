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
void CStateChimeraHuntingMoveToCoverAbstract::initialize()
{
	inherited::initialize();
	
	
}

TEMPLATE_SPECIALIZATION
bool CStateChimeraHuntingMoveToCoverAbstract::check_completion()
{
	return false;
}

TEMPLATE_SPECIALIZATION
void CStateChimeraHuntingMoveToCoverAbstract::execute()
{
	
}


#undef TEMPLATE_SPECIALIZATION
#undef CStateChimeraHuntingMoveToCoverAbstract
