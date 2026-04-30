#pragma once

#if !defined(USE_OGL)
#include <DirectXMath.h>

using namespace DirectX;
#else
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/matrix_access.hpp"

namespace xray::render::fg
{
inline void XRVec3TransformCoordArray(glm::vec3* out, const glm::vec3* in, const glm::mat4& matrix, unsigned int elements)
{
    for (unsigned int i = 0; i < elements; ++i)
        out[i] = glm::vec3(glm::translate(matrix, in[i]) * glm::vec4(1.f,1.f,1.f,1.f));
}

inline void XRMatrixOrthoOffCenterLH(Fmatrix* pout, float l, float r, float b, float t, float zn, float zf)
{
    pout->identity();
    pout->m[0][0] = 2.0f / (r - l);
    pout->m[1][1] = 2.0f / (t - b);
    pout->m[2][2] = 2.0f / (zf -zn);
    pout->m[3][0] = -1.0f -2.0f *l / (r - l);
    pout->m[3][1] = 1.0f + 2.0f * t / (b - t);
    pout->m[3][2] = (zn + zf) / (zn -zf);
}

inline void XRMatrixInverse(Fmatrix* pout, float* pdeterminant, const Fmatrix& pm)
{
    glm::mat4 out = glm::inverse(glm::make_mat4x4(&pm.m[0][0]));
    *pout = *(Fmatrix*)glm::value_ptr(out);
}
} // namespace xray::render::fg
#endif

#define DW_AS_FLT(DW) (*(float*)&(DW))
#define FLT_AS_DW(F) (*(u32*)&(F))
#define FLT_SIGN(F) ((FLT_AS_DW(F) & 0x80000000L))
#define ALMOST_ZERO(F) ((FLT_AS_DW(F) & 0x7f800000L) == 0)
#define IS_SPECIAL(F) ((FLT_AS_DW(F) & 0x7f800000L) == 0x7f800000L)

namespace xray::render::fg
{
const u32 LIGHT_CUBOIDSIDEPOLYS_COUNT = 4;
const u32 LIGHT_CUBOIDVERTICES_COUNT = 2 * LIGHT_CUBOIDSIDEPOLYS_COUNT;

const float tweak_COP_initial_offs = 1200.f;
const float tweak_ortho_xform_initial_offs = 1000.f; //. ?
const float tweak_guaranteed_range = 20.f; //. ?

const float MAP_SIZE_START = 6.f;
const float MAP_GROW_FACTOR = 4.f;

//////////////////////////////////////////////////////////////////////////
// tables to calculate view-frustum bounds in world space
// note: D3D uses [0..1] range for Z
namespace sun
{
static constexpr Fvector3 corners[8] =
{
    { -1, -1, +0 }, { -1, -1, +1 },
    { -1, +1, +1 }, { -1, +1, +0 },
    { +1, +1, +1 }, { +1, +1, +0 },
    { +1, -1, +1 }, { +1, -1, +0 }
};

static constexpr int facetable[6][4] =
{
    { 6, 7, 5, 4 }, { 1, 0, 7, 6 },
    { 1, 2, 3, 0 }, { 3, 2, 4, 5 },
    // near and far planes
    { 0, 3, 5, 7 }, { 1, 6, 4, 2 },
};
} // namespace sun

inline Fvector3 wform(Fmatrix const& m, Fvector3 const& v)
{
    Fvector4 r;
    r.x = v.x * m._11 + v.y * m._21 + v.z * m._31 + m._41;
    r.y = v.x * m._12 + v.y * m._22 + v.z * m._32 + m._42;
    r.z = v.x * m._13 + v.y * m._23 + v.z * m._33 + m._43;
    r.w = v.x * m._14 + v.y * m._24 + v.z * m._34 + m._44;
    // VERIFY		(r.w>0.f);
    const float invW = 1.0f / r.w;
    return { r.x * invW, r.y * invW, r.z * invW };
}

#if defined(USE_DX11)
struct BoundingBox
{
    XMFLOAT3 minPt;
    XMFLOAT3 maxPt;

    BoundingBox() : minPt(1e33f, 1e33f, 1e33f), maxPt(-1e33f, -1e33f, -1e33f) {}

    BoundingBox(const BoundingBox& other) : minPt(other.minPt), maxPt(other.maxPt) {}

    explicit BoundingBox(const XMFLOAT3* points, u32 n) : minPt(1e33f, 1e33f, 1e33f), maxPt(-1e33f, -1e33f, -1e33f)
    {
        for (unsigned int i = 0; i < n; i++)
            Merge(&points[i]);
    }

    explicit BoundingBox(const xr_vector<XMFLOAT3>* points)
        : minPt(1e33f, 1e33f, 1e33f), maxPt(-1e33f, -1e33f, -1e33f)
    {
        for (unsigned int i = 0; i < points->size(); i++)
            Merge(&(*points)[i]);
    }

