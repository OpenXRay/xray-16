#include "StdAfx.h"
#include "dcylinder/dCylinder.h"
struct dContactGeom;
int dCollideCylRay(dxGeom* o1, dxGeom* o2, int flags, dContactGeom* contact, int skip);

#pragma warning(push)
#pragma warning(disable : 4995)
#pragma warning(disable : 4267)
#include "ode/ode/src/collision_std.h"
#pragma warning(pop)

struct dxRayMotions
{
    dGeomID ray;
    dGeomID ray_ownwer;
    dxRayMotions()
    {
        ray = 0;
        ray_ownwer = 0;
    }
};

int dRayMotionsClassUser = -1;

#define CONTACT(p, skip) ((dContactGeom*)(((char*)p) + (skip)))

int dCollideRMB(dxGeom* o1, dxGeom* o2, int flags, dContactGeom* contact, int skip)
{
    dxRayMotions* rm = (dxRayMotions*)dGeomGetClassData(o1);
    int ret = dCollideRayBox(rm->ray, o2, flags, contact, skip);
    for (int i = 0; i < ret; i++)
    {
        dContactGeom* c = CONTACT(contact, skip * i);
        c->g1 = rm->ray_ownwer;
        // c->depth*=60.f;
    }
    return ret;
}

int dCollideRMS(dxGeom* o1, dxGeom* o2, int flags, dContactGeom* contact, int skip)
{
    dxRayMotions* rm = (dxRayMotions*)dGeomGetClassData(o1);
    int ret = dCollideRaySphere(rm->ray, o2, flags, contact, skip);
    for (int i = 0; i < ret; i++)
    {
        dContactGeom* c = CONTACT(contact, skip * i);
        c->g1 = rm->ray_ownwer;
        // c->depth*=60.f;
    }
    return ret;
}
inline void revert_contact(dContactGeom* c)
{
    c->normal[0] = -c->normal[0];
    c->normal[1] = -c->normal[1];
    c->normal[2] = -c->normal[2];
    dxGeom* tmp = c->g1;
    c->g1 = c->g2;
    c->g2 = tmp;
}
int dCollideRMCyl(dxGeom* o1, dxGeom* o2, int flags, dContactGeom* contact, int skip)
{
    dxRayMotions* rm = (dxRayMotions*)dGeomGetClassData(o1);
    int ret = dCollideCylRay(o2, rm->ray, flags, contact, skip);
    for (int i = 0; i < ret; i++)
    {
        dContactGeom* c = CONTACT(contact, skip * i);
        revert_contact(c);
        c->g1 = rm->ray_ownwer;
    }
    return ret;
}

static dColliderFn* dRayMotionsColliderFn(int num)
{
    if (num == dBoxClass)
        return (dColliderFn*)&dCollideRMB;
    if (num == dSphereClass)
        return (dColliderFn*)&dCollideRMS;
    if (num == dCylinderClassUser)
        return (dColliderFn*)&dCollideRMCyl;
    return 0;
}

static void dRayMotionsAABB(dxGeom* geom, dReal aabb[6])
{
    dxRayMotions* c = (dxRayMotions*)dGeomGetClassData(geom);
    dGeomGetAABB(c->ray, aabb);
    /// dInfiniteAABB(geom,aabb);
}
void dGeomRayMotionDestroy(dGeomID ray) { dGeomDestroy(((dxRayMotions*)dGeomGetClassData(ray))->ray); }
dxGeom* dCreateRayMotions(dSpaceID space)
{
    if (dRayMotionsClassUser == -1)
    {
        dGeomClass c;
        c.bytes = sizeof(dxRayMotions);
        c.collider = &dRayMotionsColliderFn;
        c.aabb = &dRayMotionsAABB;
        c.aabb_test = 0;
        c.dtor = &dGeomRayMotionDestroy;
        dRayMotionsClassUser = dCreateGeomClass(&c);
    }
    dGeomID g = dCreateGeom(dRayMotionsClassUser);
    if (space)
        dSpaceAdd(space, g);
    dxRayMotions* c = (dxRayMotions*)dGeomGetClassData(g);
    c->ray = dCreateRay(space, REAL(1.));
    return g;
}

void dGeomRayMotionsSet(dGeomID g, const dReal* p, const dReal* d, dReal l)
{
    dxRayMotions* c = (dxRayMotions*)dGeomGetClassData(g);
    dGeomRaySetLength(c->ray, l);
    dGeomRaySet(c->ray, p[0], p[1], p[2], d[0], d[1], d[2]);
    dGeomMoved(g);
}

void dGeomRayMotionSetGeom(dGeomID rm, dGeomID g)
{
    dxRayMotions* c = (dxRayMotions*)dGeomGetClassData(rm);
    c->ray_ownwer = g;
}
