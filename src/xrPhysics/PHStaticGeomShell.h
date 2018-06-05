#ifndef PH_STATIC_GEOM_SHELL_H
#define PH_STATIC_GEOM_SHELL_H
#include "PHGeometryOwner.h"
#include "PHObject.h"
#include "PHUpdateObject.h"
#include "IPHStaticGeomShell.h"
class CPHStaticGeomShell : public CPHGeometryOwner, public CPHObject, public CPHUpdateObject, public IPHStaticGeomShell
{
#ifdef DEBUG
    virtual IPhysicsShellHolder* ref_object() { return CPHGeometryOwner::PhysicsRefObject(); }
#endif

    void get_spatial_params();
    virtual void EnableObject(CPHObject* obj) { CPHUpdateObject::Activate(); }
    virtual dGeomID dSpacedGeom() { return dSpacedGeometry(); }
    virtual void PhDataUpdate(dReal step);
    virtual void PhTune(dReal step) {}
    virtual void InitContact(dContact* c, bool& do_collide, u16 /*material_idx_1*/, u16 /*material_idx_2*/) {}
    virtual u16 get_elements_number() { return 0; };
    virtual CPHSynchronize* get_element_sync(u16 element) { return NULL; };
public:
    void Activate(const Fmatrix& form);
    void Deactivate();
    CPHStaticGeomShell();
    virtual ~CPHStaticGeomShell(){};
};

// CPHStaticGeomShell* P_BuildStaticGeomShell(CGameObject* obj,ObjectContactCallbackFun* object_contact_callback);
// CPHStaticGeomShell* P_BuildStaticGeomShell(CGameObject* obj,ObjectContactCallbackFun* object_contact_callback,Fobb
// &b);
// void				P_BuildStaticGeomShell(CPHStaticGeomShell* shell,CGameObject* obj,ObjectContactCallbackFun*
// object_contact_callback,Fobb &b);
#endif