    explicit BoundingBox(const xr_vector<BoundingBox>* boxes)
        : minPt(1e33f, 1e33f, 1e33f), maxPt(-1e33f, -1e33f, -1e33f)
    {
        for (unsigned int i = 0; i < boxes->size(); i++)
        {
            Merge(&(*boxes)[i].maxPt);
            Merge(&(*boxes)[i].minPt);
        }
    }

    void Merge(const XMFLOAT3* vec)
    {
        minPt.x = std::min(minPt.x, vec->x);
        minPt.y = std::min(minPt.y, vec->y);
        minPt.z = std::min(minPt.z, vec->z);
        maxPt.x = std::max(maxPt.x, vec->x);
        maxPt.y = std::max(maxPt.y, vec->y);
        maxPt.z = std::max(maxPt.z, vec->z);
    }
};

///////////////////////////////////////////////////////////////////////////
//  PlaneIntersection
//    computes the point where three planes intersect
//    returns whether or not the point exists.
inline bool PlaneIntersection(
    XMVECTOR& intersectPt, FXMVECTOR n0, FXMVECTOR n1, FXMVECTOR n2)
{
    XMVECTOR n1_n2 = XMVector3Cross(n1, n2);
    XMVECTOR n2_n0 = XMVector3Cross(n2, n0);
    XMVECTOR n0_n1 = XMVector3Cross(n0, n1);

    const float cosTheta = XMVectorGetX(XMVector3Dot(n0, n1_n2));

    if (ALMOST_ZERO(cosTheta) || IS_SPECIAL(cosTheta))
        return false;

    const float secTheta = 1.f / cosTheta;

    n1_n2 = n1_n2 * XMVectorGetW(n0);
    n2_n0 = n2_n0 * XMVectorGetW(n1);
    n0_n1 = n0_n1 * XMVectorGetW(n2);

    intersectPt = -(n1_n2 + n2_n0 + n0_n1) * secTheta;
    return true;
}

struct Frustum
{
    XMFLOAT4 camPlanes[6];
    int nVertexLUT[6];
    XMFLOAT3 pntList[8];

    //  build a frustum from a camera (projection, or viewProjection) matrix
    Frustum(const XMFLOAT4X4* matrix)
    {
        //  build a view frustum based on the current view & projection matrices...
        const XMVECTOR column4 = XMVectorSet(matrix->_14, matrix->_24, matrix->_34, matrix->_44);
        const XMVECTOR column1 = XMVectorSet(matrix->_11, matrix->_21, matrix->_31, matrix->_41);
        const XMVECTOR column2 = XMVectorSet(matrix->_12, matrix->_22, matrix->_32, matrix->_42);
        const XMVECTOR column3 = XMVectorSet(matrix->_13, matrix->_23, matrix->_33, matrix->_43);

        XMVECTOR planes[6];
        planes[0] = column4 - column1; // left
        planes[1] = column4 + column1; // right
        planes[2] = column4 - column2; // bottom
        planes[3] = column4 + column2; // top
        planes[4] = column4 - column3; // near
        planes[5] = column4 + column3; // far
        // ignore near & far plane

        for (int p = 0; p < 6; p++) // normalize the planes
            planes[p] = XMVector3Normalize(planes[p]);

        for (int p = 0; p < 6; p++)
            XMStoreFloat4(&camPlanes[p], planes[p]);

        //  build a bit-field that will tell us the indices for the nearest and farthest vertices from each plane...
        for (int i = 0; i < 6; i++)
            nVertexLUT[i] = ((camPlanes[i].x < 0.f) ? 1 : 0) | ((camPlanes[i].y < 0.f) ? 2 : 0) | ((camPlanes[i].z < 0.f) ? 4 : 0);

        for (int i = 0; i < 8; i++) // compute extrema
        {
            XMVECTOR intersect;
            PlaneIntersection(intersect,
                (i & 1) ? planes[4] : planes[5],
                (i & 2) ? planes[3] : planes[2],
                (i & 4) ? planes[0] : planes[1]);
            XMStoreFloat3(&pntList[i], intersect);
        }
    }
};

//////////////////////////////////////////////////////////////////////////
// OLES: naive 3D clipper - roubustness around 0, but works for this sample
// note: normals points to 'outside'
//////////////////////////////////////////////////////////////////////////
struct DumbClipper
{
    CFrustum frustum;
    xr_vector<XMFLOAT4> planes;
    bool clip(XMFLOAT3& p0, XMFLOAT3& p1) // returns true if result meaningfull
    {
        XMVECTOR v0 = XMLoadFloat3(&p0);
        XMVECTOR v1 = XMLoadFloat3(&p1);
        for (auto& plane : planes)
        {
            XMVECTOR P = XMLoadFloat4(&plane);
            XMVECTOR cls0 = XMPlaneDotCoord(P, v0);
            XMVECTOR cls1 = XMPlaneDotCoord(P, v1);
            float cls0f = XMVectorGetX(cls0), cls1f = XMVectorGetX(cls1);
            if (cls0f > 0 && cls1f > 0)
                return false; // fully outside

            if (cls0f > 0)
            {
                // clip v0
                XMVECTOR D = v1 - v0;
                XMVECTOR denum = XMPlaneDotNormal(P, D);
                if (XMVectorGetX(denum) != 0)
                {
                    v0 += -D * cls0 / denum;
                    XMStoreFloat3(&p0, v0);
                }
            }
            if (cls1f > 0)
            {
                // clip v1
                XMVECTOR D = v0 - v1;
                XMVECTOR denum = XMPlaneDotNormal(P, D);
                if (XMVectorGetX(denum) != 0)
                {
                    v1 += -D * cls1 / denum;
                    XMStoreFloat3(&p1, v1);
                }
            }
        }
        return true;
    }

