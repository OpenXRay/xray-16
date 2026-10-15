#include "StdAfx.h"
#include <limits>
#include "PHDynamicData.h"
#include "Physics.h"
#include "PHContactBodyEffector.h"
#include "PHCollideValidator.h"
#include "ExtendedGeom.h"
#include "xrPhysicsCore/IPhysicsCore.h" // Наше ядро Jolt

#ifdef DEBUG
#include "debug_output.h"
#endif

extern CPHWorld* ph_world;

const float default_w_limit = 9.8174770f;
const float default_l_limit = 150.f;
const float default_l_scale = 1.01f;
const float default_w_scale = 1.01f;
const float default_k_l = 0.0002f;
const float default_k_w = 0.05f;

extern const u16 max_joint_allowed_for_exeact_integration = 30;

// base params
const float base_fixed_step = 0.02f;
const float base_erp = 0.54545456f;
const float base_cfm = 1.1363636e-006f;
// base params
float fixed_step = 0.01f;
float world_cfm = CFM(SPRING_S(base_cfm, base_erp, base_fixed_step), DAMPING(base_cfm, base_erp));
float world_erp = ERP(SPRING_S(base_cfm, base_erp, base_fixed_step), DAMPING(base_cfm, base_erp));
float world_spring = 1.0f * SPRING(world_cfm, world_erp);
float world_damping = 1.0f * DAMPING(world_cfm, world_erp);

const float default_world_gravity = 2 * 9.81f;

int phIterations = 18;
float phTimefactor = 1.f;
Fbox phBoundaries = {1000.f, 1000.f, -1000.f, -1000.f};

CBlockAllocator<CPHContactBodyEffector, 128> ContactEffectors;

void FixBody(CharacterVirtualHandle body, float ext_param, float mass_param)
{
    auto* core = GetPhysicsCore();
    PhysicsMassProperties properties;
    properties.mass = mass_param;
    const float inertia = .4f * mass_param * ext_param * ext_param;
    properties.inertia.i.set(inertia, 0, 0);
    properties.inertia.j.set(0, inertia, 0);
    properties.inertia.k.set(0, 0, inertia);
    core->SetBodyMassProperties(body, properties);
    core->SetBodyGravityFactor(body, 0);
    const Fvector zero = Fvector().set(0, 0, 0);
    core->SetBodyLinearVelocity(body, zero);
    core->SetBodyAngularVelocity(body, zero);
    core->SetBodyForce(body, zero);
    core->SetBodyTorque(body, zero);
}

void FixBody(CharacterVirtualHandle body)
{
    FixBody(body, fix_ext_param, fix_mass_param);
}

void BodyCutForce(CharacterVirtualHandle body, float l_limit, float w_limit)
{
    auto* core = GetPhysicsCore();
    Fvector force;
    core->GetBodyForce(body, force);
    const float limit = l_limit * core->GetBodyMass(body) / fixed_step;
    const float magnitude = force.magnitude();
    if (magnitude > limit) { force.mul(limit / magnitude); core->SetBodyForce(body, force); }
    Fvector torque;
    core->GetBodyTorque(body, torque);
    if (torque.square_magnitude() < 1e-6f) return;
    const auto mass = core->GetBodyMassProperties(body);
    if (mass.mass <= 0) return;
    Fmatrix inverse, orientation;
    if (!inverse.invert_b(mass.inertia)) return;
    core->GetBodyTransform(body, orientation);
    orientation.transpose();
    Fvector localTorque, acceleration;
    orientation.transform_dir(localTorque, torque);
    inverse.transform_dir(acceleration, localTorque);
    const float accelerationMagnitude = acceleration.magnitude();
    const float angularLimit = w_limit / fixed_step;
    if (accelerationMagnitude > angularLimit) {
        torque.mul(angularLimit / accelerationMagnitude);
        core->SetBodyTorque(body, torque);
    }
}

float E_NlS(CharacterVirtualHandle body, const Fvector& norm, float norm_sign)
{
    if (body == INVALID_CHARACTER_VIRTUAL_HANDLE) return 0.f;

    Fvector vel;
    GetPhysicsCore()->GetBodyLinearVelocity(body, vel);

    float mass = GetPhysicsCore()->GetBodyMass(body);
    if (mass <= 0.f) return 0.f;

    float vel_pr = vel.dotproduct(norm) * norm_sign;

    if (vel_pr > 0.f) return 0.f;

    return (vel_pr * vel_pr * mass) / 2.f;
}

float E_NLD(CharacterVirtualHandle b1, CharacterVirtualHandle b2, const Fvector& norm)
{
    if (b1 == INVALID_CHARACTER_VIRTUAL_HANDLE || b2 == INVALID_CHARACTER_VIRTUAL_HANDLE) return 0.f;

    Fvector vel1, vel2;
    GetPhysicsCore()->GetBodyLinearVelocity(b1, vel1);
    GetPhysicsCore()->GetBodyLinearVelocity(b2, vel2);

    float m1 = GetPhysicsCore()->GetBodyMass(b1);
    float m2 = GetPhysicsCore()->GetBodyMass(b2);

    float vel_pr1 = vel1.dotproduct(norm);
    float vel_pr2 = vel2.dotproduct(norm);

    if (vel_pr1 > vel_pr2) return 0.f;

    Fvector impuls1 = vel1; impuls1.mul(m1);
    Fvector impuls2 = vel2; impuls2.mul(m2);

    Fvector c_mas_impuls;
    c_mas_impuls.add(impuls1, impuls2);

    float cmass = m1 + m2;
    if (cmass <= 0.f) return 0.f;

    Fvector c_mass_vel = c_mas_impuls;
    c_mass_vel.div(cmass);

    float c_mass_vel_prg = c_mass_vel.dotproduct(norm);

    float kin_energy_start = (vel_pr1 * vel_pr1 * m1) / 2.f + (vel_pr2 * vel_pr2 * m2) / 2.f;
    float kin_energy_end = (c_mass_vel_prg * c_mass_vel_prg * cmass) / 2.f;

    return (kin_energy_start - kin_energy_end);
}

float E_NL(CharacterVirtualHandle b1, CharacterVirtualHandle b2, const Fvector& norm)
{
    VERIFY(b1 != INVALID_CHARACTER_VIRTUAL_HANDLE || b2 != INVALID_CHARACTER_VIRTUAL_HANDLE);
    if (b1 != INVALID_CHARACTER_VIRTUAL_HANDLE)
    {
        if (b2 != INVALID_CHARACTER_VIRTUAL_HANDLE)
            return E_NLD(b1, b2, norm);
        else
            return E_NlS(b1, norm, 1);
    }
    else
        return E_NlS(b2, norm, -1);
}

void ApplyGravityAccel(CharacterVirtualHandle body, const Fvector& accel)
{
    GetPhysicsCore()->ApplyForce(body, Fvector().set(accel).mul(GetPhysicsCore()->GetBodyMass(body)));
}
