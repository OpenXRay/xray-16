#include "StdAfx.h"

#include "PHCharacter.h"
#include "PHDynamicData.h"
#include "Physics.h"
#include "ExtendedGeom.h"
#include "IPhysicsShellHolder.h"

#include "xrCDB/Intersect.hpp"

#include "PHAICharacter.h"
#include "PHActorCharacter.h"
#include "xrPhysicsCore/IPhysicsCore.h"

CPHCharacter::CPHCharacter(void) : CPHDisablingTranslational()
{
    m_params.acceleration = 0.001f;
    m_params.velocity = 0.0001f;
    m_char_handle = INVALID_CHARACTER_VIRTUAL_HANDLE;
    m_safe_velocity.set(0.f, 0.f, 0.f);
    m_force.set(0.f, 0.f, 0.f);
    m_safe_position.set(0.f, 0.f, 0.f);
    m_mean_y = 0.f;
    m_new_restriction_type = m_restriction_type = rtNone;
    b_actor_movable = true;
    p_lastMaterialIDX = &lastMaterialIDX;
    lastMaterialIDX = GAMEMTL_NONE_IDX;
    injuriousMaterialIDX = GAMEMTL_NONE_IDX;
    m_creation_step = u64(-1);
    b_in_touch_resrtrictor = false;
    m_current_object_radius = -1.f;
}

CPHCharacter::~CPHCharacter(void) {}

void CPHCharacter::FreezeContent()
{
    if (m_char_handle != INVALID_CHARACTER_VIRTUAL_HANDLE)
        GetPhysicsCore()->DeactivateCharacterVirtual(m_char_handle);
    CPHObject::FreezeContent();
}

void CPHCharacter::UnFreezeContent()
{
    if (m_char_handle != INVALID_CHARACTER_VIRTUAL_HANDLE)
        GetPhysicsCore()->ActivateCharacterVirtual(m_char_handle);
    CPHObject::UnFreezeContent();
}

void CPHCharacter::getForce(Fvector& force)
{
    force = m_force;
}

void CPHCharacter::setForce(const Fvector& force)
{
    m_force = force;
}

void CPHCharacter::get_State(SPHNetState& state)
{
    GetPosition(state.position);
    m_char_handle_interpolation.GetPosition(state.previous_position, 0);
    GetVelocity(state.linear_vel);
    getForce(state.force);

    state.angular_vel.set(0.f, 0.f, 0.f);
    state.quaternion.identity();
    state.previous_quaternion.identity();
    state.torque.set(0.f, 0.f, 0.f);

    if (!b_exist)
    {
        state.enabled = false;
        return;
    }
    state.enabled = CPHObject::is_active();
}

void CPHCharacter::set_State(const SPHNetState& state)
{
    m_char_handle_interpolation.SetPosition(state.previous_position, 0);
    m_char_handle_interpolation.SetPosition(state.position, 1);
    SetPosition(state.position);
    SetVelocity(state.linear_vel);
    setForce(state.force);

    if (!b_exist)
        return;
    if (state.enabled)
        Enable();
    else
        Disable();
}

void CPHCharacter::Disable()
{
    CPHObject::deactivate();
    if (m_char_handle != INVALID_CHARACTER_VIRTUAL_HANDLE)
        GetPhysicsCore()->DeactivateCharacterVirtual(m_char_handle);
    m_char_handle_interpolation.ResetPositions();
}

void CPHCharacter::Enable()
{
    if (!b_exist)
        return;
    CPHObject::activate();
    if (m_char_handle != INVALID_CHARACTER_VIRTUAL_HANDLE)
        GetPhysicsCore()->ActivateCharacterVirtual(m_char_handle);
}

void CPHCharacter::GetSavedVelocity(Fvector& vvel)
{
    if (IsEnabled())
        vvel.set(m_safe_velocity);
    else
        GetVelocity(vvel);
}

void CPHCharacter::CutVelocity(float l_limit, float a_limit)
{
    if (m_char_handle == INVALID_CHARACTER_VIRTUAL_HANDLE) return;

    Fvector linear_velocity;
    GetPhysicsCore()->GetCharacterVirtualVelocity(m_char_handle, linear_velocity);

    float mag = linear_velocity.magnitude();
    if (mag > l_limit && !fis_zero(mag))
    {
        linear_velocity.mul(l_limit / mag);
        GetPhysicsCore()->SetCharacterVirtualVelocity(m_char_handle, linear_velocity);
    }
}

const Fmatrix& CPHCharacter::XFORM() const
{
    return m_phys_ref_object->ObjectXFORM();
}

void CPHCharacter::get_LinearVel(Fvector& velocity) const { GetVelocity(velocity); }
void CPHCharacter::get_AngularVel(Fvector& velocity) const { velocity.set(0, 0, 0); }

const Fvector& CPHCharacter::mass_Center() const
{
    // В оригинале X-Ray здесь возвращался cast_fv(dBodyGetLinearVel(m_char_handle)),
    // что логически является скоростью, а не позицией. Сохраняем это поведение.
    if (m_char_handle != INVALID_CHARACTER_VIRTUAL_HANDLE)
        GetPhysicsCore()->GetCharacterVirtualVelocity(m_char_handle, m_last_velocity_cache);
    else
        m_last_velocity_cache.set(0,0,0);

    return m_last_velocity_cache;
}

void CPHCharacter::get_body_position(Fvector& p)
{
    VERIFY(b_exist);
    VERIFY(m_char_handle != INVALID_CHARACTER_VIRTUAL_HANDLE);
    GetPhysicsCore()->GetCharacterVirtualPosition(m_char_handle, p);
}

void CPHCharacter::UpdateL1()
{
    m_stateL1.Reset();
    if (m_char_handle == INVALID_CHARACTER_VIRTUAL_HANDLE) return;

    Fvector position;
    GetPhysicsCore()->GetCharacterVirtualPosition(m_char_handle, position);

    Fvector velocity;
    GetPhysicsCore()->GetCharacterVirtualVelocity(m_char_handle, velocity);

    CPHDisablingBase::UpdateValues(position, velocity);
}

void virtual_move_collide_callback(
    bool& do_colide, bool bo1,
    CPhysicsGeom* my_geom, CPhysicsGeom* oposite_geom,
    const Fvector& contact_normal, const Fvector& contact_pos,
    SGameMtl* material_1, SGameMtl* material_2)
{
    if (!do_colide)
        return;
    do_colide = false;
}

void CPHCharacter::fix_body_rotation()
{
    if (m_char_handle != INVALID_CHARACTER_VIRTUAL_HANDLE)
    {
        // Для Jolt можно также принудительно обнулить кватернион вращения, если требуется,
        // но обычно для Character используется LockRotations, поэтому здесь ничего не делаем.
    }
}

CPHCharacter* create_ai_character() { return xr_new<CPHAICharacter>(); }
CPHCharacter* create_actor_character(bool single_game) { return xr_new<CPHActorCharacter>(single_game); }