    XMFLOAT3 point(Fbox& bb, int i) const
    {
        return XMFLOAT3((i & 1) ? bb.vMin.x : bb.vMax.x, (i & 2) ? bb.vMin.y : bb.vMax.y, (i & 4) ? bb.vMin.z : bb.vMax.z);
    }

    Fbox clipped_AABB(xr_vector<Fbox>& src, Fmatrix& xf)
    {
        Fbox3 result;
        result.invalidate();
        for (int it = 0; it < int(src.size()); it++)
        {
            Fbox& bb = src[it];
            u32 mask = frustum.getMask();
            EFC_Visible res = frustum.testAABB(&bb.vMin.x, mask);
            switch (res)
            {
            case fcvNone: continue;
            case fcvFully:
                for (int c = 0; c < 8; c++)
                {
                    XMFLOAT3 p0 = point(bb, c);
                    Fvector x0 = wform(xf, *((Fvector*)(&p0)));
                    result.modify(x0);
                }
                break;
            case fcvPartial:
                for (int c0 = 0; c0 < 8; c0++)
                {
                    for (int c1 = 0; c1 < 8; c1++)
                    {
                        if (c0 == c1)
                            continue;
                        XMFLOAT3 p0 = point(bb, c0);
                        XMFLOAT3 p1 = point(bb, c1);
                        if (!clip(p0, p1))
                            continue;
                        Fvector x0 = wform(xf, *((Fvector*)(&p0)));
                        Fvector x1 = wform(xf, *((Fvector*)(&p1)));
                        result.modify(x0);
                        result.modify(x1);
                    }
                }
                break;
            } // switch (res)
        }
        return result;
    }
};

inline XMFLOAT2 BuildTSMProjectionMatrix_caster_depth_bounds(FXMMATRIX lightSpaceBasis, const xr_vector<Fbox>& casters)
{
    float min_z = 1e32f, max_z = -1e32f;

    Fmatrix minmax_xform;
    {
        XMMATRIX view      = XMLoadFloat4x4((XMFLOAT4X4*)&Device.mView);
        XMMATRIX minmax_xf = XMMatrixMultiply(view, lightSpaceBasis);
        XMStoreFloat4x4((XMFLOAT4X4*)&minmax_xform, minmax_xf);
    }
    for (u32 c = 0; c < casters.size(); c++)
    {
        Fvector3 pt;
        for (int e = 0; e < 8; e++)
        {
            casters[c].getpoint(e, pt);
            pt = wform(minmax_xform, pt);
            min_z = std::min(min_z, pt.z);
            max_z = std::max(max_z, pt.z);
        }
    }
    return XMFLOAT2(min_z, max_z);
}
#elif defined(USE_OGL)
struct BoundingBox
{
    glm::vec3 minPt;
    glm::vec3 maxPt;

    BoundingBox() : minPt(1e33f, 1e33f, 1e33f), maxPt(-1e33f, -1e33f, -1e33f) {}

    BoundingBox(const BoundingBox& other) : minPt(other.minPt), maxPt(other.maxPt) {}

    explicit BoundingBox(const glm::vec3* points, u32 n) : minPt(1e33f, 1e33f, 1e33f), maxPt(-1e33f, -1e33f, -1e33f)
    {
        for (unsigned int i = 0; i < n; i++)
            Merge(&points[i]);
    }

    explicit BoundingBox(const xr_vector<glm::vec3>* points)
        : minPt(1e33f, 1e33f, 1e33f), maxPt(-1e33f, -1e33f, -1e33f)
    {
        for (const auto& point : *points)
            Merge(&point);
    }

    explicit BoundingBox(const xr_vector<BoundingBox>* boxes)
        : minPt(1e33f, 1e33f, 1e33f), maxPt(-1e33f, -1e33f, -1e33f)
    {
        for (const auto & box : *boxes)
        {
            Merge(&box.maxPt);
            Merge(&box.minPt);
        }
    }

    void Merge(const glm::vec3* vec)
    {
        minPt.x = std::min(minPt.x, vec->x);
        minPt.y = std::min(minPt.y, vec->y);
        minPt.z = std::min(minPt.z, vec->z);
        maxPt.x = std::max(maxPt.x, vec->x);
        maxPt.y = std::max(maxPt.y, vec->y);
        maxPt.z = std::max(maxPt.z, vec->z);
    }
};

///////////////////////////////////////////////////////////////////////////
//  PlaneIntersection
//    computes the point where three planes intersect
//    returns whether or not the point exists.
inline bool PlaneIntersection(glm::vec3* intersectPt, const glm::vec4& p0, const glm::vec4& p1,
                                     const glm::vec4& p2)
{
    glm::vec3 n0 = glm::vec3(p0.x, p0.y, p0.z);
    glm::vec3 n1 = glm::vec3(p1.x, p1.y, p1.z);
    glm::vec3 n2 = glm::vec3(p2.x, p2.y, p2.z);

    glm::vec3 n1_n2 = glm::cross(n1, n2);
    glm::vec3 n2_n0 = glm::cross(n2, n0);
    glm::vec3 n0_n1 = glm::cross(n0, n1);

    float cosTheta = glm::dot(n0, n1_n2);

    if (ALMOST_ZERO(cosTheta) || IS_SPECIAL(cosTheta))
        return false;

    float secTheta = 1.f / cosTheta;

    n1_n2 *= p0.w;
    n2_n0 *= p1.w;
    n0_n1 *= p2.w;

    *intersectPt = -(n1_n2 + n2_n0 + n0_n1) * secTheta;
    return true;
}

struct Frustum
{
    glm::vec4 camPlanes[6];
    int nVertexLUT[6];
    glm::vec3 pntList[8];

    //  build a frustum from a camera (projection, or viewProjection) matrix
    explicit Frustum(const glm::mat4* matrix)
    {
        //  build a view frustum based on the current view & projection matrices...
        glm::vec4 column1 = glm::column(*matrix, 0);
        glm::vec4 column2 = glm::column(*matrix, 1);
        glm::vec4 column3 = glm::column(*matrix, 2);
        glm::vec4 column4 = glm::column(*matrix, 3);

        glm::vec4 planes[6];
        planes[0] = column4 - column1; // left
        planes[1] = column4 + column1; // right
        planes[2] = column4 - column2; // bottom
        planes[3] = column4 + column2; // top
        planes[4] = column4 - column3; // near
        planes[5] = column4 + column3; // far
        // ignore near & far plane

        for (int p = 0; p < 6; p++) // normalize the planes
        {
            camPlanes[p] = glm::normalize(planes[p]);
            // build a bit-field that will tell us the indices for the nearest and farthest vertices from each plane...
            nVertexLUT[p] = (camPlanes[p].x < 0.f ? 1 : 0) | (camPlanes[p].y < 0.f ? 2 : 0) | (camPlanes[p].z < 0.f ? 4 : 0);
        }

        for (int i = 0; i < 8; i++) // compute extrema
        {
            const glm::vec4& p0 = i & 1 ? camPlanes[4] : camPlanes[5];
            const glm::vec4& p1 = i & 2 ? camPlanes[3] : camPlanes[2];
            const glm::vec4& p2 = i & 4 ? camPlanes[0] : camPlanes[1];
            PlaneIntersection(&pntList[i], p0, p1, p2);
        }
    }
};

inline Fvector3 wform(Fmatrix const& m, glm::vec3 const& v)
{
    Fvector4 r;
    r.x = v.x * m._11 + v.y * m._21 + v.z * m._31 + m._41;
    r.y = v.x * m._12 + v.y * m._22 + v.z * m._32 + m._42;
    r.z = v.x * m._13 + v.y * m._23 + v.z * m._33 + m._43;
    r.w = v.x * m._14 + v.y * m._24 + v.z * m._34 + m._44;
    // VERIFY		(r.w>0.f);
    const float invW = 1.0f / r.w;
    return {r.x * invW, r.y * invW, r.z * invW};
}

inline Fvector3 wform(glm::mat4 const& m, Fvector3 const& v)
{
    Fvector4 r;
    r.x = v.x * m[0][0] + v.y * m[1][0] + v.z * m[2][0] + m[3][0];
    r.y = v.x * m[0][1] + v.y * m[1][1] + v.z * m[2][1] + m[3][1];
    r.z = v.x * m[0][2] + v.y * m[1][2] + v.z * m[2][2] + m[3][2];
    r.w = v.x * m[0][3] + v.y * m[1][3] + v.z * m[2][3] + m[3][3];
    // VERIFY		(r.w>0.f);
    const float invW = 1.0f / r.w;
    return {r.x * invW, r.y * invW, r.z * invW};
}

inline Fvector3 wform(glm::mat4 const& m, glm::vec3 const& v)
{
    Fvector4 r;
    r.x = v.x * m[0][0] + v.y * m[1][0] + v.z * m[2][0] + m[3][0];
    r.y = v.x * m[0][1] + v.y * m[1][1] + v.z * m[2][1] + m[3][1];
    r.z = v.x * m[0][2] + v.y * m[1][2] + v.z * m[2][2] + m[3][2];
    r.w = v.x * m[0][3] + v.y * m[1][3] + v.z * m[2][3] + m[3][3];
    // VERIFY		(r.w>0.f);
    const float invW = 1.0f / r.w;
    return {r.x * invW, r.y * invW, r.z * invW};
}

//////////////////////////////////////////////////////////////////////////
// OLES: naive 3D clipper - roubustness around 0, but works for this sample
// note: normals points to 'outside'
//////////////////////////////////////////////////////////////////////////
struct DumbClipper
{
    CFrustum frustum;
    xr_vector<glm::vec4> planes;

