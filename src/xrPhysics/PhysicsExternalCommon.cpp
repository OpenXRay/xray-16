#include "StdAfx.h"
#include "PhysicsExternalCommon.h"
#include "ExtendedGeom.h"
#include "MathUtilsOde.h"
bool ContactShotMarkGetEffectPars(dContactGeom* c, dxGeomUserData*& data, float& vel_cret, bool& b_invert_normal)
{
    dBodyID b = dGeomGetBody(c->g1);

    b_invert_normal = false;
    if (!b)
    {
        b = dGeomGetBody(c->g2);
        data = dGeomGetUserData(c->g2);
        b_invert_normal = true;
    }
    else
    {
        data = dGeomGetUserData(c->g1);
    }
    if (!b)
        return false;

    dVector3 vel;
    dMass m;
    dBodyGetMass(b, &m);
    dBodyGetPointVel(b, c->pos[0], c->pos[1], c->pos[2], vel);
    vel_cret = _abs(dDOT(vel, c->normal)) * _sqrt(m.mass);
    return true;
}
