#include "StdAfx.h"
#include "dTriColliderCommon.h"
#include "xrPhysics/dcylinder/dCylinder.h"
#include "dTriCylinder.h"
#include "xrPhysics/MathUtils.h"
#include "dcTriListCollider.h"
#include "xrPhysics/PHWorld.h"
#include "xrCDB/xr_area.h"

/*(lp+t*lv-cp)^2=r^2 => t1,t2 =>O1=lp+t1*lv;O2=lp+t2*lv */
// O1,O2 seems to be sphere line intersections
bool dcTriListCollider::circleLineIntersection(
    const dReal* cn, const dReal* cp, dReal r, const dReal* lv, const dReal* lp, dReal sign, dVector3 point)
{
    dVector3 LC = {lp[0] - cp[0], lp[1] - cp[1], lp[2] - cp[2]};

    dReal A, B, C, B_A, B_A_2, D;
    dReal t1, t2;
    A = dDOT(lv, lv);
    B = dDOT(LC, lv);
    C = dDOT(LC, LC) - r * r;
    B_A = B / A;
    B_A_2 = B_A * B_A;
    D = B_A_2 - C;
    if (D < 0.f)
    {
        point[0] = lp[0] - lv[0] * B;
        point[1] = lp[1] - lv[1] * B;
        point[2] = lp[2] - lv[2] * B;
        return false;
    }
    else
    {
        t1 = -B_A - _sqrt(D);
        t2 = -B_A + _sqrt(D);

        dVector3 O1 = {lp[0] + lv[0] * t1, lp[1] + lv[1] * t1, lp[2] + lv[2] * t1};
        dVector3 O2 = {lp[0] + lv[0] * t2, lp[1] + lv[1] * t2, lp[2] + lv[2] * t2};
        // dVector3 test1={O1[0]-cp[0],O1[1]-cp[1],O1[2]-cp[2]};
        // dVector3 test2={O2[0]-cp[0],O2[1]-cp[1],O2[2]-cp[2]};
        // dReal t=_sqrt(dDOT(test1,test1));
        // t=_sqrt(dDOT(test2,test2));

        dReal cpPr = sign * dDOT(cn, cp);

        dReal dist1 = (sign * dDOT41(cn, O1) - cpPr);
        dReal dist2 = (sign * dDOT41(cn, O2) - cpPr);

        if (dist1 < dist2)
        {
            point[0] = O1[0];
            point[1] = O1[1];
            point[2] = O1[2];
        }
        else
        {
            point[0] = O2[0];
            point[1] = O2[1];
            point[2] = O2[2];
        }

        return true;
    }
}

