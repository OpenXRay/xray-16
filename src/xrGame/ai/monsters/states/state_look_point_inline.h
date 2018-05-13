#pragma once

#define TEMPLATE_SPECIALIZATION \
    template <typename _Object\
>

#define CStateMonsterLookToPointAbstract CStateMonsterLookToPoint<_Object>

TEMPLATE_SPECIALIZATION
CStateMonsterLookToPointAbstract::CStateMonsterLookToPoint(_Object* obj) : inherited(obj, &data) {}
TEMPLATE_SPECIALIZATION
CStateMonsterLookToPointAbstract::~CStateMonsterLookToPoint() {}
TEMPLATE_SPECIALIZATION
void CStateMonsterLookToPointAbstract::initialize() { inherited::initialize(); }
TEMPLATE_SPECIALIZATION
void CStateMonsterLookToPointAbstract::execute()
{
    this->object->anim().m_tAction = data.action.action;
    this->object->anim().SetSpecParams(data.action.spec_params);
    this->object->dir().face_target(data.point, data.face_delay);

    if (data.action.sound_type != u32(-1))
    {
        if (data.action.sound_delay != u32(-1))
            this->object->sound().play(data.action.sound_type, 0, 0, data.action.sound_delay);
        else
            this->object->sound().play(data.action.sound_type);
    }
}

TEMPLATE_SPECIALIZATION
bool CStateMonsterLookToPointAbstract::check_completion()
{
    if (data.action.time_out != 0)
    {
        if (this->time_state_started + data.action.time_out < Device.dwTimeGlobal)
            return true;
    }
    else if (!this->object->control().direction().is_turning())
        return true;
    return false;
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateMonsterLookToPointAbstract
