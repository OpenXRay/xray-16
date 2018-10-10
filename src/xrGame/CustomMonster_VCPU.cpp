#include "StdAfx.h"
#include "CustomMonster.h"
#include "movement_manager.h"

void CCustomMonster::mk_rotation(Fvector& dir, SRotation& R)
{
    // parse yaw
    Fvector DYaw;
    DYaw.set(dir.x, 0.f, dir.z);
    DYaw.normalize_safe();
    clamp(DYaw.x, -0.9999999f, 0.9999999f);
    clamp(DYaw.y, -0.9999999f, 0.9999999f);
    clamp(DYaw.z, -0.9999999f, 0.9999999f);

    if (DYaw.x >= 0)
        R.yaw = acosf(DYaw.z);
    else
        R.yaw = 2 * PI - acosf(DYaw.z);

    // parse pitch
    dir.normalize_safe();
    R.pitch = -asinf(dir.y);
}

void CCustomMonster::Exec_Look(float dt)
{
    if (animation_movement_controlled())
        return;

    movement().m_body.current.yaw = angle_normalize_signed(movement().m_body.current.yaw);
    movement().m_body.current.pitch = angle_normalize_signed(movement().m_body.current.pitch);
    movement().m_body.target.yaw = angle_normalize_signed(movement().m_body.target.yaw);
    movement().m_body.target.pitch = angle_normalize_signed(movement().m_body.target.pitch);

    float pitch_speed = get_custom_pitch_speed(movement().m_body.speed);
    angle_lerp_bounds(movement().m_body.current.yaw, movement().m_body.target.yaw, movement().m_body.speed, dt);
    angle_lerp_bounds(movement().m_body.current.pitch, movement().m_body.target.pitch, pitch_speed, dt);

    Fvector P = Position();
    XFORM().setHPB(-NET_Last.o_model, -NET_Last.o_torso.pitch, 0);
    Position() = P;
}
