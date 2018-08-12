#include "StdAfx.h"
#include "actor_mp_server.h"

#include "xrPhysics/phvalide.h"

void CSE_ActorMP::fill_state(actor_mp_state& state)
{
    state.physics_quaternion = m_AliveState.quaternion;
    state.physics_angular_velocity = m_AliveState.angular_vel;
    state.physics_linear_velocity = m_AliveState.linear_vel;
    state.physics_force = m_AliveState.force;
    state.physics_torque = m_AliveState.torque;
    state.physics_position = m_AliveState.position;

    state.position = o_Position;

    state.logic_acceleration = accel;

    state.model_yaw = angle_normalize(o_model);
    state.camera_yaw = angle_normalize(o_torso.yaw);
    state.camera_pitch = angle_normalize(o_torso.pitch);
    state.camera_roll = angle_normalize(o_torso.roll);

    state.time = timestamp;

    state.inventory_active_slot = weapon;
    state.body_state_flags = mstate;
    state.health = get_health();
    state.radiation = fRadiation;
    state.physics_state_enabled = m_AliveState.enabled ? 1 : 0;

    m_ready_to_update = true;
}

void CSE_ActorMP::UPDATE_Write(NET_Packet& packet)
{
    if (!m_ready_to_update)
    {
        actor_mp_state state;
        fill_state(state);
        m_state_holder.relevant(state);
    }
    // Msg("--- Client 0x%08x UPDATE_Write, health is: %2.04f", this->ID, m_state_holder.state().health);
    R_ASSERT2(valid_pos(m_state_holder.state().position), "trying to write bad position");
    m_state_holder.write(packet);
}
