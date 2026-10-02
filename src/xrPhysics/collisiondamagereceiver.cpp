#include "StdAfx.h"

#include "icollisiondamagereceiver.h"
#include "IPhysicsShellHolder.h"
#include "ExtendedGeom.h"
#include "Physics.h"
#include "PHCharacter.h"

namespace {
float GeometryMotion(CPhysicsGeom* geometry, Fvector& velocity) {
    velocity.set(0, 0, 0);
    if (!geometry) return 0;
    if (geometry->ph_object && geometry->ph_object->CastType() == CPHObject::tpCharacter) {
        auto* character = static_cast<CPHCharacter*>(geometry->ph_object);
        character->GetVelocity(velocity);
        return character->Mass();
    }
    const auto body = geometry->get_body();
    if (body == INVALID_BODY_HANDLE) return 0;
    const auto mass = GetPhysicsCore()->GetBodyMass(body);
    if (mass > 0) GetPhysicsCore()->GetBodyLinearVelocity(body, velocity);
    return mass;
}
}
float NativeCollisionEnergy(CPhysicsGeom* first, CPhysicsGeom* second, const Fvector& normal) {
    Fvector firstVelocity, secondVelocity;
    const float firstMass = GeometryMotion(first, firstVelocity), secondMass = GeometryMotion(second, secondVelocity);
    const float firstSpeed = firstVelocity.dotproduct(normal), secondSpeed = secondVelocity.dotproduct(normal);
    if (firstMass > 0 && secondMass > 0) {
        const float closingSpeed = secondSpeed - firstSpeed;
        return closingSpeed > 0 ? .5f * firstMass * secondMass / (firstMass + secondMass) * closingSpeed * closingSpeed : 0;
    }
    if (firstMass > 0) return firstSpeed < 0 ? .5f * firstMass * firstSpeed * firstSpeed : 0;
    return secondSpeed > 0 ? .5f * secondMass * secondSpeed * secondSpeed : 0;
}

void DamageReceiverCollisionCallback(bool& do_colide, bool bo1, CPhysicsGeom* geom1, CPhysicsGeom* geom2, const Fvector& contact_normal, const Fvector& contact_pos, SGameMtl* material_1, SGameMtl* material_2)
{
    if (material_1->Flags.test(SGameMtl::flPassable) || material_2->Flags.test(SGameMtl::flPassable))
        return;

    CPhysicsGeom* geom_self = bo1 ? geom1 : geom2;
    CPhysicsGeom* geom_damager = bo1 ? geom2 : geom1;

    SGameMtl* material_self = bo1 ? material_1 : material_2;
    SGameMtl* material_damager = bo1 ? material_2 : material_1;

    VERIFY(geom_self);

    IPhysicsShellHolder* o_self = geom_self->ph_ref_object;
    IPhysicsShellHolder* o_damager = geom_damager ? geom_damager->ph_ref_object : nullptr;

    u16 source_id = o_damager ? o_damager->ObjectID() : u16(-1);

    ICollisionDamageReceiver* dr = o_self->ObjectPhCollisionDamageReceiver();
    VERIFY2(dr, "wrong callback");

    float damager_material_factor = material_damager->fBounceDamageFactor;

    if (o_damager && geom_damager && geom_damager->ph_object && geom_damager->ph_object->CastType() == CPHObject::tpCharacter)
        o_damager->BonceDamagerCallback(damager_material_factor);

    float dfs = (material_self->fBounceDamageFactor + damager_material_factor);
    if (fis_zero(dfs))
        return;

    Fvector dir = contact_normal;
    Fvector pos;

    Fmatrix self_transform;
    geom_self->get_xform(self_transform);
    pos.sub(contact_pos, self_transform.c);

    dr->CollisionHit(
        source_id, geom_self->m_bone_id, NativeCollisionEnergy(geom1, geom2, contact_normal) * damager_material_factor / dfs, dir, pos);
}

void BreakableObjectCollisionCallback(
    bool& do_colide, bool bo1, CPhysicsGeom* geom1, CPhysicsGeom* geom2, const Fvector& contact_normal, const Fvector& contact_pos, SGameMtl* material_1, SGameMtl* material_2)
{
    VERIFY(geom1 && geom2);

    ICollisionDamageReceiver* damag_receiver = nullptr;
    CPhysicsGeom* hitter_geom = bo1 ? geom2 : geom1;
    CPhysicsGeom* target_geom = bo1 ? geom1 : geom2;
    float norm_sign = bo1 ? -1.f : 1.f;

    if (!target_geom->ph_ref_object)
        return;

    damag_receiver = target_geom->ph_ref_object->ObjectPhCollisionDamageReceiver();
    if (!damag_receiver)
        return;

    Fvector hitter_vel;
    const float hitter_mass = GeometryMotion(hitter_geom, hitter_vel);
    const float normal_speed = hitter_vel.dotproduct(contact_normal) * norm_sign;
    const float c_damage = normal_speed < 0 ? .5f * hitter_mass * normal_speed * normal_speed : 0;
    Fvector dir = Fvector().set(contact_normal).mul(-norm_sign);

    Fvector pos = contact_pos;

    damag_receiver->CollisionHit(u16(-1), u16(-1), c_damage, dir, pos);
}
