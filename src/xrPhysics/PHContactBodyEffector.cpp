#include "StdAfx.h"
#include "PHContactBodyEffector.h"
#include "ExtendedGeom.h"
#include "PhysicsCommon.h"
#include "xrPhysicsCore/IPhysicsCore.h"

void CPHContactBodyEffector::Init(CharacterVirtualHandle body, const Fvector& normal, float depth, SGameMtl* material)
{
    CPHBaseBodyEffector::Init(body);
    m_contact_normal = normal;
    m_contact_depth = depth;
    m_recip_flotation = 1.f - material->fFlotationFactor;
    m_material = material;
}

void CPHContactBodyEffector::Merge(const Fvector& normal, float depth, SGameMtl* material)
{
    m_recip_flotation = _max(1.f - material->fFlotationFactor, m_recip_flotation);

}
void CPHContactBodyEffector::Apply()
{
    if (m_char_handle == INVALID_CHARACTER_VIRTUAL_HANDLE) return;

    Fvector linear_velocity;
    GetPhysicsCore()->GetBodyLinearVelocity(m_char_handle, linear_velocity);

    float linear_velocity_smag = linear_velocity.square_magnitude();
    float linear_velocity_mag = _sqrt(linear_velocity_smag);
    float effect = 10000.f * m_recip_flotation * m_recip_flotation;

    float mass = GetPhysicsCore()->GetBodyMass(m_char_handle);

    float l_air = linear_velocity_mag * effect;
    if (l_air > mass / fixed_step)
        l_air = mass / fixed_step;

    if (!fis_zero(l_air))
    {
        Fvector force;
        force.set(linear_velocity);
        force.mul(-l_air);

        if (!m_material->Flags.is(SGameMtl::flPassable))
        {
            Fvector norm = m_contact_normal;
            norm.normalize_safe();

            float prg = force.dotproduct(norm);
            force.mad(norm, -prg);
        }

        GetPhysicsCore()->ApplyForce(m_char_handle, force);
    }
}