    BOOL clip(glm::vec3& p0, glm::vec3& p1) // returns TRUE if result meaningfull
    {
        float denum;
        glm::vec3 D;
        for (auto P : planes)
        {
            float cls0 = glm::dot(P, glm::vec4(p0, 1));
            float cls1 = glm::dot(P, glm::vec4(p1, 1));
            if (cls0 > 0 && cls1 > 0)
                return false; // fully outside

            if (cls0 > 0)
            {
                // clip p0
                D = p1 - p0;
                denum = glm::dot(P, glm::vec4(D, 0));
                if (denum != 0)
                    p0 += -D * cls0 / denum;
            }
            if (cls1 > 0)
            {
                // clip p1
                D = p0 - p1;
                denum = glm::dot(P, glm::vec4(D, 0));
                if (denum != 0)
                    p1 += -D * cls1 / denum;
            }
        }
        return true;
    }

    static glm::vec3 point(Fbox& bb, int i)
    {
        return glm::vec3(i & 1 ? bb.vMin.x : bb.vMax.x, i & 2 ? bb.vMin.y : bb.vMax.y, i & 4 ? bb.vMin.z : bb.vMax.z);
    }

    Fbox clipped_AABB(xr_vector<Fbox>& src, glm::mat4& xf)
    {
        Fbox3 result;
        result.invalidate();
        for (auto& bb : src)
        {
            u32 mask = frustum.getMask();
            EFC_Visible res = frustum.testAABB(&bb.vMin.x, mask);
            switch (res)
            {
            case fcvFully:
                for (int c = 0; c < 8; c++)
                {
                    glm::vec3 p0 = point(bb, c);
                    Fvector x0 = wform(xf, p0);
                    result.modify(x0);
                }
                break;
            case fcvPartial:
                for (int c0 = 0; c0 < 8; c0++)
                {
                    for (int c1 = 0; c1 < 8; c1++)
                    {
                        if (c0 == c1)
                            continue;
                        glm::vec3 p0 = point(bb, c0);
                        glm::vec3 p1 = point(bb, c1);
                        if (!clip(p0, p1))
                            continue;
                        Fvector x0 = wform(xf, p0);
                        Fvector x1 = wform(xf, p1);
                        result.modify(x0);
                        result.modify(x1);
                    }
                }
                break;
            } // switch (res)
        }
        return result;
    }
};

inline glm::vec2 BuildTSMProjectionMatrix_caster_depth_bounds(glm::mat4& lightSpaceBasis, const xr_vector<Fbox>& casters)
{
    float min_z = 1e32f, max_z = -1e32f;
    glm::mat4 minmax_xform = glm::make_mat4x4(&Device.mView.m[0][0]) * lightSpaceBasis;
    for (auto& caster : casters)
    {
        Fvector3 pt;
        for (int e = 0; e < 8; e++)
        {
            caster.getpoint(e, pt);
            pt = wform(minmax_xform, pt);
            min_z = _min(min_z, pt.z);
            max_z = _max(max_z, pt.z);
        }
    }
    return glm::vec2(min_z, max_z);
}
#endif

//////////////////////////////////////////////////////////////////////////
// OLES: naive builder of infinite volume expanded from base frustum towards
//		 light source. really slow, but it works for our simple usage :)
// note: normals points to 'outside'
//////////////////////////////////////////////////////////////////////////
template <bool _debug>
class DumbConvexVolume
{
public:
    struct _poly
    {
        xr_vector<int> points;
        Fvector3 planeN;
        float planeD;
        float classify(Fvector3& p) { return planeN.dotproduct(p) + planeD; }
    };
    struct _edge
    {
        int p0, p1;
        int counter;
        _edge(int _p0, int _p1, int m) : p0(_p0), p1(_p1), counter(m)
        {
            if (p0 > p1)
                std::swap(p0, p1);
        }
        bool equal(_edge& E) { return p0 == E.p0 && p1 == E.p1; }
    };

public:
    xr_vector<Fvector3> points;
    xr_vector<_poly> polys;
    xr_vector<_edge> edges;

public:
    void compute_planes()
    {
        for (int it = 0; it < int(polys.size()); it++)
        {
            _poly& P = polys[it];
            Fvector3 t1, t2;
            t1.sub(points[P.points[0]], points[P.points[1]]);
            t2.sub(points[P.points[0]], points[P.points[2]]);
            P.planeN.crossproduct(t1, t2);

            float len = P.planeN.magnitude();

            if (len > std::numeric_limits<float>::min())
            {
                P.planeN.mul(1 / len);
            }
            else
            {
                t2.sub(points[P.points[0]], points[P.points[3]]);
                P.planeN.crossproduct(t1, t2);
                if (len > std::numeric_limits<float>::min())
                {
                    P.planeN.mul(1 / len);
                }
                else
                {
                    //	HACK:	Remove plane.
                    // VERIFY(!"Can't build normal to plane!");
                    polys.erase(polys.begin() + it);
                    --it;
                    continue;
                }
            }
            P.planeD = -P.planeN.dotproduct(points[P.points[0]]);

            // verify
            if constexpr (_debug)
            {
                Fvector& p0 = points[P.points[0]];
                Fvector& p1 = points[P.points[1]];
                Fvector& p2 = points[P.points[2]];
                Fvector& p3 = points[P.points[3]];
                Fplane p012;
                p012.build(p0, p1, p2);
                Fplane p123;
                p123.build(p1, p2, p3);
                Fplane p230;
                p230.build(p2, p3, p0);
                Fplane p301;
                p301.build(p3, p0, p1);
                VERIFY(p012.n.similar(p123.n) && p012.n.similar(p230.n) && p012.n.similar(p301.n));
            }
        }
    }

