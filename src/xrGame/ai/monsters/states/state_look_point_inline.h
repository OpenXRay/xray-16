#pragma once

#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>

#define CStateMonsterLookToPointAbstract CStateMonsterLookToPoint<_Object>

TEMPLATE_SPECIALIZATION
CStateMonsterLookToPointAbstract::CStateMonsterLookToPoint(_Object *obj) : inherited(obj, &data)
{
}

TEMPLATE_SPECIALIZATION
CStateMonsterLookToPointAbstract::~CStateMonsterLookToPoint()
{
}

TEMPLATE_SPECIALIZATION
void CStateMonsterLookToPointAbstract::initialize()
{
	inherited::initialize();
}

TEMPLATE_SPECIALIZATION
void CStateMonsterLookToPointAbstract::execute()
{
	object->anim().m_tAction				= data.action.action;
	object->anim().SetSpecParams			(data.action.spec_params);
	object->dir().face_target				(data.point, data.face_delay);

	if (data.action.sound_type != u32(-1)) {
		if (data.action.sound_delay != u32(-1))
			object->sound().play(data.action.sound_type, 0,0,data.action.sound_delay);
		else 
			object->sound().play(data.action.sound_type);
	}

}

TEMPLATE_SPECIALIZATION
bool CStateMonsterLookToPointAbstract::check_completion()
{	
	if (data.action.time_out != 0) {
		if (time_state_started + data.action.time_out < Device.dwTimeGlobal) return true;
	} else if (!object->control().direction().is_turning()) return true;
	return false;
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateMonsterLookToPointAbstract