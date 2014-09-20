#pragma once

#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>

#define CStateMonsterRestSleepAbstract CStateMonsterRestSleep<_Object>

TEMPLATE_SPECIALIZATION
CStateMonsterRestSleepAbstract::CStateMonsterRestSleep(_Object *obj) : inherited(obj)
{
}

TEMPLATE_SPECIALIZATION
CStateMonsterRestSleepAbstract::~CStateMonsterRestSleep	()
{
}

TEMPLATE_SPECIALIZATION
void CStateMonsterRestSleepAbstract::initialize()
{
	inherited::initialize	();
	object->fall_asleep		();
}

TEMPLATE_SPECIALIZATION
void CStateMonsterRestSleepAbstract::execute()
{
	object->set_action				(ACT_SLEEP);
	object->set_state_sound			(MonsterSound::eMonsterSoundIdle);	
}

TEMPLATE_SPECIALIZATION
void CStateMonsterRestSleepAbstract::finalize()
{
	inherited::finalize	();
	object->wake_up		();
}

TEMPLATE_SPECIALIZATION
void CStateMonsterRestSleepAbstract::critical_finalize()
{
	inherited::critical_finalize	();
	object->wake_up					();
}