    void compute_caster_model(xr_vector<Fplane>& dest, Fvector3 direction)
    {
        CRenderTarget& T = *RImplementation.m_framegraphRenderer->m_pTarget;

        // COG
        Fvector3 cog = { 0, 0, 0 };
        for (int it = 0; it < int(points.size()); it++)
            cog.add(points[it]);
        cog.div(float(points.size()));

        // planes
        compute_planes();
        for (int it = 0; it < int(polys.size()); it++)
        {
            _poly& base = polys[it];
            if (base.classify(cog) > 0)
                std::reverse(base.points.begin(), base.points.end());
        }

        // remove faceforward polys, build list of edges -> find open ones
        compute_planes();
        for (int it = 0; it < int(polys.size()); it++)
        {
            _poly& base = polys[it];
            VERIFY(base.classify(cog) < 0); // debug

            int marker = (base.planeN.dotproduct(direction) <= 0) ? -1 : 1;

            // register edges
            xr_vector<int>& plist = polys[it].points;
            for (int p = 0; p < int(plist.size()); p++)
            {
                _edge E(plist[p], plist[(p + 1) % plist.size()], marker);
                bool found = false;
                for (int e = 0; e < int(edges.size()); e++)
                    if (edges[e].equal(E))
                    {
                        edges[e].counter += marker;
                        found = true;
                        break;
                    }
                if (!found)
                {
                    if constexpr (_debug)
                        T.dbg_addline(points[E.p0], points[E.p1], color_rgba(255, 0, 0, 255));
                    edges.emplace_back(std::move(E));
                }
            }

            // remove if unused
            if (marker < 0)
            {
                polys.erase(polys.begin() + it);
                it--;
            }
        }

        // Extend model to infinity, the volume is not capped, so this is indeed up to infinity
        for (int e = 0; e < int(edges.size()); e++)
        {
            if (edges[e].counter != 0)
                continue;
            _edge& E = edges[e];
            if constexpr (_debug)
                T.dbg_addline(points[E.p0], points[E.p1], color_rgba(255, 255, 255, 255));
            Fvector3 point;
            points.emplace_back(point.sub(points[E.p0], direction));
            points.emplace_back(point.sub(points[E.p1], direction));
            _poly& P = polys.emplace_back(_poly());
            const int pend = int(points.size());
            P.points.emplace_back(E.p0);
            P.points.emplace_back(E.p1);
            P.points.emplace_back(pend - 1); // p1 mod
            P.points.emplace_back(pend - 2); // p0 mod
            if constexpr (_debug)
            {
                T.dbg_addline(points[E.p0], point.mad(points[E.p0], direction, -1000), color_rgba(0, 255, 0, 255));
                T.dbg_addline(points[E.p1], point.mad(points[E.p1], direction, -1000), color_rgba(0, 255, 0, 255));
            }
        }

        // Reorient planes (try to write more inefficient code :)
        compute_planes();
        for (_poly& base : polys)
        {
            if (base.classify(cog) > 0)
                std::reverse(base.points.begin(), base.points.end());
        }

        // Export
        compute_planes();
        for (_poly& poly : polys)
            dest.emplace_back(Fplane{ poly.planeN, poly.planeD });
    }
};

template <bool _debug>
class FixedConvexVolume
{
public:
    struct _poly
    {
        int points[4];
        Fplane plane;
    };

