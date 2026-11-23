#pragma once

#include "PHGeometryOwner.h"
#include "PHObject.h"
#include "PHUpdateObject.h"
#include "IPHStaticGeomShell.h"

class CPHStaticGeomShell : public CPHGeometryOwner, public CPHObject, public CPHUpdateObject, public IPHStaticGeomShell
{
    BodyHandle m_native_body = INVALID_BODY_HANDLE;
#ifdef DEBUG
    virtual IPhysicsShellHolder* ref_object() { return CPHGeometryOwner::PhysicsRefObject(); }
#endif

    void get_spatial_params();
    virtual void EnableObject(CPHObject* obj) { CPHUpdateObject::Activate(); }

    virtual void PhDataUpdate(float step);
    virtual void PhTune(float step) {}
    virtual void InitContact(bool& do_collide, bool bo1, float depth, CPhysicsGeom* my_geom, CPhysicsGeom* oposite_geom, u16 material_idx_1, u16 material_idx_2) {}

    virtual u16 get_elements_number() { return 0; };
    virtual CPHSynchronize* get_element_sync(u16 element) { return nullptr; };

public:
    void Activate(const Fmatrix& form);
    void Deactivate();
    CPHStaticGeomShell();
    virtual ~CPHStaticGeomShell() {};
};
