#ifndef ACTOR_MP_STATE_H
#define ACTOR_MP_STATE_H

//#define USE_DIFFERENCES

struct actor_mp_state {
	Fquaternion		physics_quaternion;
	Fvector			physics_angular_velocity;
	Fvector			physics_linear_velocity;
	Fvector			physics_force;
	Fvector			physics_torque;
	Fvector			physics_position;
	Fvector			position;						// should be removed in future(?)
	Fvector			logic_acceleration;

	float			model_yaw;
	float			camera_yaw;						// should be removed in future
	float			camera_pitch;					// should be removed in future
	float			camera_roll;					// should be removed in future

	u32				time;							// should be removed

	float			health;
	float			radiation;

	u32				inventory_active_slot			: 4;
	u32				body_state_flags				: 15;
	u32				physics_state_enabled			: 1;
};

class actor_mp_state_holder {
private:
#ifdef USE_DIFFERENCES
	u32				m_mask;
#endif // USE_DIFFERENCES
	actor_mp_state	m_state;

private:
	IC		bool					check					(const int &flag) const;

public:
	IC								actor_mp_state_holder	();
			void					write					(NET_Packet &packet);
			void					read					(NET_Packet &packet);
			bool					relevant				(const actor_mp_state &state);
	IC		const actor_mp_state	&state					() const;
};

#include "actor_mp_state_inline.h"

#endif // ACTOR_MP_STATE_H