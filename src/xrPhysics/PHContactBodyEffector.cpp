#include "StdAfx.h"
#include "PHContactBodyEffector.h"
#include "ExtendedGeom.h"
#include "tri-colliderknoopc/dTriList.h"
#include "PhysicsCommon.h"
#include "xrEngine/GameMtlLib.h"
#include "MathUtilsOde.h"
void CPHContactBodyEffector::Init(dBodyID body, const dContact& contact, SGameMtl* material)
{
    CPHBaseBodyEffector::Init(body);
    m_contact = contact;
    m_recip_flotation = 1.f - material->fFlotationFactor;
    m_material = material;
}
void CPHContactBodyEffector::Merge(const dContact& contact, SGameMtl* material)
{
    m_recip_flotation = _max(1.f - material->fFlotationFactor, m_recip_flotation);
    // m_contact.geom.normal[0]+=contact.geom.normal[0];
    // m_contact.geom.normal[1]+=contact.geom.normal[1];
    // m_contact.geom.normal[2]+=contact.geom.normal[2];
}

void CPHContactBodyEffector::Apply()
{
    const dReal* linear_velocity = dBodyGetLinearVel(m_body);
    dReal linear_velocity_smag = dDOT(linear_velocity, linear_velocity);
    dReal linear_velocity_mag = _sqrt(linear_velocity_smag);
    dReal effect = 10000.f * m_recip_flotation * m_recip_flotation;
    dMass mass;
    dBodyGetMass(m_body, &mass);
    dReal l_air = linear_velocity_mag * effect; // force/velocity !!!
    if (l_air > mass.mass / fixed_step)
        l_air = mass.mass / fixed_step; // validate

    if (!fis_zero(l_air))
    {
        dVector3 force = {-linear_velocity[0] * l_air, -linear_velocity[1] * l_air, -linear_velocity[2] * l_air, 0.f};

        if (!m_material->Flags.is(SGameMtl::flPassable))
        {
            dVector3& norm = m_contact.geom.normal;
            accurate_normalize(norm);
            dMass m;
            dBodyGetMass(m_body, &m);
            dReal prg = 1.f * dDOT(force, norm); //+dDOT(linear_velocity,norm)*m.mass/fixed_step
            force[0] -= prg * norm[0];
            force[1] -= prg * norm[1];
            force[2] -= prg * norm[2];
        }
        dBodyAddForce(m_body, force[0], force[1], force[2]);
    }
    dBodySetData(m_body, NULL);
}
