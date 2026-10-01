#include "StdAfx.h"
#include "PhysicsShell.h"
#include "PHInterpolation.h"
#include "PHElement.h"
#include "PHObject.h"
#include "PHWorld.h"
#include "PHShell.h"
#include "xrPhysicsCore/IPhysicsCore.h"

void CPHElement::get_State(SPHNetState& state)
{
    GetGlobalPositionDynamic(&state.position);
    getQuaternion(state.quaternion);
    m_char_handle_interpolation.GetPosition(state.previous_position, 0);
    m_char_handle_interpolation.GetRotation(state.previous_quaternion, 0);
    get_LinearVel(state.linear_vel);
    get_AngularVel(state.angular_vel);
    getForce(state.force);
    getTorque(state.torque);

    // Добавили безопасную проверку m_char_handle
    if (!isActive() || m_char_handle == INVALID_CHARACTER_VIRTUAL_HANDLE)
    {
        state.enabled = false;
        return;
    }

    // Заменили dBodyIsEnabled
    state.enabled = GetPhysicsCore()->IsBodyActive(m_char_handle);
}

void CPHElement::set_State(const SPHNetState& state)
{
    // bUpdate=true;
    Fmatrix transform;
    transform.rotation(state.quaternion);
    transform.c.set(state.position);

    if (isActive())
    {
        GetPhysicsCore()->SetBodyTransform(m_char_handle, transform);
        CPHDisablingFull::Reinit();
        m_shell->spatial_move();
    }

    m_char_handle_interpolation.SetPosition(state.previous_position, 0);
    m_char_handle_interpolation.SetRotation(state.previous_quaternion, 0);
    m_char_handle_interpolation.SetPosition(state.position, 1);
    m_char_handle_interpolation.SetRotation(state.quaternion, 1);
    set_LinearVel(state.linear_vel);
    set_AngularVel(state.angular_vel);
    setForce(state.force);
    setTorque(state.torque);

    if (!isActive())
        return;

#if 1
    if (m_char_handle != INVALID_CHARACTER_VIRTUAL_HANDLE)
    {
        bool is_active = GetPhysicsCore()->IsBodyActive(m_char_handle);

        if (state.enabled && !is_active)
        {
            GetPhysicsCore()->ActivateBody(m_char_handle);
            m_shell->EnableObject(0);
        }
        if (!state.enabled && is_active)
        {
            m_shell->DisableObject();
            Disable();
        }
    }
#endif

    CPHDisablingFull::Reinit();
    m_flags.set(flUpdate, TRUE);
}

void CPHElement::net_Export(NET_Packet& P)
{
    SPHNetState state;
    get_State(state);
    state.net_Export(P);
}

void CPHElement::net_Import(NET_Packet& P)
{
    SPHNetState state;
    state.net_Import(P);
    set_State(state);
}
