#pragma once

#include "xrPhysics/xrPhysics.h"
#include "xrCore/xr_types.h"
#include "xrCore/_matrix.h"
#include "MathUtils.h"

class CPhysicsGeom;
struct dContactGeom;
struct dContact;
struct SGameMtl;
namespace CDB
{
class TRI;
}
class CBoneInstance;

typedef void ContactCallbackFun(bool& do_colide, bool bo1, float depth, CPhysicsGeom* my_geom, CPhysicsGeom* oposite_geom, u16 material_1, u16 material_2);

typedef void ObjectContactCallbackFun(
    bool& do_colide, bool bo1,
    CPhysicsGeom* geom1, CPhysicsGeom* geom2,
    const Fvector& contact_normal, const Fvector& contact_pos,
    SGameMtl* material_1, SGameMtl* material_2
);

typedef void BoneCallbackFun(CBoneInstance* B);

typedef void PhysicsStepTimeCallback(u32 step_start, u32 step_end);
// extern			PhysicsStepTimeCallback		*physics_step_time_callback;
struct dxGeomUserData;
struct dContactGeom;
XRPHYSICS_API bool ContactShotMarkGetEffectPars(const Fvector& pos, const Fvector& normal, CPhysicsGeom* g1, CPhysicsGeom* g2, CPhysicsGeom*& data, float& vel_cret, bool& b_invert_normal);

template <typename geom_type>
void t_get_box(const geom_type* shell, const Fmatrix& form, Fvector& sz, Fvector& c)
{
    c.set(0, 0, 0);
    VERIFY(sizeof(form.i) + sizeof(form._14_) == 4 * sizeof(float));
    for (int i = 0; 3 > i; ++i)
    {
        float lo, hi;
        const Fvector& ax = cast_fv(((const float*)&form + i * 4));
        shell->get_Extensions(ax, 0, lo, hi);
        sz[i] = hi - lo;
        c.add(Fvector().mul(ax, (lo + hi) / 2));
    }
}

enum ERestrictionType
{

    rtStalker = 0,
    rtStalkerSmall,
    rtMonsterMedium,
    rtNone,
    rtActor
};
