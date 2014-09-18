#ifndef ACTOR_MP_STATE_INLINE_H
#define ACTOR_MP_STATE_INLINE_H

IC actor_mp_state_holder::actor_mp_state_holder			()
{
	ZeroMemory						(&m_state,sizeof(m_state));
	m_state.physics_quaternion.z	= 1.f;
}

IC const actor_mp_state &actor_mp_state_holder::state	() const
{
	return							(m_state);
}

#endif // ACTOR_MP_STATE_INLINE_H