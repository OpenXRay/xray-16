#include "StdAfx.h"
#include "control_threaten.h"
#include "basemonster/base_monster.h"
#include "control_animation_base.h"
#include "control_direction_base.h"
#include "control_movement_base.h"

void CControlThreaten::reinit() { inherited::reinit(); }
void CControlThreaten::activate()
{
    m_man->capture_pure(this);
    m_man->subscribe(this, ControlCom::eventAnimationSignal);
    m_man->subscribe(this, ControlCom::eventAnimationEnd);

    m_man->path_stop(this);
    m_man->move_stop(this);

    //////////////////////////////////////////////////////////////////////////
    // set direction
    SControlDirectionData* ctrl_dir = (SControlDirectionData*)m_man->data(this, ControlCom::eControlDir);
    VERIFY(ctrl_dir);
    ctrl_dir->heading.target_speed = 1.f;
    ctrl_dir->heading.target_angle = m_man->direction().angle_to_target(m_object->EnemyMan.get_enemy()->Position());

    //////////////////////////////////////////////////////////////////////////
    IKinematicsAnimated* skel = smart_cast<IKinematicsAnimated*>(m_object->Visual());

    SControlAnimationData* ctrl_anim = (SControlAnimationData*)m_man->data(this, ControlCom::eControlAnimation);
    VERIFY(ctrl_anim);
    ctrl_anim->global.set_motion(skel->ID_Cycle_Safe(m_data.animation));
    ctrl_anim->global.actual = false;

    m_man->animation().add_anim_event(
        skel->LL_MotionID(m_data.animation), m_data.time, CControlAnimation::eAnimationCustom);
}

void CControlThreaten::update_schedule()
{
    // update direction (face to enemy here)
    if (m_object->EnemyMan.get_enemy())
    {
        SControlDirectionData* ctrl_dir = (SControlDirectionData*)m_man->data(this, ControlCom::eControlDir);
        VERIFY(ctrl_dir);
        ctrl_dir->heading.target_angle = m_man->direction().angle_to_target(m_object->EnemyMan.get_enemy()->Position());
    }
}

void CControlThreaten::on_release()
{
    m_man->release_pure(this);
    m_man->unsubscribe(this, ControlCom::eventAnimationEnd);
    m_man->unsubscribe(this, ControlCom::eventAnimationSignal);
}

bool CControlThreaten::check_start_conditions()
{
    if (is_active())
        return false;
    if (m_man->is_captured_pure())
        return false;

    const CEntityAlive* enemy = m_object->EnemyMan.get_enemy();
    if (!enemy)
        return false;
    // check if faced enemy
    if (!m_man->direction().is_face_target(enemy, PI_DIV_6))
        return false;

    return true;
}

void CControlThreaten::on_event(ControlCom::EEventType type, ControlCom::IEventData* dat)
{
    switch (type)
    {
    case ControlCom::eventAnimationEnd: m_man->notify(ControlCom::eventThreatenEnd, 0); break;
    case ControlCom::eventAnimationSignal:
    {
        SAnimationSignalEventData* event_data = (SAnimationSignalEventData*)dat;
        if (event_data->event_id == CControlAnimation::eAnimationCustom)
            m_object->on_threaten_execute();
        break;
    }
    }
}
