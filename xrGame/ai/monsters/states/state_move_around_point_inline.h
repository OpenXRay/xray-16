#pragma once

#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>

#define CStateMonsterMoveAroundPointAbstract CStateMonsterMoveAroundPoint<_Object>

TEMPLATE_SPECIALIZATION
void CStateMonsterMoveAroundPointAbstract::initialize()
{
	inherited::initialize();
}

TEMPLATE_SPECIALIZATION
void CStateMonsterMoveAroundPointAbstract::execute()
{
	//if (data.vertex != u32(-1)) object->MoveToTarget(data.point, data.vertex);
	//else object->MoveToTarget(data.point);

	//object->anim().m_tAction				= data.action.action;
	//object->anim().SetSpecParams			(data.action.spec_params);

	//if (data.accelerated) {
	//	object->anim().accel_activate	(EAccelType(data.accel_type));
	//	object->anim().accel_set_braking (data.braking);
	//}

	//if (data.action.sound_type != u32(-1)) {
	//	if (data.action.sound_delay != u32(-1))
	//		object->sound().play(data.action.sound_type, 0,0,data.action.sound_delay);
	//	else 
	//		object->sound().play(data.action.sound_type);
	//}
}

TEMPLATE_SPECIALIZATION
bool CStateMonsterMoveAroundPointAbstract::check_completion()
{	
//	if (data.time_out)

	return false;
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateMonsterMoveAroundPointAbstract
