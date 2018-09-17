#include "StdAfx.h"
#include "control_rotation_jump.h"
#include "basemonster/base_monster.h"
#include "control_manager.h"
#include "monster_velocity_space.h"
#include "control_direction_base.h"
#include "control_movement_base.h"
#include "control_animation_base.h"

#define ROTATION_JUMP_DELAY_MIN 3000
#define ROTATION_JUMP_DELAY_MAX 5000
#define CHECK_YAW 150 * PI / 180
#define START_SPEED_DELTA 2.f

void CControlRotationJump::reinit()
{
    inherited::reinit();

    m_time_next_rotation_jump = 0;
    m_skeleton_animated = smart_cast<IKinematicsAnimated*>(m_object->Visual());
}

void CControlRotationJump::activate()
{
    m_man->capture_pure(this);
    m_man->subscribe(this, ControlCom::eventAnimationEnd);

    // disable path builder and movement
    m_man->path_stop(this);
    m_man->move_stop(this);

    float yaw = Fvector().sub(m_object->EnemyMan.get_enemy()->Position(), m_object->Position()).getH();
    m_right_side = m_man->direction().is_from_right(angle_normalize(-yaw));

    //////////////////////////////////////////////////////////////////////////
    if (m_data.flags.is(SControlRotationJumpData::eStopAtOnce))
        stop_at_once();
    else
        build_line_first();
    //////////////////////////////////////////////////////////////////////////
}

void CControlRotationJump::on_release()
{
    m_man->unlock(this, ControlCom::eControlPath);

    SControlDirectionData* ctrl_data_dir = (SControlDirectionData*)m_man->data(this, ControlCom::eControlDir);
    VERIFY(ctrl_data_dir);
    ctrl_data_dir->linear_dependency = true;

    m_man->release_pure(this);
    m_man->unsubscribe(this, ControlCom::eventAnimationEnd);

    m_time_next_rotation_jump = Device.dwTimeGlobal + Random.randI(ROTATION_JUMP_DELAY_MIN, ROTATION_JUMP_DELAY_MAX);
}

bool CControlRotationJump::check_start_conditions()
{
    if (is_active())
        return false;
    if (m_man->is_captured_pure())
        return false;

    if (!m_object->EnemyMan.get_enemy())
        return false;
    if (m_time_next_rotation_jump > Device.dwTimeGlobal)
        return false;

    Fvector enemy_position;
    enemy_position.set(m_object->EnemyMan.get_enemy()->Position());
    if (m_man->direction().is_face_target(enemy_position, CHECK_YAW))
        return false;

    SVelocityParam& velocity_run = m_object->move().get_velocity(MonsterMovement::eVelocityParameterRunNormal);
    if (!fsimilar(m_man->movement().velocity_current(), velocity_run.velocity.linear, START_SPEED_DELTA))
        return false;

    return true;
}

void CControlRotationJump::on_event(ControlCom::EEventType type, ControlCom::IEventData* dat)
{
    switch (type)
    {
    case ControlCom::eventAnimationEnd:
        if ((m_stage == eStop) && (m_data.flags.is(SControlRotationJumpData::eRotateOnce) == FALSE))
            build_line_second();
        else
            m_man->notify(ControlCom::eventRotationJumpEnd, 0);
        break;
    }
}

void CControlRotationJump::stop_at_once()
{
    m_time =
        m_man->animation().motion_time(m_right_side ? m_data.anim_stop_rs : m_data.anim_stop_ls, m_object->Visual());

    // set angular speed in exclusive force mode
    SControlDirectionData* ctrl_data_dir = (SControlDirectionData*)m_man->data(this, ControlCom::eControlDir);
    VERIFY(ctrl_data_dir);

    float target_yaw;
    if (m_data.flags.is(SControlRotationJumpData::eRotateOnce) && m_object->EnemyMan.get_enemy())
    {
        // if rotate once so rotate to enemy
        Fvector dir_to_enemy;
        dir_to_enemy.sub(m_object->EnemyMan.get_enemy()->Position(), m_object->Position());
        dir_to_enemy.normalize();
        target_yaw = angle_normalize(-dir_to_enemy.getH());
    }
    else
    {
        target_yaw =
            angle_normalize(-m_object->Direction().getH() + (m_right_side ? m_data.turn_angle : -m_data.turn_angle));
    }

    ctrl_data_dir->heading.target_angle = target_yaw;

    float cur_yaw;
    m_man->direction().get_heading(cur_yaw, target_yaw);
    ctrl_data_dir->heading.target_speed = angle_difference(cur_yaw, target_yaw) / m_time;
    ctrl_data_dir->linear_dependency = false;
    VERIFY(!fis_zero(ctrl_data_dir->heading.target_speed));

    m_stage = eStop;

    // start new animation
    SControlAnimationData* ctrl_data = (SControlAnimationData*)m_man->data(this, ControlCom::eControlAnimation);
    VERIFY(ctrl_data);

    ctrl_data->global.set_motion(m_right_side ? m_data.anim_stop_rs : m_data.anim_stop_ls);
    ctrl_data->global.actual = false;
}

