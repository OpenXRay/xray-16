#pragma once

#define TEMPLATE_SPECIALIZATION \
    template <typename _Object>

#define CStateMonsterRestSleepAbstract CStateMonsterRestSleep<_Object>

TEMPLATE_SPECIALIZATION
CStateMonsterRestSleepAbstract::CStateMonsterRestSleep(_Object* obj) : inherited(obj) {}
TEMPLATE_SPECIALIZATION
CStateMonsterRestSleepAbstract::~CStateMonsterRestSleep() {}
TEMPLATE_SPECIALIZATION
void CStateMonsterRestSleepAbstract::initialize()
{
    inherited::initialize();
    this->object->fall_asleep();
}

TEMPLATE_SPECIALIZATION
void CStateMonsterRestSleepAbstract::execute()
{
    this->object->set_action(ACT_SLEEP);
    this->object->set_state_sound(MonsterSound::eMonsterSoundIdle);
}

TEMPLATE_SPECIALIZATION
void CStateMonsterRestSleepAbstract::finalize()
{
    inherited::finalize();
    this->object->wake_up();
}

TEMPLATE_SPECIALIZATION
void CStateMonsterRestSleepAbstract::critical_finalize()
{
    inherited::critical_finalize();
    this->object->wake_up();
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateMonsterRestSleepAbstract
