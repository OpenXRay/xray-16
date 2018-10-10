#include "StdAfx.h"
#include "control_critical_wound.h"
#include "basemonster/base_monster.h"
#include "control_animation_base.h"
#include "control_direction_base.h"
#include "control_movement_base.h"

void CControlCriticalWound::activate()
{
    m_man->capture_pure(this);
    m_man->subscribe(this, ControlCom::eventAnimationEnd);

    m_man->path_stop(this);
    m_man->move_stop(this);
    m_man->dir_stop(this);

    IKinematicsAnimated* skel = smart_cast<IKinematicsAnimated*>(m_object->Visual());

    SControlAnimationData* ctrl_anim = (SControlAnimationData*)m_man->data(this, ControlCom::eControlAnimation);
    VERIFY(ctrl_anim);
    ctrl_anim->global.set_motion(skel->ID_Cycle_Safe(m_data.animation));
    ctrl_anim->global.actual = false;
}

void CControlCriticalWound::on_release()
{
    m_man->release_pure(this);
    m_man->unsubscribe(this, ControlCom::eventAnimationEnd);

    m_object->critical_wounded_state_stop();
}

bool CControlCriticalWound::check_start_conditions()
{
    if (is_active())
        return false;
    if (m_man->is_captured_pure())
        return false;

    return true;
}

void CControlCriticalWound::on_event(ControlCom::EEventType type, ControlCom::IEventData* dat)
{
    switch (type)
    {
    case ControlCom::eventAnimationEnd: m_man->notify(ControlCom::eventCriticalWoundEnd, 0); break;
    }
}