    xr_vector<sun::ray> view_frustum_rays;
    sun::ray view_ray;
    sun::ray light_ray;
    Fvector3 light_cuboid_points[LIGHT_CUBOIDVERTICES_COUNT];
    _poly light_cuboid_polys[LIGHT_CUBOIDSIDEPOLYS_COUNT];

public:
    void compute_planes()
    {
        for (u32 it = 0; it < LIGHT_CUBOIDSIDEPOLYS_COUNT; it++)
        {
            _poly& P = light_cuboid_polys[it];

            P.plane.build(
                light_cuboid_points[P.points[0]], light_cuboid_points[P.points[2]], light_cuboid_points[P.points[1]]);

            // verify
            if constexpr (_debug)
            {
                Fvector& p0 = light_cuboid_points[P.points[0]];
                Fvector& p1 = light_cuboid_points[P.points[1]];
                Fvector& p2 = light_cuboid_points[P.points[2]];
                Fvector& p3 = light_cuboid_points[P.points[3]];
                Fplane p012;
                p012.build(p0, p1, p2);
                Fplane p123;
                p123.build(p1, p2, p3);
                Fplane p230;
                p230.build(p2, p3, p0);
                Fplane p301;
                p301.build(p3, p0, p1);
                VERIFY(p012.n.similar(p123.n) && p012.n.similar(p230.n) && p012.n.similar(p301.n));
            }
        }
    }