int dcTriListCollider::dSortedTriCyl(const dReal* triSideAx0, const dReal* triSideAx1, const dReal* triAx,
    // const dReal* v0,
    // const dReal* v1,
    // const dReal* v2,
    CDB::TRI* T, dReal dist, dxGeom* o1, dxGeom* o2, int flags, dContactGeom* contact, int skip)
{
    VERIFY(dGeomGetClass(o1) == dCylinderClassUser);

    const dReal* R = dGeomGetRotation(o1);
    const dReal* p = dGeomGetPosition(o1);
    dReal radius;
    dReal hlz;
    dGeomCylinderGetParams(o1, &radius, &hlz);
    hlz /= 2.f;

    // find number of contacts requested
    int maxc = flags & NUMC_MASK;
    if (maxc < 1)
        maxc = 1;
    if (maxc > 3)
        maxc = 3; // no more than 3 contacts per box allowed

    dReal signum, outDepth, cos1, sin1;
    ////////////////////////////////////////////////////////////////////////////
    // sepparation along tri plane normal;///////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////

    // cos0=dDOT14(triAx,R+0);
    cos1 = dFabs(dDOT14(triAx, R + 1));
    // cos2=dDOT14(triAx,R+2);

    // sin1=_sqrt(cos0*cos0+cos2*cos2);

    ////////////////////////
    // another way //////////
    cos1 = cos1 < REAL(1.) ? cos1 : REAL(1.); // cos1 may slightly exeed 1.f
    sin1 = _sqrt(REAL(1.) - cos1 * cos1);
    //////////////////////////////

    dReal sidePr = cos1 * hlz + sin1 * radius;

    if (dist > 0.f)
        return 0;
    dReal depth = sidePr - dist;
    outDepth = depth;
    signum = -1.f;

    int code = 0;
    if (depth < 0.f)
        return 0;

    dVector3 norm;
    unsigned int ret = 0;
    dVector3 pos;
    if (code == 0)
    {
        norm[0] = triAx[0] * signum;
        norm[1] = triAx[1] * signum;
        norm[2] = triAx[2] * signum;

        dReal Q1 = signum * dDOT14(triAx, R + 0);
        dReal Q2 = signum * dDOT14(triAx, R + 1);
        dReal Q3 = signum * dDOT14(triAx, R + 2);
        dReal factor = _sqrt(Q1 * Q1 + Q3 * Q3);
        dReal C1, C3;
        dReal centerDepth; // depth in the cirle centre
        if (factor > 0.f)
        {
            C1 = Q1 / factor;
            C3 = Q3 / factor;
        }
        else
        {
            C1 = 1.f;
            C3 = 0.f;
        }

        dReal A1 = radius * C1; // cosinus
        dReal A2 = hlz * Q2;
        dReal A3 = radius * C3; // sinus

        if (factor > 0.f)
            centerDepth = outDepth - A1 * Q1 - A3 * Q3;
        else
            centerDepth = outDepth;

        pos[0] = p[0];
        pos[1] = p[1];
        pos[2] = p[2];

        pos[0] += A2 > 0 ? hlz * R[1] : -hlz * R[1];
        pos[1] += A2 > 0 ? hlz * R[5] : -hlz * R[5];
        pos[2] += A2 > 0 ? hlz * R[9] : -hlz * R[9];

        ret = 0;
        contact->pos[0] = pos[0] + A1 * R[0] + A3 * R[2];
        contact->pos[1] = pos[1] + A1 * R[4] + A3 * R[6];
        contact->pos[2] = pos[2] + A1 * R[8] + A3 * R[10];

        {
            contact->depth = outDepth;
            ret = 1;
        }

        if (dFabs(Q2) > M_SQRT1_2)
        {
            A1 = (-C1 * M_COS_PI_3 - C3 * M_SIN_PI_3) * radius;
            A3 = (-C3 * M_COS_PI_3 + C1 * M_SIN_PI_3) * radius;
            CONTACT(contact, ret * skip)->pos[0] = pos[0] + A1 * R[0] + A3 * R[2];
            CONTACT(contact, ret * skip)->pos[1] = pos[1] + A1 * R[4] + A3 * R[6];
            CONTACT(contact, ret * skip)->pos[2] = pos[2] + A1 * R[8] + A3 * R[10];
            CONTACT(contact, ret * skip)->depth = centerDepth + Q1 * A1 + Q3 * A3;

            if (CONTACT(contact, ret * skip)->depth > 0.f)
                ++ret;

            A1 = (-C1 * M_COS_PI_3 + C3 * M_SIN_PI_3) * radius;
            A3 = (-C3 * M_COS_PI_3 - C1 * M_SIN_PI_3) * radius;
            CONTACT(contact, ret * skip)->pos[0] = pos[0] + A1 * R[0] + A3 * R[2];
            CONTACT(contact, ret * skip)->pos[1] = pos[1] + A1 * R[4] + A3 * R[6];
            CONTACT(contact, ret * skip)->pos[2] = pos[2] + A1 * R[8] + A3 * R[10];
            CONTACT(contact, ret * skip)->depth = centerDepth + Q1 * A1 + Q3 * A3;

            if (CONTACT(contact, ret * skip)->depth > 0.f)
                ++ret;
        }
        else
        {
            CONTACT(contact, ret * skip)->pos[0] = contact->pos[0] - 2.f * (A2 > 0 ? hlz * R[1] : -hlz * R[1]);
            CONTACT(contact, ret * skip)->pos[1] = contact->pos[1] - 2.f * (A2 > 0 ? hlz * R[5] : -hlz * R[5]);
            CONTACT(contact, ret * skip)->pos[2] = contact->pos[2] - 2.f * (A2 > 0 ? hlz * R[9] : -hlz * R[9]);
            CONTACT(contact, ret * skip)->depth = outDepth - Q2 * 2.f * A2;

            if (CONTACT(contact, ret * skip)->depth > 0.f)
                ++ret;
        }
    }

    if ((int)ret > maxc)
        ret = (unsigned int)maxc;

    for (unsigned int i = 0; i < ret; ++i)
    {
        CONTACT(contact, i * skip)->g1 = const_cast<dxGeom*>(o2);
        CONTACT(contact, i * skip)->g2 = const_cast<dxGeom*>(o1);
        CONTACT(contact, i * skip)->normal[0] = norm[0];
        CONTACT(contact, i * skip)->normal[1] = norm[1];
        CONTACT(contact, i * skip)->normal[2] = norm[2];
        SURFACE(contact, i * skip)->mode = T->material;
    }
    if (ret && dGeomGetUserData(o1)->callback)
        dGeomGetUserData(o1)->callback(T, contact);
    return ret;
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

IC bool dcTriListCollider::cylinderCrossesLine(
    const dReal* p, const dReal* R, dReal hlz, const dReal* v0, const dReal* v1, const dReal* l, dVector3 pos)
{
    dReal _cos = dDOT14(l, R);

    if (!(dFabs(_cos) < 1.f))
        return false;

    dReal sin2 = 1.f - _cos * _cos;

    dVector3 vp = {v0[0] - p[0], v0[1] - p[1], v0[2] - p[2]};
    dReal c1 = dDOT(vp, l);
    dReal c2 = dDOT14(vp, R);

    dReal t = (c2 * _cos - c1) / sin2;
    dReal q = (c2 - c1 * _cos) / sin2;

    if (dFabs(q) > hlz)
        return false;

    dVector3 v01 = {v1[0] - v0[0], v1[1] - v0[1], v1[2] - v0[2]};
    dReal sidelength2 = dDOT(v01, v01);

    if (t * t > sidelength2)
        return false;

    pos[0] = v0[0] + l[0] * t;
    pos[1] = v0[1] + l[1] * t;
    pos[2] = v0[2] + l[2] * t;

    return true;
}
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
//#define CHECK_EXIT
#ifdef CHECK_EXIT
int Check(int check) { return check; }
#define RETURN0 return Check(0)
#else
#define RETURN0 return 0
#endif

int dcTriListCollider::dTriCyl(const dReal* v0, const dReal* v1, const dReal* v2, Triangle* T, dxGeom* o1, dxGeom* o2,
    int flags, dContactGeom* contact, int skip)
{
    // VERIFY (skip >= (int)sizeof(dContactGeom));
    VERIFY(dGeomGetClass(o1) == dCylinderClassUser);

    const dReal* R = dGeomGetRotation(o1);
    const dReal* p = dGeomGetPosition(o1);
    dReal radius;
    dReal hlz;
    dGeomCylinderGetParams(o1, &radius, &hlz);
    hlz /= 2.f;

    // find number of contacts requested
    int maxc = flags & NUMC_MASK;
    if (maxc < 1)
        maxc = 1;
    if (maxc > 3)
        maxc = 3; // no more than 3 contacts per box allowed

    const dVector3& triAx = T->norm;
    dVector3 triSideAx0 = {T->side0[0], T->side0[1], T->side0[2]}; //{v1[0]-v0[0],v1[1]-v0[1],v1[2]-v0[2]};
    dVector3 triSideAx1 = {T->side1[0], T->side1[1], T->side1[2]}; //{v2[0]-v1[0],v2[1]-v1[1],v2[2]-v1[2]};
    dVector3 triSideAx2 = {v0[0] - v2[0], v0[1] - v2[1], v0[2] - v2[2]};
    // dCROSS(triAx,=,triSideAx0,triSideAx1);
    int code = 0;
    dReal signum, outDepth, cos0, cos1, cos2, sin1;
    ////////////////////////////////////////////////////////////////////////////
    // sepparation along tri plane normal;///////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    // accurate_normalize(triAx);

    // cos0=dDOT14(triAx,R+0);
    cos1 = dFabs(dDOT14(triAx, R + 1));
    // cos2=dDOT14(triAx,R+2);

    // sin1=_sqrt(cos0*cos0+cos2*cos2);

    ////////////////////////
    // another way //////////
    cos1 = cos1 < REAL(1.) ? cos1 : REAL(1.); // cos1 may slightly exeed 1.f
    sin1 = _sqrt(REAL(1.) - cos1 * cos1);
    //////////////////////////////

    dReal sidePr = cos1 * hlz + sin1 * radius;

    dReal dist = -T->dist; // dDOT(triAx,v0)-dDOT(triAx,p);
    if (dist > 0.f)
        RETURN0;
    dReal depth = sidePr - dFabs(dist);
    outDepth = depth;
    signum = dist > 0.f ? 1.f : -1.f;

    code = 0;
    if (depth < 0.f)
        RETURN0;

    dReal depth0, depth1, depth2, dist0, dist1, dist2;
    bool isPdist0, isPdist1, isPdist2;
    bool testV0, testV1, testV2;
    bool sideTestV00, sideTestV01, sideTestV02;
    bool sideTestV10, sideTestV11, sideTestV12;
    bool sideTestV20, sideTestV21, sideTestV22;

    //////////////////////////////////////////////////////////////////////////////
    // cylinder axis - one of the triangle vertexes touches cylinder's flat surface
    //////////////////////////////////////////////////////////////////////////////
    dist0 = dDOT14(v0, R + 1) - dDOT14(p, R + 1);
    dist1 = dDOT14(v1, R + 1) - dDOT14(p, R + 1);
    dist2 = dDOT14(v2, R + 1) - dDOT14(p, R + 1);

    isPdist0 = dist0 > 0.f;
    isPdist1 = dist1 > 0.f;
    isPdist2 = dist2 > 0.f;

    depth0 = hlz - dFabs(dist0);
    depth1 = hlz - dFabs(dist1);
    depth2 = hlz - dFabs(dist2);

    testV0 = depth0 > 0.f;
    testV1 = depth1 > 0.f;
    testV2 = depth2 > 0.f;

    if (isPdist0 == isPdist1 &&
        isPdist1 == isPdist2) //(here and lower) check the tryangle is on one side of the cylinder

    {
        if (depth0 > depth1)
            if (depth0 > depth2)
                if (testV0)
                {
                    if (depth0 < outDepth)
                    {
                        signum = isPdist0 ? 1.f : -1.f;
                        outDepth = depth0;
                        code = 1;
                    }
                }
                else
                    RETURN0;
            else if (testV2)
            {
                if (depth2 < outDepth)
                {
                    outDepth = depth2;
                    signum = isPdist2 ? 1.f : -1.f;
                    code = 3;
                }
            }
            else
                RETURN0;
        else if (depth1 > depth2)
            if (testV1)
            {
                if (depth1 < outDepth)
                {
                    outDepth = depth1;
                    signum = isPdist1 ? 1.f : -1.f;
                    code = 2;
                }
            }
            else
                RETURN0;

        else if (testV2)
        {
            if (depth2 < outDepth)
            {
                outDepth = depth2;
                signum = isPdist2 ? 1.f : -1.f;
                code = 2;
            }
        }
        else
            RETURN0;
    }

    dVector3 axis, outAx;
    dReal posProj;
    dReal pointDepth = 0.f;

#define TEST(vx, ox1, ox2, c)                                             \
    {                                                                     \
        posProj = dDOT14(v##vx, R + 1) - dDOT14(p, R + 1);                \
                                                                          \
        axis[0] = v##vx[0] - p[0] - R[1] * posProj;                       \
        axis[1] = v##vx[1] - p[1] - R[5] * posProj;                       \
        axis[2] = v##vx[2] - p[2] - R[9] * posProj;                       \
                                                                          \
        accurate_normalize(axis);                                         \
                                                                          \
        dist0 = dDOT(v0, axis) - dDOT(p, axis);                           \
        dist1 = dDOT(v1, axis) - dDOT(p, axis);                           \
        dist2 = dDOT(v2, axis) - dDOT(p, axis);                           \
                                                                          \
        isPdist0 = dist0 > 0.f;                                           \
        isPdist1 = dist1 > 0.f;                                           \
        isPdist2 = dist2 > 0.f;                                           \
                                                                          \
        depth0 = radius - dFabs(dist0);                                   \
        depth1 = radius - dFabs(dist1);                                   \
        depth2 = radius - dFabs(dist2);                                   \
                                                                          \
        sideTestV##vx##0 = depth0 > 0.f;                                  \
        sideTestV##vx##1 = depth1 > 0.f;                                  \
        sideTestV##vx##2 = depth2 > 0.f;                                  \
                                                                          \
        if (isPdist0 == isPdist1 && isPdist1 == isPdist2)                 \
                                                                          \
        {                                                                 \
            if (sideTestV##vx##0 || sideTestV##vx##1 || sideTestV##vx##2) \
            {                                                             \
                if (!(depth##vx < depth##ox1 || depth##vx < depth##ox2))  \
                {                                                         \
                    if (depth##vx < outDepth && depth##vx > pointDepth)   \
                    {                                                     \
                        pointDepth = depth##vx;                           \
                        signum = isPdist##vx ? 1.f : -1.f;                \
                        outAx[0] = axis[0];                               \
                        outAx[1] = axis[1];                               \
                        outAx[2] = axis[2];                               \
                        code = c;                                         \
                    }                                                     \
                }                                                         \
            }                                                             \
            else                                                          \
                RETURN0;                                                  \
        }                                                                 \
    \
}

    if (testV0)
        TEST(0, 1, 2, 4)
    if (testV1)
        TEST(1, 2, 0, 5)
    //&& sideTestV01
    if (testV2)
        TEST(2, 0, 1, 6)
//&& sideTestV02 && sideTestV12
#undef TEST

    dVector3 tpos, pos;
    if (code > 3)
        outDepth = pointDepth; // deepest vertex axis used if its depth less than outDepth
    // else{
    // bool outV0=!(testV0&&sideTestV00&&sideTestV10&&sideTestV20);
    // bool outV1=!(testV1&&sideTestV01&&sideTestV11&&sideTestV21);
    // bool outV2=!(testV2&&sideTestV02&&sideTestV12&&sideTestV22);
    bool outV0 = true;
    bool outV1 = true;
    bool outV2 = true;
/////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
/// crosses between triangle sides and cylinder axis//////////////////////////
/////////////////////////////////////////////////////////////////////////////
#define TEST(ax, nx, ox, c)                                                    \
    if (cylinderCrossesLine(p, R + 1, hlz, v##ax, v##nx, triSideAx##ax, tpos)) \
    {                                                                          \
        dCROSS114(axis, =, triSideAx##ax, R + 1);                              \
        accurate_normalize(axis);                                              \
        dist##ax = dDOT(v##ax, axis) - dDOT(p, axis);                          \
        dist##ox = dDOT(v##ox, axis) - dDOT(p, axis);                          \
                                                                               \
        isPdist##ax = dist##ax > 0.f;                                          \
        isPdist##ox = dist##ox > 0.f;                                          \
                                                                               \
        if (isPdist##ax == isPdist##ox)                                        \
        \
{                                                                   \
            \
depth##ax = radius - dFabs(dist##ax);                                          \
            \
depth##ox = radius - dFabs(dist##ox);                                          \
                                                                               \
            if (depth##ax > 0.f)                                               \
            {                                                                  \
                if (depth##ax <= outDepth && depth##ax >= depth##ox)           \
                {                                                              \
                    outDepth = depth##ax;                                      \
                    signum = isPdist##ax ? 1.f : -1.f;                         \
                    outAx[0] = axis[0];                                        \
                    outAx[1] = axis[1];                                        \
                    outAx[2] = axis[2];                                        \
                    pos[0] = tpos[0];                                          \
                    pos[1] = tpos[1];                                          \
                    pos[2] = tpos[2];                                          \
                    code = c;                                                  \
                }                                                              \
            }                                                                  \
            else if (depth##ox < 0.f)                                          \
                RETURN0;                                                       \
        \
\
}                                                                \
    \
}

    accurate_normalize(triSideAx0);
    if (outV0 && outV1)
        TEST(0, 1, 2, 7)

    accurate_normalize(triSideAx1);
    if (outV1 && outV2)
        TEST(1, 2, 0, 8)

    accurate_normalize(triSideAx2);
    if (outV2 && outV0)
        TEST(2, 0, 1, 9)
#undef TEST

    ////////////////////////////////////
    // test cylinder rings on triangle sides////
    ////////////////////////////////////

    dVector3 tAx, cen;
    dReal sign;
    bool cs;

#define TEST(ax, nx, ox, c)                                                         \
    \
{                                                                            \
        \
posProj = dDOT(p, triSideAx##ax) - dDOT(v##ax, triSideAx##ax);                      \
        \
axis[0] = p[0] - v0[0] - triSideAx##ax[0] * posProj;                                \
        \
axis[1] = p[1] - v0[1] - triSideAx##ax[1] * posProj;                                \
        \
axis[2] = p[2] - v0[2] - triSideAx##ax[2] * posProj;                                \
        \
sign = dDOT14(axis, R + 1) > 0.f ? 1.f : -1.f;                                      \
        \
cen[0] = p[0] - sign * R[1] * hlz;                                                  \
        \
cen[1] = p[1] - sign * R[5] * hlz;                                                  \
        \
cen[2] = p[2] - sign * R[9] * hlz;                                                  \
        \
\
cs = circleLineIntersection(R + 1, cen, radius, triSideAx##ax, v##ax, -sign, tpos); \
        \
\
axis[0] = tpos[0] - cen[0];                                                         \
        \
axis[1] = tpos[1] - cen[1];                                                         \
        \
axis[2] = tpos[2] - cen[2];                                                         \
        \
\
if(cs)                                                                              \
        {                                                                           \
            \
\
cos0 = dDOT14(axis, R + 0);                                                         \
            \
cos2 = dDOT14(axis, R + 2);                                                         \
            \
tAx[0] = R[2] * cos0 - R[0] * cos2;                                                 \
            \
tAx[1] = R[6] * cos0 - R[4] * cos2;                                                 \
            \
tAx[2] = R[10] * cos0 - R[8] * cos2;                                                \
            \
\
dCROSS(axis, =, triSideAx##ax, tAx);                                                \
        \
\
}                                                                     \
        \
accurate_normalize(axis);                                                           \
        \
dist##ax = dDOT(v##ax, axis) - dDOT(p, axis);                                       \
        \
if(dist##ax* dDOT(axis, triSideAx##nx) > 0.f)                                       \
        {                                                                           \
            \
\
cos0 = dDOT14(axis, R + 0);                                                         \
            \
cos1 = dFabs(dDOT14(axis, R + 1));                                                  \
            \
cos2 = dDOT14(axis, R + 2);                                                         \
            \
\
\
sin1 = _sqrt(cos0 * cos0 + cos2 * cos2);                                            \
            \
\
sidePr = cos1 * hlz + sin1 * radius;                                                \
                                                                                    \
            dist##ox = dDOT(v##ox, axis) - dDOT(p, axis);                           \
                                                                                    \
            isPdist##ax = dist##ax > 0.f;                                           \
            isPdist##ox = dist##ox > 0.f;                                           \
                                                                                    \
            if (isPdist##ax == isPdist##ox)                                         \
            \
\
{                                                                 \
                \
depth##ax = sidePr - dFabs(dist##ax);                                               \
                \
depth##ox = sidePr - dFabs(dist##ox);                                               \
                                                                                    \
                if (depth##ax > 0.f)                                                \
                {                                                                   \
                    if (depth##ax < outDepth)                                       \
                    {                                                               \
                        outDepth = depth##ax;                                       \
                        signum = isPdist##ax ? 1.f : -1.f;                          \
                        outAx[0] = axis[0];                                         \
                        outAx[1] = axis[1];                                         \
                        outAx[2] = axis[2];                                         \
                        pos[0] = tpos[0];                                           \
                        pos[1] = tpos[1];                                           \
                        pos[2] = tpos[2];                                           \
                        code = c;                                                   \
                    }                                                               \
                }                                                                   \
                else if (depth##ox < 0.f)                                           \
                    RETURN0;                                                        \
            \
\
\
}                                                              \
        \
}                                                                        \
    \
}

    if (7 != code)
        TEST(0, 1, 2, 10)

    if (8 != code)
        TEST(1, 2, 0, 11)

    if (9 != code)
        TEST(2, 0, 1, 12)

#undef TEST

    //}
    //////////////////////////////////////////////////////////////////////
    /// if we get to this poit tri touches cylinder///////////////////////
    /////////////////////////////////////////////////////////////////////
    // VERIFY( g_pGameLevel );
    CDB::TRI* T_array = inl_ph_world().ObjectSpace().GetStaticTris();
    dVector3 norm;
    unsigned int ret;
    flags8& gl_state = gl_cl_tries_state[I - B];
    if (code == 0)
    {
        norm[0] = triAx[0] * signum;
        norm[1] = triAx[1] * signum;
        norm[2] = triAx[2] * signum;

        dReal Q1 = dDOT14(norm, R + 0);
        dReal Q2 = dDOT14(norm, R + 1);
        dReal Q3 = dDOT14(norm, R + 2);
        dReal factor = _sqrt(Q1 * Q1 + Q3 * Q3);
        dReal C1, C3;
        dReal centerDepth; // depth in the cirle centre
        if (factor > 0.f)
        {
            C1 = Q1 / factor;
            C3 = Q3 / factor;
        }
        else
        {
            C1 = 1.f;
            C3 = 0.f;
        }

        dReal A1 = radius * C1; // cosinus
        dReal A2 = hlz; // Q2
        dReal A3 = radius * C3; // sinus

        if (factor > 0.f)
            centerDepth = outDepth - A1 * Q1 - A3 * Q3;
        else
            centerDepth = outDepth;

        pos[0] = p[0];
        pos[1] = p[1];
        pos[2] = p[2];

        pos[0] += Q2 > 0 ? hlz * R[1] : -hlz * R[1];
        pos[1] += Q2 > 0 ? hlz * R[5] : -hlz * R[5];
        pos[2] += Q2 > 0 ? hlz * R[9] : -hlz * R[9];

        ret = 0;
        dVector3 cross0, cross1, cross2;
        dReal ds0, ds1, ds2;

        dCROSS(cross0, =, triAx, triSideAx0);
        ds0 = dDOT(cross0, v0);

        dCROSS(cross1, =, triAx, triSideAx1);
        ds1 = dDOT(cross1, v1);

        dCROSS(cross2, =, triAx, triSideAx2);
        ds2 = dDOT(cross2, v2);

        contact->pos[0] = pos[0] + A1 * R[0] + A3 * R[2];
        contact->pos[1] = pos[1] + A1 * R[4] + A3 * R[6];
        contact->pos[2] = pos[2] + A1 * R[8] + A3 * R[10];

        if (dDOT(cross0, contact->pos) - ds0 > 0.f && dDOT(cross1, contact->pos) - ds1 > 0.f &&
            dDOT(cross2, contact->pos) - ds2 > 0.f)
        {
            contact->depth = outDepth;
            ret = 1;
        }

        if (dFabs(Q2) > M_SQRT1_2)
        {
            A1 = (-C1 * M_COS_PI_3 - C3 * M_SIN_PI_3) * radius;
            A3 = (-C3 * M_COS_PI_3 + C1 * M_SIN_PI_3) * radius;
            CONTACT(contact, ret * skip)->pos[0] = pos[0] + A1 * R[0] + A3 * R[2];
            CONTACT(contact, ret * skip)->pos[1] = pos[1] + A1 * R[4] + A3 * R[6];
            CONTACT(contact, ret * skip)->pos[2] = pos[2] + A1 * R[8] + A3 * R[10];
            CONTACT(contact, ret * skip)->depth = centerDepth + Q1 * A1 + Q3 * A3;

            if (CONTACT(contact, ret * skip)->depth > 0.f)
                if (dDOT(cross0, CONTACT(contact, ret * skip)->pos) - ds0 > 0.f &&
                    dDOT(cross1, CONTACT(contact, ret * skip)->pos) - ds1 > 0.f &&
                    dDOT(cross2, CONTACT(contact, ret * skip)->pos) - ds2 > 0.f)
                    ++ret;

            A1 = (-C1 * M_COS_PI_3 + C3 * M_SIN_PI_3) * radius;
            A3 = (-C3 * M_COS_PI_3 - C1 * M_SIN_PI_3) * radius;
            CONTACT(contact, ret * skip)->pos[0] = pos[0] + A1 * R[0] + A3 * R[2];
            CONTACT(contact, ret * skip)->pos[1] = pos[1] + A1 * R[4] + A3 * R[6];
            CONTACT(contact, ret * skip)->pos[2] = pos[2] + A1 * R[8] + A3 * R[10];
            CONTACT(contact, ret * skip)->depth = centerDepth + Q1 * A1 + Q3 * A3;

            if (CONTACT(contact, ret * skip)->depth > 0.f)
                if (dDOT(cross0, CONTACT(contact, ret * skip)->pos) - ds0 > 0.f &&
                    dDOT(cross1, CONTACT(contact, ret * skip)->pos) - ds1 > 0.f &&
                    dDOT(cross2, CONTACT(contact, ret * skip)->pos) - ds2 > 0.f)
                    ++ret;
        }
        else
        {
            CONTACT(contact, ret * skip)->pos[0] = contact->pos[0] - 2.f * (Q2 > 0 ? hlz * R[1] : -hlz * R[1]);
            CONTACT(contact, ret * skip)->pos[1] = contact->pos[1] - 2.f * (Q2 > 0 ? hlz * R[5] : -hlz * R[5]);
            CONTACT(contact, ret * skip)->pos[2] = contact->pos[2] - 2.f * (Q2 > 0 ? hlz * R[9] : -hlz * R[9]);
            CONTACT(contact, ret * skip)->depth = outDepth - dFabs(Q2 * 2.f * A2);

            if (CONTACT(contact, ret * skip)->depth > 0.f)
                if (dDOT(cross0, CONTACT(contact, ret * skip)->pos) - ds0 > 0.f &&
                    dDOT(cross1, CONTACT(contact, ret * skip)->pos) - ds1 > 0.f &&
                    dDOT(cross2, CONTACT(contact, ret * skip)->pos) - ds2 > 0.f)
                    ++ret;
        }
    }
    else if (code < 7) // 1-6
    {
        ret = 1;
        contact->depth = outDepth;
        switch ((code - 1) % 3)
        {
        case 0:
            if (gl_state.test(fl_engaged_v0))
                RETURN0;
            VxToGlClTriState(T->T->verts[0], T_array);
            contact->pos[0] = v0[0];
            contact->pos[1] = v0[1];
            contact->pos[2] = v0[2];
            break;
        case 1:
            if (gl_state.test(fl_engaged_v1))
                RETURN0;
            VxToGlClTriState(T->T->verts[1], T_array);
            contact->pos[0] = v1[0];
            contact->pos[1] = v1[1];
            contact->pos[2] = v1[2];
            break;
        case 2:
            if (gl_state.test(fl_engaged_v2))
                RETURN0;
            VxToGlClTriState(T->T->verts[2], T_array);
            contact->pos[0] = v2[0];
            contact->pos[1] = v2[1];
            contact->pos[2] = v2[2];
            break;
        }

        if (code < 4)
        { // 1-3
            norm[0] = R[1] * signum;
            norm[1] = R[5] * signum;
            norm[2] = R[9] * signum;
        }
        else
        {
            norm[0] = outAx[0] * signum;
            norm[1] = outAx[1] * signum;
            norm[2] = outAx[2] * signum;
        }
    }

    else
    { // 7-12
        ret = 1;
        int iv0 = (code - 7) % 3;
        int iv1 = (iv0 + 1) % 3;
        int flag = fl_engaged_s0 << (iv0);
        if (gl_state.test(u8(flag & 0xff)))
            RETURN0;
        SideToGlClTriState(T->T->verts[iv0], T->T->verts[iv1], T_array);
        contact->depth = outDepth;
        norm[0] = outAx[0] * signum;
        norm[1] = outAx[1] * signum;
        norm[2] = outAx[2] * signum;
        contact->pos[0] = pos[0];
        contact->pos[1] = pos[1];
        contact->pos[2] = pos[2];
    }

    if ((int)ret > maxc)
        ret = (unsigned int)maxc;

    for (unsigned int i = 0; i < ret; ++i)
    {
        CONTACT(contact, i * skip)->g1 = const_cast<dxGeom*>(o2);
        CONTACT(contact, i * skip)->g2 = const_cast<dxGeom*>(o1);
        CONTACT(contact, i * skip)->normal[0] = norm[0];
        CONTACT(contact, i * skip)->normal[1] = norm[1];
        CONTACT(contact, i * skip)->normal[2] = norm[2];
        SURFACE(contact, i * skip)->mode = T->T->material;
    }
    if (ret && dGeomGetUserData(o1)->callback)
        dGeomGetUserData(o1)->callback(T->T, contact);
    return ret;
}
