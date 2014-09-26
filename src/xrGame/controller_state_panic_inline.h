
#pragma once

#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>

#define CStateControllerPanicAbstract CStateControllerPanic<_Object>

TEMPLATE_SPECIALIZATION
CStateControllerPanicAbstract::CStateControllerPanic(_Object *obj) : inherited(obj)
{
	//state_ptr state_run;
	//add_state	(eStateRun,	state_run);
}

TEMPLATE_SPECIALIZATION
CStateControllerPanicAbstract::~CStateControllerPanic()
{
}

TEMPLATE_SPECIALIZATION
void CStateControllerPanicAbstract::reselect_state()
{
}
