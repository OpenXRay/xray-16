#include "StdAfx.h"
#include "actor_mp_client.h"
#include "CharacterPhysicsSupport.h"
#include "Inventory.h"
#include "xrPhysics/phvalide.h"

///	DONE (111 bytes cut from 138 bytes = 27 bytes, total 511.11% or 19.56%)
// 3     health is form 0.f to 1.f, so use only 8 bits for it, the rest one we will use for mask
// 1     removed flags
// 12    removed logical position
// 12    quantized camera stuff and model yaw
// 3     removed team, squad, group
// 0.125 body state flags
// 6     removed logical acceleration
// 6     removed logical velocity
// 3.5   quantized radiation
// 0.5   quantized active_slot
// 2     dead actors are not synchronized at all, number of synchronization items is no more
// 0.875 physics state enabled
// 12    removed angular velocity
// 9     quantized linear velocity
// 12    removed force
// 12    removed torque
// 16    removed quaternion

///	TODO
// 4   u32 move time to global packet (+ much less QPC calls)
// 12  3*float write unaffected_r_torso stuff only in case when greande is an active slot (?)
// X   mask all the changes we sent

/// LEFT
// 4  time
// 12 position
// 3  linear velocity
// 1  model yaw
// 3  camera
// 4  inventory_active_slot,body_state_flags,health,radiation,physics_state_enabled

void CActorMP::fill_state(actor_mp_state& state)
{
    if (OnClient())
    {
        // R_ASSERT						(g_Alive());
        // R_ASSERT2						(PHGetSyncItemsNumber() == 1,make_string("PHGetSyncItemsNumber() returned %d, health
        // =
        // %.2f",PHGetSyncItemsNumber(),GetfHealth()));
    }

    SPHNetState State;
    PHGetSyncItem(0)->get_State(State);

//	static test = false;
//	if (test) {
#if 0
		Msg							("Frame [%d], object [%d]",Device.dwFrame,ID());
//		Msg							("quaternion   : [%f][%f][%f][%f]",State.quaternion.x,State.quaternion.y,State.quaternion.z,State.quaternion.w);
//		Msg							("angular      : [%f][%f][%f]",State.angular_vel.x,State.angular_vel.y,State.angular_vel.z);
		Msg							("linear       : [%f][%f][%f]",State.linear_vel.x,State.linear_vel.y,State.linear_vel.z);
//		Msg							("force        : [%f][%f][%f]",State.force.x,State.force.y,State.force.z);
//		Msg							("torque       : [%f][%f][%f]",State.torque.x,State.torque.y,State.torque.z);
//		Msg							("acceleration : [%f][%f][%f]",NET_SavedAccel.x,NET_SavedAccel.y,NET_SavedAccel.z);
		Msg							("model_yaw    : [%f]",angle_normalize(r_model_yaw));
		Msg							("camera_yaw   : [%f]",angle_normalize(unaffected_r_torso.yaw));
//		Msg							("camera_pitch : [%f]",angle_normalize(unaffected_r_torso.pitch));
//		Msg							("camera_roll  : [%f]",angle_normalize(unaffected_r_torso.roll));
//	}
#endif // 0

    state.physics_quaternion = State.quaternion;
    state.physics_angular_velocity = State.angular_vel;
    state.physics_linear_velocity = State.linear_vel;
    state.physics_force = State.force;
    state.physics_torque = State.torque;
    state.physics_position = State.position;

    state.position = Position();

    state.logic_acceleration = NET_SavedAccel;

    state.model_yaw = angle_normalize(r_model_yaw);
    state.camera_yaw = angle_normalize(unaffected_r_torso.yaw);
    state.camera_pitch = angle_normalize(unaffected_r_torso.pitch);
    state.camera_roll = angle_normalize(unaffected_r_torso.roll);

    state.time = Level().timeServer();

    state.inventory_active_slot = inventory().GetActiveSlot();
    state.body_state_flags = mstate_real & 0x0000ffff;
    state.health = GetfHealth();
    // because after packing to 1 byte, this value can be positive...
    if (state.health < EPS)
        state.health = 0;

    state.radiation = g_Radiation() / 100.0f;
    state.physics_state_enabled = State.enabled ? 1 : 0;
}

BOOL CActorMP::net_Relevant()
{
    if (OnClient())
    {
        /*
        if (!g_Alive())
            return						(false);

        if (m_i_am_dead)
            return						(false);*/
    }

    if (character_physics_support()->IsRemoved())
        return (false);

    actor_mp_state state;
    fill_state(state);
    return (m_state_holder.relevant(state));
}

void CActorMP::net_Export(NET_Packet& packet)
{
    if (OnClient())
    {
        // Msg("net_Export: ID is: 0x%08x, is going to send health %2.04f", this->ID(), m_state_holder.state().health);
        // R_ASSERT						(g_Alive());
        // R_ASSERT						(PHGetSyncItemsNumber() == 1);
    }
    R_ASSERT2(valid_pos(m_state_holder.state().position), "trying to export bad position");
    m_state_holder.write(packet);
}
