#include "stdafx.h"
#pragma hdrstop

#include "xrCDB.h"

using namespace CDB;
using namespace Opcode;

//! This macro quickly finds the min & max values among 3 variables
#define FINDMINMAX(x0, x1, x2, min, max)\
    min = max = x0;\
    if (x1 < min)\
        min = x1;\
    if (x1 > max)\
        max = x1;\
    if (x2 < min)\
        min = x2;\
    if (x2 > max)\
        max = x2;

//! TO BE DOCUMENTED
ICF bool planeBoxOverlap(const Point& normal, const float d, const Point& maxbox)
{
    Point vmin, vmax;
    for (udword q = 0; q <= 2; q++)
    {
        if (((const float*)normal)[q] > 0.0f)
        {
            ((float*)vmin)[q] = -((const float*)maxbox)[q];
            ((float*)vmax)[q] = ((const float*)maxbox)[q];
        }
        else
        {
            ((float*)vmin)[q] = ((const float*)maxbox)[q];
            ((float*)vmax)[q] = -((const float*)maxbox)[q];
        }
    }
    if ((normal | vmin) + d > 0.0f)
        return false;
    if ((normal | vmax) + d >= 0.0f)
        return true;

    return false;
}

//! TO BE DOCUMENTED
#define AXISTEST_X01(a, b, fa, fb)\
    min = a * v0.y - b * v0.z;\
    max = a * v2.y - b * v2.z;\
    if (min > max)\
    {\
        const float tmp = max;\
        max = min;\
        min = tmp;\
    }\
    rad = fa * extents.y + fb * extents.z;\
    if (min > rad || max < -rad)\
        return false;

//! TO BE DOCUMENTED
#define AXISTEST_X2(a, b, fa, fb)\
    min = a * v0.y - b * v0.z;\
    max = a * v1.y - b * v1.z;\
    if (min > max)\
    {\
        const float tmp = max;\
        max = min;\
        min = tmp;\
    }\
    rad = fa * extents.y + fb * extents.z;\
    if (min > rad || max < -rad)\
        return false;

//! TO BE DOCUMENTED
#define AXISTEST_Y02(a, b, fa, fb)\
    min = b * v0.z - a * v0.x;\
    max = b * v2.z - a * v2.x;\
    if (min > max)\
    {\
        const float tmp = max;\
        max = min;\
        min = tmp;\
    }\
    rad = fa * extents.x + fb * extents.z;\
    if (min > rad || max < -rad)\
        return false;

//! TO BE DOCUMENTED
#define AXISTEST_Y1(a, b, fa, fb)\
    min = b * v0.z - a * v0.x;\
    max = b * v1.z - a * v1.x;\
    if (min > max)\
    {\
        const float tmp = max;\
        max = min;\
        min = tmp;\
    }\
    rad = fa * extents.x + fb * extents.z;\
    if (min > rad || max < -rad)\
        return false;

//! TO BE DOCUMENTED
#define AXISTEST_Z12(a, b, fa, fb)\
    min = a * v1.x - b * v1.y;\
    max = a * v2.x - b * v2.y;\
    if (min > max)\
    {\
        const float tmp = max;\
        max = min;\
        min = tmp;\
    }\
    rad = fa * extents.x + fb * extents.y;\
    if (min > rad || max < -rad)\
        return false;

//! TO BE DOCUMENTED
#define AXISTEST_Z0(a, b, fa, fb)\
    min = a * v0.x - b * v0.y;\
    max = a * v1.x - b * v1.y;\
    if (min > max)\
    {\
        const float tmp = max;\
        max = min;\
        min = tmp;\
    }\
    rad = fa * extents.x + fb * extents.y;\
    if (min > rad || max < -rad)\
        return false;

template <bool bClass3, bool bFirst>
class box_collider
{
public:
    COLLIDER* dest;
    TRI* tris;
    Fvector* verts;

    Fvector b_min, b_max;
    Point center, extents;

    Point mLeafVerts[3];

