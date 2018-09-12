#include "StdAfx.h"
#include "control_melee_jump.h"
#include "basemonster/base_monster.h"
#include "control_manager.h"

#define CHECK_YAW 165 * PI / 180
#define ROTATION_JUMP_DELAY_MIN 500
#define ROTATION_JUMP_DELAY_MAX 1000
#define MAX_DISTANCE_TO_ENEMY 4.f

void CControlMeleeJump::reinit()
{
    inherited::reinit();

    m_time_next_melee_jump = 0;
}

bool CControlMeleeJump::check_start_conditions()
{
    if (is_active())
        return false;
    if (m_man->is_captured_pure())
        return false;

    if (!m_object->EnemyMan.get_enemy())
        return false;
    if (m_time_next_melee_jump > Device.dwTimeGlobal)
        return false;

    Fvector enemy_position;
    enemy_position.set(m_object->EnemyMan.get_enemy()->Position());
    if (m_man->direction().is_face_target(enemy_position, CHECK_YAW))
        return false;
    if (enemy_position.distance_to(m_object->Position()) > MAX_DISTANCE_TO_ENEMY)
        return false;

    return true;
}

void CControlMeleeJump::activate()
{
    m_man->capture_pure(this);
    m_man->subscribe(this, ControlCom::eventAnimationEnd);

    // disable path builder and movement
    m_man->path_stop(this);
    m_man->move_stop(this);

    // get	direction to enemy
    Fvector dir_to_enemy;
    dir_to_enemy.set(m_object->Direction());
    dir_to_enemy.sub(m_object->EnemyMan.get_enemy()->Position(), m_object->Position());
    dir_to_enemy.normalize();

    float target_yaw = angle_normalize(-dir_to_enemy.getH());
    MotionID motion = ((m_man->direction().is_from_right(target_yaw)) ? m_data.anim_rs : m_data.anim_ls);
    float anim_time = m_man->animation().motion_time(motion, m_object->Visual());

    // set yaw
    SControlDirectionData* ctrl_data_dir = (SControlDirectionData*)m_man->data(this, ControlCom::eControlDir);
    VERIFY(ctrl_data_dir);
    ctrl_data_dir->heading.target_angle = target_yaw;

    // set angular speed
    float cur_yaw;
    m_man->direction().get_heading(cur_yaw, target_yaw);
    ctrl_data_dir->heading.target_speed = angle_difference(cur_yaw, target_yaw) / anim_time;
    ctrl_data_dir->linear_dependency = false;
    VERIFY(!fis_zero(ctrl_data_dir->heading.target_speed));

    // set animation
    SControlAnimationData* ctrl_data = (SControlAnimationData*)m_man->data(this, ControlCom::eControlAnimation);
    VERIFY(ctrl_data);

    ctrl_data->global.set_motion(motion);
    ctrl_data->global.actual = false;
}

void CControlMeleeJump::on_release()
{
    SControlDirectionData* ctrl_data_dir = (SControlDirectionData*)m_man->data(this, ControlCom::eControlDir);
    VERIFY(ctrl_data_dir);
    ctrl_data_dir->linear_dependency = true;

    m_man->release_pure(this);
    m_man->unsubscribe(this, ControlCom::eventAnimationEnd);

    m_time_next_melee_jump = Device.dwTimeGlobal + Random.randI(ROTATION_JUMP_DELAY_MIN, ROTATION_JUMP_DELAY_MAX);
}

void CControlMeleeJump::on_event(ControlCom::EEventType type, ControlCom::IEventData* dat)
{
    switch (type)
    {
    case ControlCom::eventAnimationEnd: m_man->notify(ControlCom::eventMeleeJumpEnd, 0); break;
    }
}
