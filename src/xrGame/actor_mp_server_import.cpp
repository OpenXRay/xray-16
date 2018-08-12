#include "StdAfx.h"
#include "actor_mp_server.h"

#include "xrPhysics/phvalide.h"

void CSE_ActorMP::UPDATE_Read(NET_Packet& packet)
{
    flags = 0;
    m_u16NumItems = 1;
    velocity.set(0.f, 0.f, 0.f);

    if (get_health() <= 0)
    {
        actor_mp_state_holder tmp_state_holder;
        tmp_state_holder.read(packet);
        return;
    }
    m_state_holder.read(packet);
    R_ASSERT2(valid_pos(m_state_holder.state().position), "read bad position");

    m_AliveState.quaternion = m_state_holder.state().physics_quaternion;
    m_AliveState.angular_vel = m_state_holder.state().physics_angular_velocity;
    m_AliveState.linear_vel = m_state_holder.state().physics_linear_velocity;
    m_AliveState.force = m_state_holder.state().physics_force;
    m_AliveState.torque = m_state_holder.state().physics_torque;
    m_AliveState.position = m_state_holder.state().physics_position;

    o_Position = m_state_holder.state().position;

    accel = m_state_holder.state().logic_acceleration;

    o_model = m_state_holder.state().model_yaw;
    o_torso.yaw = m_state_holder.state().camera_yaw;
    o_torso.pitch = m_state_holder.state().camera_pitch;
    o_torso.roll = m_state_holder.state().camera_roll;

    timestamp = m_state_holder.state().time;

    weapon = m_state_holder.state().inventory_active_slot;
    mstate = m_state_holder.state().body_state_flags;
    set_health(m_state_holder.state().health);
    fRadiation = m_state_holder.state().radiation;
    m_AliveState.enabled = m_state_holder.state().physics_state_enabled;

    m_ready_to_update = true;
    // Msg("--- Client 0x%08x UPDATE_Read, health is: %2.04f", this->ID, m_state_holder.state().health);
}