    IC void _init(COLLIDER* CL, Fvector* V, TRI* T, const Fvector& C, const Fvector& E)
    {
        dest = CL;
        verts = V;
        tris = T;
        center = Point(C.x, C.y, C.z);
        extents = Point(E.x, E.y, E.z);
        b_min.sub(C, E);
        b_max.add(C, E);
    }
    ICF bool _box(const Fvector& C, const Fvector& E)
    {
        if (b_max.x < C.x - E.x)
            return false;
        if (b_max.y < C.y - E.y)
            return false;
        if (b_max.z < C.z - E.z)
            return false;
        if (b_min.x > C.x + E.x)
            return false;
        if (b_min.y > C.y + E.y)
            return false;
        if (b_min.z > C.z + E.z)
            return false;
        return true;
    };
    ICF bool _tri()
    {
        // move everything so that the boxcenter is in (0,0,0)
        Point v0, v1, v2;
        v0.x = mLeafVerts[0].x - center.x;
        v1.x = mLeafVerts[1].x - center.x;
        v2.x = mLeafVerts[2].x - center.x;

        // First, test overlap in the {x,y,z}-directions
        {
            float min, max;
            // Find min, max of the triangle in x-direction, and test for overlap in X
            FINDMINMAX(v0.x, v1.x, v2.x, min, max);
            if (min > extents.x || max < -extents.x)
                return false;
            // Same for Y
            v0.y = mLeafVerts[0].y - center.y;
            v1.y = mLeafVerts[1].y - center.y;
            v2.y = mLeafVerts[2].y - center.y;

            FINDMINMAX(v0.y, v1.y, v2.y, min, max);
            if (min > extents.y || max < -extents.y)
                return false;

            // Same for Z
            v0.z = mLeafVerts[0].z - center.z;
            v1.z = mLeafVerts[1].z - center.z;
            v2.z = mLeafVerts[2].z - center.z;
            FINDMINMAX(v0.z, v1.z, v2.z, min, max);
            if (min > extents.z || max < -extents.z)
                return false;
        }

        // 2) Test if the box intersects the plane of the triangle
        // compute plane equation of triangle: normal*x+d=0
        // ### could be precomputed since we use the same leaf triangle several times
        const Point e0 = v1 - v0;
        const Point e1 = v2 - v1;
        const Point normal = e0 ^ e1;
        const float d = -normal | v0;
        if (!planeBoxOverlap(normal, d, extents))
            return false;

        // 3) "Class III" tests
        if (bClass3)
        {
            float rad;
            float min, max;
            // compute triangle edges
            // - edges lazy evaluated to take advantage of early exits
            // - fabs precomputed (half less work, possible since extents are always >0)
            // - customized macros to take advantage of the null component
            // - axis vector3 discarded, possibly saves useless movs

            const float fey0 = _abs(e0.y);
            const float fez0 = _abs(e0.z);
            AXISTEST_X01(e0.z, e0.y, fez0, fey0);
            const float fex0 = _abs(e0.x);
            AXISTEST_Y02(e0.z, e0.x, fez0, fex0);
            AXISTEST_Z12(e0.y, e0.x, fey0, fex0);

            const float fey1 = _abs(e1.y);
            const float fez1 = _abs(e1.z);
            AXISTEST_X01(e1.z, e1.y, fez1, fey1);
            const float fex1 = _abs(e1.x);
            AXISTEST_Y02(e1.z, e1.x, fez1, fex1);
            AXISTEST_Z0(e1.y, e1.x, fey1, fex1);

            const Point e2 = mLeafVerts[0] - mLeafVerts[2];
            const float fey2 = _abs(e2.y);
            const float fez2 = _abs(e2.z);
            AXISTEST_X2(e2.z, e2.y, fez2, fey2);
            const float fex2 = _abs(e2.x);
            AXISTEST_Y1(e2.z, e2.x, fez2, fex2);
            AXISTEST_Z12(e2.y, e2.x, fey2, fex2);
        }
        return true;
    }
    void _prim(u32 prim)
    {
        TRI& T = tris[prim];
        Fvector& v0 = verts[T.verts[0]];
        mLeafVerts[0].x = v0.x;
        mLeafVerts[0].y = v0.y;
        mLeafVerts[0].z = v0.z;
        Fvector& v1 = verts[T.verts[1]];
        mLeafVerts[1].x = v1.x;
        mLeafVerts[1].y = v1.y;
        mLeafVerts[1].z = v1.z;
        Fvector& v2 = verts[T.verts[2]];
        mLeafVerts[2].x = v2.x;
        mLeafVerts[2].y = v2.y;
        mLeafVerts[2].z = v2.z;
        if (!_tri())
            return;
        RESULT& R = dest->r_add();
        R.id = prim;
        R.verts[0] = v0;
        R.verts[1] = v1;
        R.verts[2] = v2;
        R.dummy = T.dummy;
    }
    void _stab(const AABBNoLeafNode* node)
    {
        // Actual box-box test
        if (!_box((Fvector&)node->mAABB.mCenter, (Fvector&)node->mAABB.mExtents))
            return;

        // 1st chield
        if (node->HasLeaf())
            _prim(node->GetPrimitive());
        else
            _stab(node->GetPos());

        // Early exit for "only first"
        if (bFirst && dest->r_count())
            return;

        // 2nd chield
        if (node->HasLeaf2())
            _prim(node->GetPrimitive2());
        else
            _stab(node->GetNeg());
    }
};

void COLLIDER::box_query(const MODEL* m_def, const Fvector& b_center, const Fvector& b_dim)
{
    m_def->syncronize();

    // Get nodes
    const AABBNoLeafTree* T = (const AABBNoLeafTree*)m_def->tree->GetTree();
    const AABBNoLeafNode* N = T->GetNodes();
    r_clear();

    // Binary dispatcher
    if (box_mode & OPT_FULL_TEST)
    {
        if (box_mode & OPT_ONLYFIRST)
        {
            box_collider<true, true> BC;
            BC._init(this, m_def->verts, m_def->tris, b_center, b_dim);
            BC._stab(N);
        }
        else
        {
            box_collider<true, false> BC;
            BC._init(this, m_def->verts, m_def->tris, b_center, b_dim);
            BC._stab(N);
        }
    }
    else
    {
        if (box_mode & OPT_ONLYFIRST)
        {
            box_collider<false, true> BC;
            BC._init(this, m_def->verts, m_def->tris, b_center, b_dim);
            BC._stab(N);
        }
        else
        {
            box_collider<false, false> BC;
            BC._init(this, m_def->verts, m_def->tris, b_center, b_dim);
            BC._stab(N);
        }
    }
}
