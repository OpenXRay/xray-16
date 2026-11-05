#include "StdAfx.h"
#include "PHStaticGeomShell.h"

#include "IPhysicsShellHolder.h"
#include "PHCharacter.h"
#include "IClimableObject.h"

#include "Include/xrRender/Kinematics.h"
#include "PHCollideValidator.h"
#include "xrEngine/xr_object.h"
#include "xrCore/Animation/Bone.hpp"
#include "xrPhysicsCore/IPhysicsCore.h"

void CPHStaticGeomShell::get_spatial_params()
{
    if (!m_geoms.empty() && m_geoms.front()->get_body() != INVALID_CHARACTER_VIRTUAL_HANDLE)
    {
        Fvector extents;
        GetPhysicsCore()->GetBodyAABB(m_geoms.front()->get_body(), spatial.sphere.P, extents);

        spatial.sphere.R = extents.magnitude();
        AABB.set(extents);
    }
    else
    {
        spatial.sphere.P.set(0.f, 0.f, 0.f);
        spatial.sphere.R = EPS_L;
        AABB.set(EPS_L, EPS_L, EPS_L);
    }
}

void CPHStaticGeomShell::PhDataUpdate(float step)
{
    // Jolt Physics handles islands and sleeping states internally. ODE manual unmerge removed.
    PhysicsRefObject()->enable_notificate();
    CPHUpdateObject::Deactivate();
}

void CPHStaticGeomShell::Activate(const Fmatrix& form)
{
    get_mc_data();
    build();
    xr_vector<PhysicsShapeHandle> shapes;
    xr_vector<Fmatrix> transforms;
    for (auto* geometry : m_geoms) {
        shapes.push_back(geometry->geometry());
        Fmatrix local;
        geometry->get_local_form(local);
        local.c.sub(m_mass_center);
        transforms.push_back(local);
    }
    const auto shape = GetPhysicsCore()->CreateCompoundShape(shapes.data(), transforms.data(), shapes.size());
    R_ASSERT(shape);
    m_native_body = GetPhysicsCore()->CreateStaticBody(shape, m_mass_center);
    GetPhysicsCore()->DestroyCDBModel(shape);
    R_ASSERT(m_native_body != INVALID_BODY_HANDLE);
    GetPhysicsCore()->SetBodyObjectLayer(m_native_body, 5);
    GetPhysicsCore()->SetBodyUserData(m_native_body, this);
    set_body(m_native_body);
    SetPhObjectInGeomData(this);
    setStaticForm(form);
    get_spatial_params();
    spatial_register();
}

void CPHStaticGeomShell::Deactivate()
{
    spatial_unregister();
    CPHUpdateObject::Deactivate();
    GetPhysicsCore()->DestroyBody(m_native_body);
    m_native_body = INVALID_BODY_HANDLE;
    set_body(INVALID_BODY_HANDLE);
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
    pUnbrokenObject->set_ObjectContactCallback(object_contact_callback);
    CPHCollideValidator::SetNonDynamicObject(*pUnbrokenObject);
}

CPHStaticGeomShell* P_BuildStaticGeomShell(
    IPhysicsShellHolder* obj, ObjectContactCallbackFun* object_contact_callback, const Fobb& b)
{
    CPHStaticGeomShell* pUnbrokenObject = xr_new<CPHStaticGeomShell>();
    P_BuildStaticGeomShell(pUnbrokenObject, obj, object_contact_callback, b);
    return pUnbrokenObject;
}

IPHStaticGeomShell* P_BuildStaticGeomShell(IPhysicsShellHolder* obj, ObjectContactCallbackFun* object_contact_callback)
{
    Fobb b;
    IKinematics* K = obj->ObjectKinematics();
    R_ASSERT2(K, "need visual to build");
    K->CalculateBones(TRUE);

    K->GetBox().getradius(b.m_halfsize);

    b.xform_set(Fidentity);
    CPHStaticGeomShell* pUnbrokenObject = P_BuildStaticGeomShell(obj, object_contact_callback, b);

    K->CalculateBones(TRUE);
    for (u16 k = 0; k < K->LL_BoneCount(); k++)
    {
        K->LL_GetBoneInstance(k).set_callback(bctPhysics, cb, K->LL_GetBoneInstance(k).callback_param(), TRUE);
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
    UnbrokenObject = nullptr;
}

class CPHLeaderGeomShell : public CPHStaticGeomShell
{
    IClimableObject* m_pClimable;

public:
    CPHLeaderGeomShell(IClimableObject* climable);
    void near_callback(CPHObject* obj);
};

IPHStaticGeomShell* P_BuildLeaderGeomShell(IClimableObject* obj, ObjectContactCallbackFun* callback, const Fobb& b)
{
    CPHLeaderGeomShell* pStaticShell = xr_new<CPHLeaderGeomShell>(obj);
    P_BuildStaticGeomShell(smart_cast<CPHStaticGeomShell*>(pStaticShell), smart_cast<IPhysicsShellHolder*>(obj), nullptr, b);
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
