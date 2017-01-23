#pragma once

#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>

#define CStateMonsterCustomActionLookAbstract CStateMonsterCustomActionLook<_Object>

TEMPLATE_SPECIALIZATION
CStateMonsterCustomActionLookAbstract::CStateMonsterCustomActionLook(_Object *obj) : inherited(obj, &data)
{
}

TEMPLATE_SPECIALIZATION
CStateMonsterCustomActionLookAbstract::~CStateMonsterCustomActionLook()
{
}


TEMPLATE_SPECIALIZATION
void CStateMonsterCustomActionLookAbstract::execute()
{
	object->set_action			(data.action);
	object->anim().SetSpecParams(data.spec_params);
	object->dir().face_target	(data.point);

	if (data.sound_type != u32(-1)) {
		if (data.sound_delay != u32(-1))
			object->sound().play(data.sound_type, 0,0,data.sound_delay);
		else 
			object->sound().play(data.sound_type);
	}

}

TEMPLATE_SPECIALIZATION
bool CStateMonsterCustomActionLookAbstract::check_completion()
{	
	if (data.time_out) {
		if (time_state_started + data.time_out < time()) return true;
	}

	return false;
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateMonsterCustomActionLookAbstract
