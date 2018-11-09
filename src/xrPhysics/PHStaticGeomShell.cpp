#include "StdAfx.h"
#include "PHStaticGeomShell.h"
#include "SpaceUtils.h"

#include "IPhysicsShellHolder.h"
#include "PHCharacter.h"
#include "IClimableObject.h"

#include "Include/xrRender/Kinematics.h"
#include "PHCollideValidator.h"
#include "xrEngine/xr_object.h"
#include "xrCore/Animation/Bone.hpp"

void CPHStaticGeomShell::get_spatial_params()
{
    Fvector AABB;
    spatialParsFromDGeom(dSpacedGeometry(), spatial.sphere.P, AABB, spatial.sphere.R);
}

void CPHStaticGeomShell::PhDataUpdate(dReal step)
{
    Island().Step(step);
    Island().Unmerge();
    PhysicsRefObject()->enable_notificate();
    CPHUpdateObject::Deactivate();
}
void CPHStaticGeomShell::Activate(const Fmatrix& form)
{
    build();
    setStaticForm(form);
    get_spatial_params();
    spatial_register();
}

void CPHStaticGeomShell::Deactivate()
{
    spatial_unregister();
    CPHUpdateObject::Deactivate();
    destroy();
}

CPHStaticGeomShell::CPHStaticGeomShell() { spatial.type |= STYPE_PHYSIC; }
void cb(CBoneInstance* B) {}
void P_BuildStaticGeomShell(CPHStaticGeomShell* pUnbrokenObject, IPhysicsShellHolder* obj,
    ObjectContactCallbackFun* object_contact_callback, const Fobb& b)
{
    pUnbrokenObject->add_Box(b);
    pUnbrokenObject->Activate(obj->ObjectXFORM());

    pUnbrokenObject->set_PhysicsRefObject(obj);
    // m_pUnbrokenObject->SetPhObjectInGeomData(m_pUnbrokenObject);
    pUnbrokenObject->set_ObjectContactCallback(object_contact_callback);
    CPHCollideValidator::SetNonDynamicObject(*pUnbrokenObject);
}
CPHStaticGeomShell* P_BuildStaticGeomShell(
    IPhysicsShellHolder* obj, ObjectContactCallbackFun* object_contact_callback, const Fobb& b)
{
    CPHStaticGeomShell* pUnbrokenObject = new CPHStaticGeomShell();
    P_BuildStaticGeomShell(pUnbrokenObject, obj, object_contact_callback, b);
    return pUnbrokenObject;
}

IPHStaticGeomShell* P_BuildStaticGeomShell(IPhysicsShellHolder* obj, ObjectContactCallbackFun* object_contact_callback)
{
    Fobb b;
    // IRenderVisual* V=obj->ObjectVisual();
    // R_ASSERT2(V,"need visual to build");
    IKinematics* K = obj->ObjectKinematics();
    R_ASSERT2(K, "need visual to build");
    K->CalculateBones(TRUE); //. bForce - was TRUE

    // V->getVisData().box.getradius	(b.m_halfsize);
    K->GetBox().getradius(b.m_halfsize);

    b.xform_set(Fidentity);
    CPHStaticGeomShell* pUnbrokenObject = P_BuildStaticGeomShell(obj, object_contact_callback, b);

    // IKinematics* K=smart_cast<IKinematics*>(V); VERIFY(K);
    K->CalculateBones(TRUE);
    for (u16 k = 0; k < K->LL_BoneCount(); k++)
    {
        K->LL_GetBoneInstance(k).set_callback(bctPhysics, cb, K->LL_GetBoneInstance(k).callback_param(), TRUE);
        // K->LL_GetBoneInstance(k).Callback_overwrite = TRUE;
        // K->LL_GetBoneInstance(k).Callback = cb;
    }
    return pUnbrokenObject;
}

void DestroyStaticGeomShell(IPHStaticGeomShell*& UnbrokenObject)
{
    if (!UnbrokenObject)
        return;
    CPHStaticGeomShell* gs = static_cast<CPHStaticGeomShell*>(UnbrokenObject);
    gs->Deactivate();
    xr_delete(gs);
    UnbrokenObject = 0;
}

class IClimableObject;
class CPHLeaderGeomShell : public CPHStaticGeomShell
{
    IClimableObject* m_pClimable;

public:
    CPHLeaderGeomShell(IClimableObject* climable);
    void near_callback(CPHObject* obj);
};

IPHStaticGeomShell* P_BuildLeaderGeomShell(IClimableObject* obj, ObjectContactCallbackFun* callback, const Fobb& b)
{
    CPHLeaderGeomShell* pStaticShell = new CPHLeaderGeomShell(obj);
    P_BuildStaticGeomShell(smart_cast<CPHStaticGeomShell*>(pStaticShell), smart_cast<IPhysicsShellHolder*>(obj), 0, b);
    pStaticShell->SetMaterial(obj->Material());
    pStaticShell->set_ObjectContactCallback(callback);
    return pStaticShell;
}

CPHLeaderGeomShell::CPHLeaderGeomShell(IClimableObject* climable) { m_pClimable = climable; }
void CPHLeaderGeomShell::near_callback(CPHObject* obj)
{
    if (obj && obj->CastType() == CPHObject::tpCharacter)
    {
        CPHCharacter* ch = static_cast<CPHCharacter*>(obj);
        ch->SetElevator(m_pClimable);
    }
}
