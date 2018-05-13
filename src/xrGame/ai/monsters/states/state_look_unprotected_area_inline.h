#pragma once

#include "sound_player.h"

#define TEMPLATE_SPECIALIZATION \
    template <typename _Object\
>

#define CStateMonsterLookToUnprotectedAreaAbstract CStateMonsterLookToUnprotectedArea<_Object>

TEMPLATE_SPECIALIZATION
CStateMonsterLookToUnprotectedAreaAbstract::CStateMonsterLookToUnprotectedArea(_Object* obj) : inherited(obj, &data) {}
TEMPLATE_SPECIALIZATION
CStateMonsterLookToUnprotectedAreaAbstract::~CStateMonsterLookToUnprotectedArea() {}
TEMPLATE_SPECIALIZATION
void CStateMonsterLookToUnprotectedAreaAbstract::initialize()
{
    inherited::initialize();

    Fvector position = this->object->Position();
    position.y += 0.3f;

    const float angle = ai().level_graph().vertex_high_cover_angle(
        this->object->ai_location().level_vertex_id(), PI_DIV_6, std::less<float>());

    Fvector dir;
    dir.set(1.f, 0.f, 0.f);
    dir.setHP(angle + PI, 0.f);
    dir.normalize();

    target_point.mad(this->object->Position(), dir, 1.f);
}

TEMPLATE_SPECIALIZATION
void CStateMonsterLookToUnprotectedAreaAbstract::execute()
{
    this->object->anim().m_tAction = data.action;
    this->object->anim().SetSpecParams(data.spec_params);
    this->object->dir().face_target(target_point);

    if (data.sound_type != u32(-1))
    {
        if (data.sound_delay != u32(-1))
            this->object->sound().play(data.sound_type, 0, 0, data.sound_delay);
        else
            this->object->sound().play(data.sound_type);
    }
}

TEMPLATE_SPECIALIZATION
bool CStateMonsterLookToUnprotectedAreaAbstract::check_completion()
{
    if (data.time_out != 0)
    {
        if (this->time_state_started + data.time_out < Device.dwTimeGlobal)
            return true;
    }
    else if (!this->object->control().direction().is_turning())
        return true;

    return false;
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateMonsterLookToUnprotectedAreaAbstract
