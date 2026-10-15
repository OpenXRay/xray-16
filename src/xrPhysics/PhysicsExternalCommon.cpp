#include "StdAfx.h"
#include "PhysicsExternalCommon.h"
#include "ExtendedGeom.h"
#include "Geometry.h"
#include "PHCharacter.h"
#include "xrPhysicsCore/IPhysicsCore.h"

bool ContactShotMarkGetEffectPars(const Fvector& pos, const Fvector& normal, CPhysicsGeom* g1, CPhysicsGeom* g2, CPhysicsGeom*& data, float& vel_cret, bool& b_invert_normal)
{
    if (!g1 || !g2) return false;

    auto measure = [&](CPhysicsGeom* geometry) {
        Fvector velocity;
        float mass = 0;
        if (geometry->ph_object && geometry->ph_object->CastType() == CPHObject::tpCharacter) {
            auto* character = static_cast<CPHCharacter*>(geometry->ph_object);
            mass = character->Mass();
            character->GetVelocity(velocity);
        } else {
            const auto body = geometry->get_body();
            if (body == INVALID_BODY_HANDLE) return false;
            mass = GetPhysicsCore()->GetBodyMass(body);
            if (mass <= 0) return false;
            GetPhysicsCore()->GetBodyPointVelocity(body, pos, velocity);
        }
        if (mass <= 0) return false;
        data = geometry;
        vel_cret = _abs(velocity.dotproduct(normal)) * _sqrt(mass);
        return true;
    };
    b_invert_normal = false;
    if (measure(g1)) return true;
    b_invert_normal = true;
    return measure(g2);
}