    void compute_caster_model_fixed(
        xr_vector<Fplane>& dest, Fvector3& translation, float map_size, bool clip_by_view_near)
    {
        translation.set(0.f, 0.f, 0.f);

        if (fis_zero(1 - abs(view_ray.D.dotproduct(light_ray.D)), EPS_S))
            return;

        // compute planes for each polygon.
        compute_planes();

        for (u32 i = 0; i < LIGHT_CUBOIDSIDEPOLYS_COUNT; i++)
            VERIFY(light_cuboid_polys[i].plane.classify(light_ray.P) > 0);

        int align_planes[2];
        int align_planes_count = 0;

        // find one or two planes that align to view frustum from behind.
        for (u32 i = 0; i < LIGHT_CUBOIDSIDEPOLYS_COUNT; i++)
        {
            float tmp_dot = view_ray.D.dotproduct(light_cuboid_polys[i].plane.n);
            if (tmp_dot <= EPS_L)
                continue;

            align_planes[align_planes_count] = i;
            ++align_planes_count;

            if (align_planes_count == 2)
                break;
        }

        Fvector align_vector;
        align_vector.set(0.f, 0.f, 0.f);

        // Align ray points to the align planes.
        for (int p = 0; p < align_planes_count; ++p)
        {
            // Hack !
            float min_dist = 10000;
            for (u32 i = 0; i < view_frustum_rays.size(); ++i)
            {
                float tmp_dist = 0;
                Fvector tmp_point = view_frustum_rays[i].P;

                tmp_dist = light_cuboid_polys[align_planes[p]].plane.classify(tmp_point);
                min_dist = std::min(tmp_dist, min_dist);
            }

            Fvector shift = light_cuboid_polys[align_planes[p]].plane.n;
            shift.mul(min_dist);
            align_vector.add(shift);
        }

        translation.add(align_vector);

        // Move light ray by the alignment shift.
        light_ray.P.add(align_vector);

        // Here we can skip this stage us in the next pass we need only normals of planes.
        // in the next translate_light_model call will contain this shift as well.
        // translate_light_model	( align_vector );

        // Reset to reuse.
        align_vector.set(0.f, 0.f, 0.f);

        // Check if view edges intersect, and push planes................
        for (int p = 0; p < align_planes_count; ++p)
        {
            float max_mag = 0;
            for (u32 i = 0; i < view_frustum_rays.size(); ++i)
            {
                float plane_dot_ray = view_frustum_rays[i].D.dotproduct(light_cuboid_polys[align_planes[p]].plane.n);
                if (plane_dot_ray < 0)
                {
                    Fvector per_plane_view;
                    per_plane_view.crossproduct(light_cuboid_polys[align_planes[p]].plane.n, view_ray.D);
                    Fvector per_view_to_plane;
                    per_view_to_plane.crossproduct(per_plane_view, view_ray.D);

                    float tmp_mag = -plane_dot_ray / view_frustum_rays[i].D.dotproduct(per_view_to_plane);

                    max_mag = (max_mag < tmp_mag) ? tmp_mag : max_mag;
                }
            }

            if (fis_zero(max_mag))
                continue;

            VERIFY(max_mag <= 1.f);

            float dist = -light_cuboid_polys[align_planes[p]].plane.n.dotproduct(translation);
            align_vector.mad(light_cuboid_polys[align_planes[p]].plane.n, dist * max_mag);
        }

        translation.add(align_vector);
        light_ray.P.add(align_vector);
        translate_light_model(translation);

        // compute culling planes by rays as edges
        for (u32 i = 0; i < view_frustum_rays.size(); ++i)
        {
            Fvector tmp_vector;
            tmp_vector.crossproduct(view_frustum_rays[i].D, light_ray.D);

            // check if the vectors are parallel
            if (fis_zero(tmp_vector.square_magnitude(), EPS))
                continue;

            Fplane tmp_plane;
            tmp_plane.build(view_frustum_rays[i].P, tmp_vector);

            float sign = 0;
            if (check_cull_plane_valid(tmp_plane, sign, 5))
            {
                tmp_plane.n.mul(-sign);
                tmp_plane.d *= -sign;
                dest.emplace_back(std::move(tmp_plane));
            }
        }

        // compute culling planes by ray points pairs as edges
        if (clip_by_view_near && abs(view_ray.D.dotproduct(light_ray.D)) < 0.8)
        {
            Fvector perp_light_view, perp_light_to_view;
            perp_light_view.crossproduct(view_ray.D, light_ray.D);
            perp_light_to_view.crossproduct(perp_light_view, light_ray.D);

            Fplane plane;
            plane.build(view_ray.P, perp_light_to_view);

            float max_dist = -1000;
            for (u32 i = 0; i < view_frustum_rays.size(); ++i)
                max_dist = _max(plane.classify(view_frustum_rays[i].P), max_dist);

            for (u32 i = 0; i < view_frustum_rays.size(); ++i)
            {
                Fvector P = view_frustum_rays[i].P;
                P.mad(view_frustum_rays[i].D, 5);

                if (plane.classify(P) > max_dist)
                {
                    max_dist = 0.f;
                    break;
                }
            }

            if (max_dist > -1000)
            {
                plane.d += max_dist;
                dest.emplace_back(std::move(plane));
            }
        }

        for (u32 i = 0; i < LIGHT_CUBOIDSIDEPOLYS_COUNT; i++)
        {
            Fplane& plane = dest.emplace_back(light_cuboid_polys[i].plane);
            plane.n.mul(-1);
            plane.d *= -1;
            VERIFY(light_cuboid_polys[i].plane.classify(light_ray.P) > 0);
        }

        // Compute ray intersection with light model, this is needed to next cascade to start it's placement.
        for (u32 i = 0; i < view_frustum_rays.size(); ++i)
        {
            float min_dist = 2 * map_size;
            for (int p = 0; p < 4; ++p)
            {
                float dist = 0.0f;
                if ((light_cuboid_polys[p].plane.n.dotproduct(view_frustum_rays[i].D)) > -0.1)
                    dist = map_size;
                else
                    light_cuboid_polys[p].plane.intersectRayDist(view_frustum_rays[i].P, view_frustum_rays[i].D, dist);

                if (dist > EPS_L && dist < min_dist)
                    min_dist = dist;
            }

            view_frustum_rays[i].P.mad(view_frustum_rays[i].D, min_dist);
        }
    }

    bool check_cull_plane_valid(Fplane const& plane, float& sign, float mad_factor = 0.f)
    {
        bool valid = false;
        bool oriented = false;
        float orient = 0;
        for (u32 j = 0; j < view_frustum_rays.size(); ++j)
        {
            float tmp_dist = 0.f;
            Fvector tmp_pt = view_frustum_rays[j].P;
            tmp_pt.mad(view_frustum_rays[j].D, mad_factor);
            tmp_dist = plane.classify(tmp_pt);

            if (fis_zero(tmp_dist, EPS_L))
                continue;

            if (!oriented)
            {
                orient = tmp_dist > 0.f ? 1.f : -1.f;
                valid = true;
                oriented = true;
                continue;
            }

            if (tmp_dist < 0 && orient < 0 || tmp_dist > 0 && orient > 0)
                continue;

            valid = false;
            break;
        }
        sign = orient;
        return valid;
    }

    void translate_light_model(Fvector translate)
    {
        Fmatrix trans_mat;
        trans_mat.translate(translate);
        for (u32 i = 0; i < LIGHT_CUBOIDSIDEPOLYS_COUNT; ++i)
            light_cuboid_polys[i].plane.d -= translate.dotproduct(light_cuboid_polys[i].plane.n);
    }
};
} // namespace xray::render::fg
