// Do NOT build this file seperately. It is included in dTriList.cpp automatically.
#include "xrCDB/Intersect.hpp"
#include "dTriCollideK.h"
#include "dSortTriPrimitive.h"
#include "xrPhysics/dcylinder/dCylinder.h"
#include "xrPhysics/MathUtils.h"
#ifdef DEBUG
#include "xrPhysics/debug_output.h"
#endif

dcTriListCollider::dcTriListCollider(dxGeom* Geometry)
{
    this->Geometry = Geometry;
    GeomData = (dxTriList*)dGeomGetClassData(Geometry);
}

dcTriListCollider::~dcTriListCollider() {}
int dCollideBP(const dxGeom* o1, const dxGeom* o2, int flags, dContactGeom* contact, int skip); // ODE internal function

//#define CONTACT(Ptr, Stride) ((dContactGeom*) (((byte*)Ptr) + (Stride)))
//#define SURFACE(Ptr, Stride) ((dSurfaceParameters*) (((byte*)Ptr) + (Stride-sizeof(dSurfaceParameters))))

int dcTriListCollider::CollideBox(dxGeom* Box, int Flags, dContactGeom* Contacts, int Stride)
{
    Fvector AABB;
    dVector3 BoxSides;
    dGeomBoxGetLengths(Box, BoxSides);
    dReal* R = const_cast<dReal*>(dGeomGetRotation(Box));
    AABB.x = (dFabs(BoxSides[0] * R[0]) + dFabs(BoxSides[1] * R[1]) + dFabs(BoxSides[2] * R[2])) / 2.f + 10.f * EPS_L;
    AABB.y = (dFabs(BoxSides[0] * R[4]) + dFabs(BoxSides[1] * R[5]) + dFabs(BoxSides[2] * R[6])) / 2.f + 10.f * EPS_L;
    AABB.z = (dFabs(BoxSides[0] * R[8]) + dFabs(BoxSides[1] * R[9]) + dFabs(BoxSides[2] * R[10])) / 2.f + 10.f * EPS_L;
    dBodyID box_body = dGeomGetBody(Box);
    if (box_body)
    {
        const dReal* velocity = dBodyGetLinearVel(box_body);
        AABB.x += dFabs(velocity[0]) * 0.04f;
        AABB.y += dFabs(velocity[1]) * 0.04f;
        AABB.z += dFabs(velocity[2]) * 0.04f;
    }

    BoxTri bt(*this);
    return dSortTriPrimitiveCollide(bt, Box, Geometry, Flags, Contacts, Stride, AABB);
}

int dcTriListCollider::CollideCylinder(dxGeom* Cylinder, int Flags, dContactGeom* Contacts, int Stride)
{
    Fvector AABB;
    dReal CylinderRadius, CylinderLength;

    dGeomCylinderGetParams(Cylinder, &CylinderRadius, &CylinderLength);

    dReal* R = const_cast<dReal*>(dGeomGetRotation(Cylinder));

    AABB.x = REAL(0.5) * dFabs(R[1] * CylinderLength) + (_sqrt(R[0] * R[0] + R[2] * R[2]) * CylinderRadius);

    AABB.y = REAL(0.5) * dFabs(R[5] * CylinderLength) + (_sqrt(R[4] * R[4] + R[6] * R[6]) * CylinderRadius);

    AABB.z = REAL(0.5) * dFabs(R[9] * CylinderLength) + (_sqrt(R[8] * R[8] + R[10] * R[10]) * CylinderRadius);

    const dReal* velocity = dBodyGetLinearVel(dGeomGetBody(Cylinder));
    AABB.x += dFabs(velocity[0]) * 0.04f;
    AABB.y += dFabs(velocity[1]) * 0.04f;
    AABB.z += dFabs(velocity[2]) * 0.04f;

    CylTri ct(*this);
    return dSortTriPrimitiveCollide(ct, Cylinder, Geometry, Flags, Contacts, Stride, AABB);
}

////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
int dcTriListCollider::CollideSphere(dxGeom* Sphere, int Flags, dContactGeom* Contacts, int Stride)
{
    const float SphereRadius = dGeomSphereGetRadius(Sphere);
    Fvector AABB;

    // Make AABB
    AABB.x = SphereRadius;
    AABB.y = SphereRadius;
    AABB.z = SphereRadius;

    const dReal* velocity = dBodyGetLinearVel(dGeomGetBody(Sphere));
    AABB.x += dFabs(velocity[0]) * 0.04f;
    AABB.y += dFabs(velocity[1]) * 0.04f;
    AABB.z += dFabs(velocity[2]) * 0.04f;
    SphereTri st(*this);
    return dSortTriPrimitiveCollide(st, Sphere, Geometry, Flags, Contacts, Stride, AABB);
}
