#pragma once
#include "PhysicsExternalCommon.h"
class IPHStaticGeomShell
{
protected:
#if defined(WINDOWS)
    virtual ~IPHStaticGeomShell() = 0 {}
#elif defined(LINUX)
    virtual ~IPHStaticGeomShell() {}
#endif

    //	virtual void						set_ObjectContactCallback	(ObjectContactCallbackFun* callback);
};

class IPhysicsShellHolder;
class IClimableObject;
XRPHYSICS_API IPHStaticGeomShell* P_BuildStaticGeomShell(
    IPhysicsShellHolder* obj, ObjectContactCallbackFun* object_contact_callback);
XRPHYSICS_API IPHStaticGeomShell* P_BuildLeaderGeomShell(
    IClimableObject* obj, ObjectContactCallbackFun* callback, const Fobb& b);
XRPHYSICS_API void DestroyStaticGeomShell(IPHStaticGeomShell*& p);

// CPHStaticGeomShell* P_BuildStaticGeomShell(CGameObject* obj,ObjectContactCallbackFun* object_contact_callback,Fobb
// &b);
// void				P_BuildStaticGeomShell(CPHStaticGeomShell* shell,CGameObject* obj,ObjectContactCallbackFun*
// object_contact_callback,Fobb &b);
