#include "StdAfx.h"

#include "PHDynamicData.h"
#include "Physics.h"
#include "ExtendedGeom.h"
#include "xrCDB/Intersect.hpp"

#include "PHAICharacter.h"
#include "xrEngine/device.h"
#include "xrPhysicsCore/IPhysicsCore.h"

#ifdef DEBUG
#include "debug_output.h"
#endif

CPHAICharacter::CPHAICharacter() { m_forced_physics_control = false; }

void CPHAICharacter::Create(Fvector sizes)
{
    inherited::Create(sizes);
    m_forced_physics_control = false;
}

bool CPHAICharacter::TryPosition(Fvector target, bool exact_state)
{
    if (!b_exist || m_char_handle == INVALID_CHARACTER_VIRTUAL_HANDLE ||
        m_forced_physics_control || JumpState() || DoCollideObj()) return false;
    Fvector original, original_velocity;
    GetPosition(original);
    GetVelocity(original_velocity);
    Fvector displacement = Fvector().sub(target, original);
    const float distance = displacement.magnitude();
    if (distance < EPS || Device.fTimeDelta < EPS) return true;
    auto* core = GetPhysicsCore();
    const float original_gravity = core->GetCharacterVirtualGravityFactor(m_char_handle);
    core->SetCharacterVirtualGravityFactor(m_char_handle, 0);
    const int steps = std::clamp(iCeil(distance / FootRadius()), 1, 15);
    Fvector velocity = Fvector().set(displacement).div(steps * fixed_step);
    Enable();
    for (int index = 0; index < steps; ++index) {
        core->SetCharacterVirtualVelocity(m_char_handle, velocity);
        core->UpdateCharacterVirtual(m_char_handle, fixed_step, Fvector().set(0, 0, 0));
    }
    core->SetCharacterVirtualGravityFactor(m_char_handle, original_gravity);
    core->SetCharacterVirtualVelocity(m_char_handle, original_velocity);
    Fvector position;
    GetPosition(position);
    m_last_move.sub(position, original).div(Device.fTimeDelta);
    m_safe_velocity = m_last_move;
    m_char_handle_interpolation.UpdatePositions();
    spatial_move();
    const bool success = Fvector().sub(position, target).magnitude() < .05f;
    if (success) Disable();
    m_collision_damage_info.m_contact_velocity = 0;
    return success;
}
void CPHAICharacter::GetSavedVelocity(Fvector& vvel)
{
    if (m_last_move.square_magnitude() > EPS_L)
        vvel.set(m_last_move);
    else
        inherited::GetSavedVelocity(vvel);
}

void CPHAICharacter::GetVelocity(Fvector& vvel) const
{
    if (m_last_move.square_magnitude() > EPS_L)
        vvel.set(m_last_move);
    else
        inherited::GetVelocity(vvel);
}

void CPHAICharacter::Jump(const Fvector& jump_velocity)
{
    b_jump = true;
    m_jump_accel.set(jump_velocity);
}

void CPHAICharacter::ValidateWalkOn()
{
    inherited::ValidateWalkOn();
}

void CPHAICharacter::InitContact(bool& do_collide, bool bo1, float depth, CPhysicsGeom* my_geom, CPhysicsGeom* oposite_geom, u16 material_idx_1, u16 material_idx_2)
{
    SGameMtl* material_1 = GMLib.GetMaterialByIdx(material_idx_1);
    SGameMtl* material_2 = GMLib.GetMaterialByIdx(material_idx_2);

    if ((material_1 && material_1->Flags.test(SGameMtl::flActorObstacle)) ||
        (material_2 && material_2->Flags.test(SGameMtl::flActorObstacle)))
        do_collide = true;

    inherited::InitContact(do_collide, bo1, depth, my_geom, oposite_geom, material_idx_1, material_idx_2);

    // Примечание: В оригинальном ODE здесь сбрасывалось трение (c->surface.mu = 0.00f).
    // В Jolt Physics трение настраивается через Contact Listener.

    if (my_geom && oposite_geom && my_geom->ph_object && oposite_geom->ph_object &&
        my_geom->ph_object->CastType() == tpCharacter &&
        oposite_geom->ph_object->CastType() == tpCharacter)
    {
        b_on_object = true;
        b_valide_wall_contact = false;
    }

#ifdef DEBUG
    if (debug_output().ph_dbg_draw_mask().test(phDbgNeverUseAiPhMove))
        do_collide = false;
#endif
}

#ifdef DEBUG
void CPHAICharacter::OnRender()
{
    inherited::OnRender();
}
#endif