void CControlRotationJump::build_line_first()
{
    // get animation time
    m_time =
        m_man->animation().motion_time(m_right_side ? m_data.anim_stop_rs : m_data.anim_stop_ls, m_object->Visual());
    // set acceleration and velocity
    m_start_velocity = m_man->movement().velocity_current();
    m_target_velocity = 0.f;

    // acceleration
    m_accel = (m_target_velocity - m_start_velocity) / m_time;

    // path distance
    m_dist = (m_target_velocity * m_target_velocity - m_start_velocity * m_start_velocity) / (2 * m_accel);

    // set angular speed in exclusive force mode
    SControlDirectionData* ctrl_data_dir = (SControlDirectionData*)m_man->data(this, ControlCom::eControlDir);
    VERIFY(ctrl_data_dir);

    float target_yaw =
        angle_normalize(-m_object->Direction().getH() + (m_right_side ? m_data.turn_angle : -m_data.turn_angle));
    ctrl_data_dir->heading.target_angle = target_yaw;

    float cur_yaw;
    m_man->direction().get_heading(cur_yaw, target_yaw);
    ctrl_data_dir->heading.target_speed = angle_difference(cur_yaw, target_yaw) / m_time;
    ctrl_data_dir->linear_dependency = false;

    VERIFY(!fis_zero(ctrl_data_dir->heading.target_speed));

    u32 velocity_mask = MonsterMovement::eVelocityParameterStand | MonsterMovement::eVelocityParameterRunNormal;
    m_stage = eStop;

    Fvector target_position;
    target_position.mad(m_object->Position(), m_object->Direction(), m_dist);

    if (!m_man->build_path_line(this, target_position, u32(-1), velocity_mask))
    {
        m_man->notify(ControlCom::eventRotationJumpEnd, 0);
    }
    else
    {
        // enable path
        SControlPathBuilderData* ctrl_path = (SControlPathBuilderData*)m_man->data(this, ControlCom::eControlPath);
        VERIFY(ctrl_path);
        ctrl_path->enable = true;

        m_man->lock(this, ControlCom::eControlPath);

        SControlMovementData* ctrl_move = (SControlMovementData*)m_man->data(this, ControlCom::eControlMovement);
        VERIFY(ctrl_move);
        ctrl_move->velocity_target = m_target_velocity;
        ctrl_move->acc = _abs(m_accel);

        // start new animation
        SControlAnimationData* ctrl_data = (SControlAnimationData*)m_man->data(this, ControlCom::eControlAnimation);
        VERIFY(ctrl_data);

        ctrl_data->global.set_motion(m_right_side ? m_data.anim_stop_rs : m_data.anim_stop_ls);
        ctrl_data->global.actual = false;
    }
}

void CControlRotationJump::build_line_second()
{
    if (!m_object->EnemyMan.get_enemy())
    {
        m_man->notify(ControlCom::eventRotationJumpEnd, 0);
        return;
    }

    // set acceleration and velocity
    m_target_velocity = m_start_velocity;
    m_start_velocity = 0;

    // get animation time
    m_time = m_man->animation().motion_time(m_right_side ? m_data.anim_run_rs : m_data.anim_run_ls, m_object->Visual());

    // acceleration
    m_accel = (m_target_velocity - m_start_velocity) / m_time;

    // path distance
    m_dist = (m_target_velocity * m_target_velocity - m_start_velocity * m_start_velocity) / (2 * m_accel);

    // set angular speed in exclusive force mode
    SControlDirectionData* ctrl_data_dir = (SControlDirectionData*)m_man->data(this, ControlCom::eControlDir);
    VERIFY(ctrl_data_dir);

    Fvector dir_to_enemy;
    dir_to_enemy.sub(m_object->EnemyMan.get_enemy()->Position(), m_object->Position());
    dir_to_enemy.normalize();

    float target_yaw = dir_to_enemy.getH();
    target_yaw = angle_normalize(-target_yaw);
    ctrl_data_dir->heading.target_angle = target_yaw;

    float cur_yaw;
    m_man->direction().get_heading(cur_yaw, target_yaw);
    ctrl_data_dir->heading.target_speed = angle_difference(cur_yaw, target_yaw) / m_time;
    ctrl_data_dir->linear_dependency = false;

    VERIFY(!fis_zero(ctrl_data_dir->heading.target_speed));

    // Velocity mask
    u32 velocity_mask = MonsterMovement::eVelocityParameterStand | MonsterMovement::eVelocityParameterRunNormal;

    m_stage = eRun;

    Fvector target_position;
    target_position.mad(m_object->Position(), dir_to_enemy, m_dist);

    if (!m_man->build_path_line(this, target_position, u32(-1), velocity_mask))
    {
        m_man->notify(ControlCom::eventRotationJumpEnd, 0);
    }
    else
    {
        // enable path
        SControlPathBuilderData* ctrl_path = (SControlPathBuilderData*)m_man->data(this, ControlCom::eControlPath);
        VERIFY(ctrl_path);
        ctrl_path->enable = true;

        m_man->lock(this, ControlCom::eControlPath);

        SControlMovementData* ctrl_move = (SControlMovementData*)m_man->data(this, ControlCom::eControlMovement);
        VERIFY(ctrl_move);
        ctrl_move->velocity_target = m_target_velocity;
        ctrl_move->acc = m_accel;

        // start new animation
        SControlAnimationData* ctrl_data = (SControlAnimationData*)m_man->data(this, ControlCom::eControlAnimation);
        VERIFY(ctrl_data);

        ctrl_data->global.set_motion(m_right_side ? m_data.anim_run_rs : m_data.anim_run_ls);
        ctrl_data->global.actual = false;
    }
}
