#include "StdAfx.h"
#include "dTriColliderCommon.h"
#include "dTriColliderMath.h"
#include "dTriSphere.h"
#include "dcTriListCollider.h"
#include "xrPhysics/PHWorld.h"
#include "xrCDB/xr_area.h"

IC dReal dcTriListCollider::PointSphereTest(const dReal* center, const dReal radius, const dReal* pt, dReal* norm)
{
    norm[0] = center[0] - pt[0];
    norm[1] = center[1] - pt[1];
    norm[2] = center[2] - pt[2];
    dReal mag = dSqrt(dDOT(norm, norm));
    dReal depth = radius - mag;
    if (depth < 0.f)
        return -1.f;
    if (mag > 0.f)
    {
        norm[0] /= mag;
        norm[1] /= mag;
        norm[2] /= mag;
    }
    else
    {
        norm[0] = 0;
        norm[1] = 1;
        norm[2] = 0;
    }
    return depth;
}

inline dReal dcTriListCollider::FragmentonSphereTest(
    const dReal* center, const dReal radius, const dReal* pt1, const dReal* pt2, dReal* norm)
{
    dVector3 direction = {pt2[0] - pt1[0], pt2[1] - pt1[1], pt2[2] - pt1[2]};
    cast_fv(direction).normalize();
    dReal center_prg = dDOT(center, direction);
    dReal pt1_prg = dDOT(pt1, direction);
    dReal pt2_prg = dDOT(pt2, direction);
    dReal from1_dist = center_prg - pt1_prg;
    if (center_prg < pt1_prg || center_prg > pt2_prg)
        return -1;
    dVector3 line_to_center = {-pt1[0] - direction[0] * from1_dist + center[0],
        -pt1[1] - direction[1] * from1_dist + center[1], -pt1[2] - direction[2] * from1_dist + center[2]};

    float mag = dSqrt(dDOT(line_to_center, line_to_center));
    // dNormalize3(norm);

    dReal depth = radius - mag;
    if (depth < 0.f)
        return -1.f;
    if (mag > 0.f)
    {
        norm[0] = line_to_center[0] / mag;
        norm[1] = line_to_center[1] / mag;
        norm[2] = line_to_center[2] / mag;
    }
    else
    {
        norm[0] = 0;
        norm[1] = 1;
        norm[2] = 0;
    }
    return depth;
}

///////////////////////////////////////////////////////////////////////////////
IC bool dcTriListCollider::FragmentonSphereTest(
    const dReal* center, const dReal radius, const dReal* pt1, const dReal* pt2, dReal* norm, dReal& depth)
{
    dVector3 V = {pt2[0] - pt1[0], pt2[1] - pt1[1], pt2[2] - pt1[2]};
    dVector3 L = {pt1[0] - center[0], pt1[1] - center[1], pt1[2] - center[2]};
    dReal sq_mag_V = dDOT(V, V);
    dReal dot_L_V = dDOT(L, V);
    dReal t = -dot_L_V / sq_mag_V; // t
    if (t < 0.f || t > 1.f)
        return false;
    dVector3 Pc = {pt1[0] + t * V[0], pt1[1] + t * V[1], pt1[2] + t * V[2]};
    dVector3 Dc = {center[0] - Pc[0], center[1] - Pc[1], center[2] - Pc[2]};
    dReal sq_mag_Dc = dDOT(Dc, Dc);
    if (sq_mag_Dc > radius * radius)
        return false;
    // dReal dot_V_Pc=dDOT(V,Pc);
    // if(dDOT(V,pt1)>dot_V_Pc||dDOT(V,pt2)<dot_V_Pc) return false;

    dReal mag = dSqrt(sq_mag_Dc);
    depth = radius - mag;
    if (mag > 0.f)
    {
        norm[0] = Dc[0] / mag;
        norm[1] = Dc[1] / mag;
        norm[2] = Dc[2] / mag;
    }
    else
    {
        norm[0] = 0;
        norm[1] = 1;
        norm[2] = 0;
    }
    return true;
}

