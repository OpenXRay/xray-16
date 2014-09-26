#pragma once

#include "psy_dog_state_psy_attack_hide.h"

#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>

#define CStatePsyDogPsyAttackAbstract CStatePsyDogPsyAttack<_Object>

TEMPLATE_SPECIALIZATION
CStatePsyDogPsyAttackAbstract::CStatePsyDogPsyAttack(_Object *obj) : inherited(obj)
{
	add_state	(eStateAttack_HideInCover,	xr_new<CStatePsyDogHide<_Object> >	(obj));
}
TEMPLATE_SPECIALIZATION
void CStatePsyDogPsyAttackAbstract::reselect_state()
{
	select_state(eStateAttack_HideInCover);
}


#undef TEMPLATE_SPECIALIZATION
#undef CStatePsyDogPsyAttackAbstract