IC bool dcTriListCollider::PointSphereTest(
    const dReal* center, const dReal radius, const dReal* pt, dReal* norm, dReal& depth)
{
    norm[0] = center[0] - pt[0];
    norm[1] = center[1] - pt[1];
    norm[2] = center[2] - pt[2];
    dReal smag = dDOT(norm, norm);
    if (smag > radius * radius)
        return false;
    float mag = dSqrt(smag);
    depth = radius - mag;
    if (mag > 0.f)
    {
        norm[0] /= mag;
        norm[1] /= mag;
        norm[2] /= mag;
    }
    else
    {
        norm[0] = 0;
        norm[1] = 1;
        norm[2] = 0;
    }
    return true;
}
/////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
int dcTriListCollider::dSortedTriSphere(const dReal* /**v1/**/, const dReal* /**v2/**/, const dReal* triAx, CDB::TRI* T,
    dReal dist, dxGeom* Sphere, dxGeom* Geometry, int Flags, dContactGeom* Contacts, int skip)
{
    // const dReal* v1=(dReal*)T->verts[1];
    // const dReal* v2=(dReal*)T->verts[2];
    const dReal* SphereCenter = dGeomGetPosition(Sphere);
    const float SphereRadius = dGeomSphereGetRadius(Sphere);

    //	dNormalize3(triAx);
    const dReal* ContactNormal = triAx; //{triAx[0],triAx[1],triAx[2]};
    dVector3 ContactPos = {SphereCenter[0] - triAx[0] * SphereRadius, SphereCenter[1] - triAx[1] * SphereRadius,
        SphereCenter[2] - triAx[2] * SphereRadius};

    float ContactDepth = -dist + SphereRadius;
    if (ContactDepth >= 0)
    {
        Contacts->normal[0] = -ContactNormal[0];
        Contacts->normal[1] = -ContactNormal[1];
        Contacts->normal[2] = -ContactNormal[2];
        Contacts->depth = ContactDepth;
        ////////////////////

        Contacts->pos[0] = ContactPos[0];
        Contacts->pos[1] = ContactPos[1];
        Contacts->pos[2] = ContactPos[2];
        Contacts->g1 = Geometry;
        Contacts->g2 = Sphere;
        ((dxGeomUserData*)dGeomGetData(Sphere))->tri_material = T->material;
        if (dGeomGetUserData(Sphere)->callback)
            dGeomGetUserData(Sphere)->callback(T, Contacts);
        SURFACE(Contacts, 0)->mode = T->material;
        //////////////////////////////////
        return 1;
    }

    return 0;
}

int dcTriListCollider::dTriSphere(const dReal* v0, const dReal* v1, const dReal* v2, Triangle* T, dxGeom* Sphere,
    dxGeom* Geometry, int Flags, dContactGeom* Contacts, int /**skip/**/)
{
    const dVector3& triSideAx0 = T->side0;
    const dVector3& triSideAx1 = T->side1;
    const dVector3& triAx = T->norm;

    // if(!TriPlaneContainPoint(triAx,v0,SphereCenter)) return 0;

    const dReal radius = dGeomSphereGetRadius(Sphere);
    float Depth = -T->dist + radius;
    if (Depth < 0.f)
        return 0;
    const dReal* pos = dGeomGetPosition(Sphere);
    dVector3 ContactNormal;
    if (TriContainPoint(v0, v1, v2, triAx, triSideAx0, triSideAx1, pos))
    {
        ContactNormal[0] = triAx[0];
        ContactNormal[1] = triAx[1];
        ContactNormal[2] = triAx[2];
        // dVector3 ContactPos={pos[0]-triAx[0]* radius,pos[1]-triAx[1]* radius,pos[2]-triAx[2]* radius};
    }
    else
    {
        // VERIFY( g_pGameLevel );
        CDB::TRI* T_array = inl_ph_world().ObjectSpace().GetStaticTris();
        flags8& gl_state = gl_cl_tries_state[I - B];
        if (gl_state.test(fl_engaged_s0) || gl_state.test(fl_engaged_s1) || gl_state.test(fl_engaged_s2))
            return 0;
        if (FragmentonSphereTest(pos, radius, v0, v1, ContactNormal, Depth))
        {
            SideToGlClTriState(T->T->verts[0], T->T->verts[1], T_array);
        }
        else if (FragmentonSphereTest(pos, radius, v1, v2, ContactNormal, Depth))
        {
            SideToGlClTriState(T->T->verts[1], T->T->verts[2], T_array);
        }
        else if (FragmentonSphereTest(pos, radius, v2, v0, ContactNormal, Depth))
        {
            SideToGlClTriState(T->T->verts[2], T->T->verts[0], T_array);
        }
        else
        {
            if (gl_state.test(fl_engaged_v0) || gl_state.test(fl_engaged_v1) || gl_state.test(fl_engaged_v2))
                return 0;
            if (PointSphereTest(pos, radius, v0, ContactNormal, Depth))
            {
                VxToGlClTriState(T->T->verts[0], T_array);
            }
            else if (PointSphereTest(pos, radius, v1, ContactNormal, Depth))
            {
                VxToGlClTriState(T->T->verts[1], T_array);
            }
            else if (PointSphereTest(pos, radius, v2, ContactNormal, Depth))
            {
                VxToGlClTriState(T->T->verts[2], T_array);
            }
            else
                return 0;
        }
    }

    Contacts->normal[0] = -ContactNormal[0];
    Contacts->normal[1] = -ContactNormal[1];
    Contacts->normal[2] = -ContactNormal[2];
    Contacts->depth = Depth;
    ////////////////////

    Contacts->pos[0] = pos[0] - ContactNormal[0] * radius;
    Contacts->pos[1] = pos[1] - ContactNormal[1] * radius;
    Contacts->pos[2] = pos[2] - ContactNormal[2] * radius;
    Contacts->g1 = Geometry;
    Contacts->g2 = Sphere;
    ((dxGeomUserData*)dGeomGetData(Sphere))->tri_material = T->T->material;
    if (dGeomGetUserData(Sphere)->callback)
        dGeomGetUserData(Sphere)->callback(T->T, Contacts);
    SURFACE(Contacts, 0)->mode = T->T->material;
    //////////////////////////////////
    //	++OutTriCount;
    return 1;
}
