#include "stdafx.h"

#include "r2_R_sun_support.h"

#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/IRenderable.h"
#include "Layers/xrRender/FBasicVisual.h"
#include "xrCore/Threading/ParallelFor.hpp"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/matrix_access.hpp"

// float			OLES_SUN_LIMIT_27_01_07			= 180.f		;
float OLES_SUN_LIMIT_27_01_07 = 100.f;

//////////////////////////////////////////////////////////////////////////
// XXX: examine
#define DW_AS_FLT(DW) (*(float*)&(DW))
#define FLT_AS_DW(F) (*(u32*)&(F))
#define FLT_SIGN(F) ((FLT_AS_DW(F) & 0x80000000L))
#define ALMOST_ZERO(F) ((FLT_AS_DW(F) & 0x7f800000L) == 0)
#define IS_SPECIAL(F) ((FLT_AS_DW(F) & 0x7f800000L) == 0x7f800000L)

//////////////////////////////////////////////////////////////////////////
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
static inline bool PlaneIntersection(glm::vec3* intersectPt, const glm::vec4& p0, const glm::vec4& p1,
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
    explicit Frustum(const glm::mat4* matrix);

    glm::vec4 camPlanes[6];
    int nVertexLUT[6];
    glm::vec3 pntList[8];
};

//  build a frustum from a camera (projection, or viewProjection) matrix
Frustum::Frustum(const glm::mat4* matrix)
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

    int p;

    for (p = 0; p < 6; p++) // normalize the planes
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

//////////////////////////////////////////////////////////////////////////
Fvector3 wform(Fmatrix const& m, glm::vec3 const& v)
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

Fvector3 wform(glm::mat4 const& m, Fvector3 const& v)
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

Fvector3 wform(glm::mat4 const& m, glm::vec3 const& v)
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
const float _eps = 0.000001f;

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

xr_vector<Fbox> s_casters;

glm::vec2 BuildTSMProjectionMatrix_caster_depth_bounds(glm::mat4& lightSpaceBasis)
{
    float min_z = 1e32f, max_z = -1e32f;
    glm::mat4 minmax_xform = glm::make_mat4x4(&Device.mView.m[0][0]) * lightSpaceBasis;
    for (auto& s_caster : s_casters)
    {
        Fvector3 pt;
        for (int e = 0; e < 8; e++)
        {
            s_caster.getpoint(e, pt);
            pt = wform(minmax_xform, pt);
            min_z = _min(min_z, pt.z);
            max_z = _max(max_z, pt.z);
        }
    }
    return glm::vec2(min_z, max_z);
}

void render_sun_old::init()
{
    u32 cascade_count = R__NUM_SUN_CASCADES;
    m_sun_cascades.resize(cascade_count);

    float fBias = -0.0000025f;
    //	float size = MAP_SIZE_START;
    m_sun_cascades[0].reset_chain = true;
    m_sun_cascades[0].size = 20;
    m_sun_cascades[0].bias = m_sun_cascades[0].size * fBias;

    m_sun_cascades[1].size = 40;
    m_sun_cascades[1].bias = m_sun_cascades[1].size * fBias;

    m_sun_cascades[2].size = 160;
    m_sun_cascades[2].bias = m_sun_cascades[2].size * fBias;

    // 	for( u32 i = 0; i < cascade_count; ++i )
    // 	{
    // 		m_sun_cascades[i].size = size;
    // 		size *= MAP_GROW_FACTOR;
    // 	}
    /// 	m_sun_cascades[m_sun_cascades.size()-1].size = 80;
    sun = (light*)RImplementation.Lights.sun._get();

    const Fcolor sun_color = sun->color;
    o.active = ps_r2_ls_flags.test(R2FLAG_SUN) && (u_diffuse2s(sun_color.r, sun_color.g, sun_color.b) > EPS);
    if (RImplementation.o.sunstatic)
        o.active = false;

    if (!o.active)
        return;

    o.mt_calc_enabled = RImplementation.o.mt_calculate;

    // pre-allocate context
    context_id = RImplementation.alloc_context();
    VERIFY(context_id != R_dsgraph_structure::INVALID_CONTEXT_ID);
}

void render_sun_old::render_sun() const
{
    PIX_EVENT(render_sun);
    glm::mat4 m_LightViewProj;

    // calculate view-frustum bounds in world space
    glm::mat4 ex_full, ex_project, ex_full_inverse;
    {
        float _far_ = std::min(OLES_SUN_LIMIT_27_01_07, g_pGamePersistent->Environment().CurrentEnv.far_plane);
        ex_project = glm::perspective(deg2rad(Device.fFOV), Device.fASPECT, VIEWPORT_NEAR, _far_);
        ex_full = ex_project * glm::make_mat4x4(&Device.mView.m[0][0]);
        ex_full_inverse = glm::inverse(ex_full);
    }

    // Compute volume(s) - something like a frustum for infinite directional light
    // Also compute virtual light position and sector it is inside
    CFrustum cull_frustum;
    xr_vector<Fplane> cull_planes;
    Fvector3 cull_COP;
    glm::mat4 cull_xform;
    {
        // Lets begin from base frustum
        DumbConvexVolume<false> hull;
        {
            hull.points.reserve(std::size(sun::corners));
            hull.polys.reserve(std::size(sun::facetable));

            for (const auto& corner : sun::corners)
            {
                Fvector3 xf = wform(ex_full_inverse, corner);
                hull.points.emplace_back(xf);
            }
            for (auto& plane : sun::facetable)
            {
                auto& poly = hull.polys.emplace_back();
                poly.points.reserve(std::size(plane));
                for (const int pt : plane)
                    poly.points.emplace_back(pt);
            }
        }
        hull.compute_caster_model(cull_planes, sun->direction);

        // COP - 100 km away
        cull_COP.mad(Device.vCameraPosition, sun->direction, -tweak_COP_initial_offs);

        // Create frustum for query
        cull_frustum._clear();
        for (auto& cull_plane : cull_planes)
            cull_frustum._add(cull_plane);

        // Create approximate ortho-xform
        // view: auto find 'up' and 'right' vectors
        glm::mat4 mdir_View, mdir_Project;
        glm::vec3 L_dir, L_up, L_right, L_pos;
        L_pos = glm::vec3(sun->position.x, sun->position.y, sun->position.z);
        L_dir = glm::normalize(glm::vec3(sun->direction.x, sun->direction.y, sun->direction.z));
        L_up = glm::vec3(0.f, 1.f, 0.f);
        if (_abs(glm::dot(L_up, L_dir)) > .99f) L_up = glm::vec3(0.f, 0.f, 1.f);
        L_right = glm::normalize(glm::cross(L_up, L_dir));
        L_up = glm::normalize(glm::cross(L_dir, L_right));
        mdir_View = glm::lookAt(L_pos, L_dir, L_up);

        // projection: box
        Fbox frustum_bb;
        frustum_bb.invalidate();
        for (const auto& point : hull.points)
        {
            Fvector xf = wform(mdir_View, point);
            frustum_bb.modify(xf);
        }
        Fbox& bb = frustum_bb;
        bb.grow(EPS);

        mdir_Project = glm::ortho(bb.vMin.x, bb.vMax.x, bb.vMin.y, bb.vMax.y,
                                  bb.vMin.z - tweak_ortho_xform_initial_offs, bb.vMax.z);

        // full-xform
        cull_xform = mdir_Project * mdir_View;
    }

    // Begin SMAP-render
    xr_vector<Fbox3>& s_receivers = RImplementation.main_coarse_structure;
    s_casters.reserve(s_receivers.size());

    auto& dsgraph = RImplementation.get_context(context_id);
    {
        //		sun->svis.begin					();
        dsgraph.o.phase = CRender::PHASE_SMAP;
        dsgraph.r_pmask(true, RImplementation.o.Tshadows);
        dsgraph.o.sector_id = RImplementation.get_largest_sector();
        dsgraph.o.xform = *(Fmatrix*)glm::value_ptr(cull_xform);
        dsgraph.o.view_pos = cull_COP;
        dsgraph.o.view_frustum = cull_frustum;
        dsgraph.set_Recorder(&s_casters);

        // Fill the database
        dsgraph.build_subspace();

        // IGNORE PORTALS
        if (ps_r2_ls_flags.test(R2FLAG_SUN_IGNORE_PORTALS))
        {
            for (auto& S : dsgraph.Sectors)
            {
                dxRender_Visual* root = static_cast<CSector*>(S)->root();
                dsgraph.add_static(root, cull_frustum, cull_frustum.getMask());
            }
        }
        dsgraph.set_Recorder(nullptr);
    }

    //	Prepare to interact with D3DX code
    const glm::mat4 m_View = glm::make_mat4x4(&Device.mView.m[0][0]);
    const glm::mat4 m_Projection = ex_project;
    glm::vec3 m_lightDir = -glm::vec3(sun->direction.x, sun->direction.y, sun->direction.z);

    //  these are the limits specified by the physical camera
    //  gamma is the "tilt angle" between the light and the view direction.
    float m_fCosGamma = m_lightDir.x * m_View[0][2] +
        m_lightDir.y * m_View[1][2] +
        m_lightDir.z * m_View[2][2];
    float m_fTSM_Delta = ps_r2_sun_tsm_projection;

    // Compute REAL sheared xform based on receivers/casters information
    if (_abs(m_fCosGamma) < 0.99f && ps_r2_ls_flags.test(R2FLAG_SUN_TSM))
    {
        //  get the near and the far plane (points) in eye space.
        constexpr size_t POINTS_NUM = 8;
        glm::vec3 frustumPnts[POINTS_NUM];

        Frustum eyeFrustum(&m_Projection); // autocomputes all the extrema points

        for (int i = 0; i < 4; i++)
        {
            frustumPnts[i] = eyeFrustum.pntList[(i << 1)]; // far plane
            frustumPnts[i + 4] = eyeFrustum.pntList[i << 1 | 0x1]; // near plane
        }

        //   we need to transform the eye into the light's post-projective space.
        //   however, the sun is a directional light, so we first need to find an appropriate
        //   rotate/translate matrix, before constructing an ortho projection.
        //   this matrix is a variant of "light space" from LSPSMs, with the Y and Z axes permuted

        glm::vec3 leftVector, viewVector;
        const glm::vec3 eyeVector(0.f, 0.f, -1.f); //  eye is always -Z in eye space

        //  code copied straight from BuildLSPSMProjectionMatrix
        glm::mat4 translate = glm::translate(m_View, m_lightDir); // lightDir is defined in eye space, so xform it
        glm::vec4 vector(1.f,1.f,1.f,1.f);
        glm::vec3 upVector = glm::vec3(translate * vector);
        leftVector = glm::cross(upVector, eyeVector);
        leftVector = glm::normalize(leftVector);
        viewVector = glm::cross(upVector, leftVector);

        glm::mat4 lightSpaceBasis;
        lightSpaceBasis[0][0] = leftVector.x;
        lightSpaceBasis[0][1] = viewVector.x;
        lightSpaceBasis[0][2] = -upVector.x;
        lightSpaceBasis[0][3] = 0.f;
        lightSpaceBasis[1][0] = leftVector.y;
        lightSpaceBasis[1][1] = viewVector.y;
        lightSpaceBasis[1][2] = -upVector.y;
        lightSpaceBasis[1][3] = 0.f;
        lightSpaceBasis[2][0] = leftVector.z;
        lightSpaceBasis[2][1] = viewVector.z;
        lightSpaceBasis[2][2] = -upVector.z;
        lightSpaceBasis[2][3] = 0.f;
        lightSpaceBasis[3][0] = 0.f;
        lightSpaceBasis[3][1] = 0.f;
        lightSpaceBasis[3][2] = 0.f;
        lightSpaceBasis[3][3] = 1.f;

        //  rotate the view frustum into light space
        XRVec3TransformCoordArray(frustumPnts, frustumPnts, lightSpaceBasis, POINTS_NUM);

        //  build an off-center ortho projection that translates and scales the eye frustum's 3D AABB to the unit cube
        BoundingBox frustumBox(frustumPnts, POINTS_NUM);

        //  also - transform the shadow caster bounding boxes into light projective space.  we want to translate along the Z axis so that
        //  all shadow casters are in front of the near plane.
        glm::vec2 depthbounds = BuildTSMProjectionMatrix_caster_depth_bounds(lightSpaceBasis);

        float min_z = std::min(depthbounds.x, frustumBox.minPt.z);
        float max_z = std::max(depthbounds.y, frustumBox.maxPt.z);

        if (min_z <= 1.f) //?
        {
            glm::mat4 lightSpaceTranslate;
            lightSpaceTranslate = glm::translate(lightSpaceTranslate, glm::vec3(0.f, 0.f, -min_z + 1.f));
            max_z = -min_z + max_z + 1.f;
            min_z = 1.f;
            lightSpaceBasis *= lightSpaceTranslate;
            XRVec3TransformCoordArray(frustumPnts, frustumPnts, lightSpaceTranslate, POINTS_NUM);
            frustumBox = BoundingBox(frustumPnts, POINTS_NUM);
        }

        glm::mat4 lightSpaceOrtho = glm::ortho(frustumBox.minPt.x, frustumBox.maxPt.x,
                                               frustumBox.minPt.y, frustumBox.maxPt.y,
                                               min_z, max_z);

        //  transform the view frustum by the new matrix
        XRVec3TransformCoordArray(frustumPnts, frustumPnts, lightSpaceOrtho, POINTS_NUM);

        glm::vec2 centerPts[2];
        //  near plane
        centerPts[0].x = 0.25f * (frustumPnts[4].x + frustumPnts[5].x + frustumPnts[6].x + frustumPnts[7].x);
        centerPts[0].y = 0.25f * (frustumPnts[4].y + frustumPnts[5].y + frustumPnts[6].y + frustumPnts[7].y);
        //  far plane
        centerPts[1].x = 0.25f * (frustumPnts[0].x + frustumPnts[1].x + frustumPnts[2].x + frustumPnts[3].x);
        centerPts[1].y = 0.25f * (frustumPnts[0].y + frustumPnts[1].y + frustumPnts[2].y + frustumPnts[3].y);

        glm::vec2 centerOrig = centerPts[0];
        centerOrig += centerPts[1];
        centerOrig *= 0.5f;

        glm::mat4 trapezoid_space;

        glm::mat4 xlate_center = {1.f, 0.f, 0.f, 0.f,
                                0.f, 1.f, 0.f, 0.f,
                                0.f, 0.f, 1.f, 0.f,
                                -centerOrig.x, -centerOrig.y, 0.f, 1.f};

        glm::vec2 center_dirl = centerPts[1];
        center_dirl -= centerOrig;
        float half_center_len = glm::length(center_dirl);
        float x_len = centerPts[1].x - centerOrig.x;
        float y_len = centerPts[1].y - centerOrig.y;

        float cos_theta = x_len / half_center_len;
        float sin_theta = y_len / half_center_len;

        glm::mat4 rot_center = {cos_theta, -sin_theta, 0.f, 0.f,
                              sin_theta, cos_theta, 0.f, 0.f,
                              0.f, 0.f, 1.f, 0.f,
                              0.f, 0.f, 0.f, 1.f};

        //  this matrix transforms the center line to y=0.
        //  since Top and Base are orthogonal to Center, we can skip computing the convex hull, and instead
        //  just find the view frustum X-axis extrema.  The most negative is Top, the most positive is Base
        //  Point Q (trapezoid projection point) will be a point on the y=0 line.
        trapezoid_space = xlate_center * rot_center;
        XRVec3TransformCoordArray(frustumPnts, frustumPnts, trapezoid_space, POINTS_NUM);

        BoundingBox frustumAABB2D(frustumPnts, POINTS_NUM);

        float x_scale = std::max(_abs(frustumAABB2D.maxPt.x), _abs(frustumAABB2D.minPt.x));
        float y_scale = std::max(_abs(frustumAABB2D.maxPt.y), _abs(frustumAABB2D.minPt.y));
        x_scale = 1.f / x_scale;
        y_scale = 1.f / y_scale;

        //  maximize the area occupied by the bounding box
        glm::mat4 scale_center = {x_scale, 0.f, 0.f, 0.f,
                                0.f, y_scale, 0.f, 0.f,
                                0.f, 0.f, 1.f, 0.f,
                                0.f, 0.f, 0.f, 1.f};

        trapezoid_space *= scale_center;

        //  scale the frustum AABB up by these amounts (keep all values in the same space)
        frustumAABB2D.minPt.x *= x_scale;
        frustumAABB2D.maxPt.x *= x_scale;
        frustumAABB2D.minPt.y *= y_scale;
        frustumAABB2D.maxPt.y *= y_scale;

        //  compute eta.
        float lambda = frustumAABB2D.maxPt.x - frustumAABB2D.minPt.x;
        float delta_proj = m_fTSM_Delta * lambda; //focusPt.x - frustumAABB2D.minPt.x;
        const float xi = -0.6f; // - 0.6f;  // 80% line
        float eta = lambda * delta_proj * (1.f + xi) / (lambda * (1.f - xi) - 2.f * delta_proj);

        //  compute the projection point a distance eta from the top line.  this point is on the center line, y=0
        glm::vec2 projectionPtQ(frustumAABB2D.maxPt.x + eta, 0.f);

        //  find the maximum slope from the projection point to any point in the frustum.  this will be the
        //  projection field-of-view
        float max_slope = -1e32f;
        float min_slope = 1e32f;

        for (auto& pnt : frustumPnts)
        {
            glm::vec2 tmp(pnt.x * x_scale, pnt.y * y_scale);
            float x_dist = tmp.x - projectionPtQ.x;
            if (!(ALMOST_ZERO(tmp.y) || ALMOST_ZERO(x_dist)))
            {
                max_slope = std::max(max_slope, tmp.y / x_dist);
                min_slope = std::min(min_slope, tmp.y / x_dist);
            }
        }

        float xn = eta;
        float xf = lambda + eta;

        glm::mat4 ptQ_xlate = {-1.f, 0.f, 0.f, 0.f,
                             0.f, 1.f, 0.f, 0.f,
                             0.f, 0.f, 1.f, 0.f,
                             projectionPtQ.x, 0.f, 0.f, 1.f};
        trapezoid_space *= ptQ_xlate;

        //  this shear balances the "trapezoid" around the y=0 axis (no change to the projection pt position)
        //  since we are redistributing the trapezoid, this affects the projection field of view (shear_amt)
        float shear_amt = (max_slope + _abs(min_slope)) * 0.5f - max_slope;
        max_slope = max_slope + shear_amt;

        glm::mat4 trapezoid_shear(1.f, shear_amt, 0.f, 0.f,
                                   0.f, 1.f, 0.f, 0.f,
                                   0.f, 0.f, 1.f, 0.f,
                                   0.f, 0.f, 0.f, 1.f);

        trapezoid_space *= trapezoid_shear;

        float z_aspect = (frustumBox.maxPt.z - frustumBox.minPt.z) / (frustumAABB2D.maxPt.y - frustumAABB2D.minPt.y);

        //  perform a 2DH projection to 'unsqueeze' the top line.
        glm::mat4 trapezoid_projection(xf / (xf - xn), 0.f, 0.f, 1.f,
                                        0.f, 1.f / max_slope, 0.f, 0.f,
                                        0.f, 0.f, 1.f / (z_aspect * max_slope), 0.f,
                                        -xn * xf / (xf - xn), 0.f, 0.f, 0.f);

        trapezoid_space *= trapezoid_projection;

        //  the x axis is compressed to [0..1] as a result of the projection, so expand it to [-1,1]
        glm::mat4 biasedScaleX(2.f, 0.f, 0.f, 0.f,
                                0.f, 1.f, 0.f, 0.f,
                                0.f, 0.f, 1.f, 0.f,
                                -1.f, 0.f, 0.f, 1.f);
        trapezoid_space *= biasedScaleX;

        m_LightViewProj *= lightSpaceBasis * lightSpaceOrtho * trapezoid_space;
    }
    else
    {
        m_LightViewProj = cull_xform;
    }

    // perform "refit" or "focusing" on relevant
    if (ps_r2_ls_flags.test(R2FLAG_SUN_FOCUS))
    {
        // create clipper
        DumbClipper view_clipper;
        glm::mat4 xform = m_LightViewProj;
        view_clipper.frustum.CreateFromMatrix(*(Fmatrix*)glm::value_ptr(ex_full), FRUSTUM_P_ALL);
        view_clipper.planes.reserve(view_clipper.frustum.p_count);
        for (size_t p = 0; p < view_clipper.frustum.p_count; p++)
        {
            Fplane& P = view_clipper.frustum.planes [p];
            view_clipper.planes.emplace_back(P.n.x, P.n.y, P.n.z, P.d);
        }

        //
        Fbox3 b_casters, b_receivers;
        Fvector3 pt;

        // casters
        b_casters.invalidate();
        for (auto& s_caster : s_casters)
        {
            for (int e = 0; e < 8; e++)
            {
                s_caster.getpoint(e, pt);
                pt = wform(xform, pt);
                b_casters.modify(pt);
            }
        }

        // receivers
        b_receivers.invalidate();
        b_receivers = view_clipper.clipped_AABB(s_receivers, xform);
        Fmatrix x_project, x_full, x_full_inverse;
        {
            //x_project.build_projection	(deg2rad(Device.fFOV),Device.fASPECT,ps_r2_sun_near,ps_r2_sun_near+tweak_guaranteed_range);
            x_project.build_projection(deg2rad(Device.fFOV), Device.fASPECT,VIEWPORT_NEAR,
                                       ps_r2_sun_near + tweak_guaranteed_range);
            x_full.mul(x_project, Device.mView);
            XRMatrixInverse(&x_full_inverse, nullptr, x_full);
        }
        for (const auto& corner : sun::corners)
        {
            pt = wform(x_full_inverse, corner); // world space
            pt = wform(xform, pt); // trapezoid space
            b_receivers.modify(pt);
        }

        // some tweaking
        b_casters.grow(EPS);
        b_receivers.grow(EPS);

        // because caster points are from coarse representation only allow to "shrink" box, not grow
        // that is the same as if we first clip casters by frustum
        if (b_receivers.vMin.x < -1)
            b_receivers.vMin.x = -1;
        if (b_receivers.vMin.y < -1)
            b_receivers.vMin.y = -1;
        if (b_casters.vMin.z < 0)
            b_casters.vMin.z = 0;
        if (b_receivers.vMax.x > +1)
            b_receivers.vMax.x = +1;
        if (b_receivers.vMax.y > +1)
            b_receivers.vMax.y = +1;
        if (b_casters.vMax.z > +1)
            b_casters.vMax.z = +1;

        // refit?
        /*
        const float EPS				= 0.001f;
        D3DXMATRIX					refit;
        D3DXMatrixOrthoOffCenterLH	( &refit, b_receivers.vMin.x, b_receivers.vMax.x, b_receivers.vMin.y,
        b_receivers.vMax.y, b_casters.vMin.z-EPS, b_casters.vMax.z+EPS );
        D3DXMatrixMultiply			( &m_LightViewProj, &m_LightViewProj, &refit);
        */

        float boxWidth = b_receivers.vMax.x - b_receivers.vMin.x;
        float boxHeight = b_receivers.vMax.y - b_receivers.vMin.y;
        //  the divide by two's cancel out in the translation, but included for clarity
        float boxX = (b_receivers.vMax.x + b_receivers.vMin.x) / 2.f;
        float boxY = (b_receivers.vMax.y + b_receivers.vMin.y) / 2.f;
        glm::mat4 trapezoidUnitCube(2.f / boxWidth, 0.f, 0.f, 0.f,
                                     0.f, 2.f / boxHeight, 0.f, 0.f,
                                     0.f, 0.f, 1.f, 0.f,
                                     -2.f * boxX / boxWidth, -2.f * boxY / boxHeight, 0.f, 1.f);
        m_LightViewProj *= trapezoidUnitCube;
        //XRMatrixMultiply( &trapezoid_space, &trapezoid_space, &trapezoidUnitCube );
    }

    // Finalize & Cleanup
    sun->X.D[0].combine = *(Fmatrix*)glm::value_ptr(m_LightViewProj);
    s_receivers.clear();
    s_casters.clear();

    // Render shadow-map
    //. !!! We should clip based on shrinked frustum (again)
    {
        bool bNormal = !dsgraph.mapNormalPasses[0][0].empty() || !dsgraph.mapMatrixPasses[0][0].empty();
        bool bSpecial = !dsgraph.mapNormalPasses[1][0].empty() || !dsgraph.mapMatrixPasses[1][0].empty() ||
            !dsgraph.mapSorted.empty();
        if (bNormal || bSpecial)
        {
            RImplementation.Target->phase_smap_direct(RCache, sun, SE_SUN_FAR);
            RCache.set_xform_world(Fidentity);
            RCache.set_xform_view(Fidentity);
            RCache.set_xform_project(sun->X.D[0].combine);
            dsgraph.render_graph(0);
            sun->X.D[0].transluent = FALSE;
            if (bSpecial)
            {
                sun->X.D[0].transluent = TRUE;
                RImplementation.Target->phase_smap_direct_tsh(RCache, sun, SE_SUN_FAR);
                dsgraph.render_graph(1); // normal level, secondary priority
                dsgraph.render_sorted(); // strict-sorted geoms
            }
        }
    }

    // End SMAP-render
    {
        //		sun->svis.end					();
        dsgraph.r_pmask(true, false);
    }

    // Accumulate
    RImplementation.Target->phase_accumulator(RCache);

    if (RImplementation.Target->use_minmax_sm_this_frame())
    {
        PIX_EVENT(SE_SUN_FAR_MINMAX_GENERATE);
        RImplementation.Target->create_minmax_SM(RCache);
    }

    PIX_EVENT(SE_SUN_FAR);
    RImplementation.Target->accum_direct(RCache, SE_SUN_FAR);

    // Restore XForms
    RCache.set_xform_world(Fidentity);
    RCache.set_xform_view(Device.mView);
    RCache.set_xform_project(Device.mProject);
}

void render_sun_old::render_sun_near()
{
    // calculate view-frustum bounds in world space
    glm::mat4 ex_full_inverse;
    {
        glm::mat4 ex_project = glm::perspective(deg2rad(Device.fFOV), Device.fASPECT,VIEWPORT_NEAR, ps_r2_sun_near);
        ex_full_inverse = glm::inverse(ex_project * glm::make_mat4x4(&Device.mView.m[0][0]));
    }

    // Compute volume(s) - something like a frustum for infinite directional light
    // Also compute virtual light position and sector it is inside
    CFrustum cull_frustum;
    xr_vector<Fplane> cull_planes;
    Fvector3 cull_COP;
    glm::mat4 cull_xform;
    {
        // Lets begin from base frustum
#ifdef _DEBUG
        using t_volume = DumbConvexVolume<true>;
#else
        using t_volume = DumbConvexVolume<false>;
#endif
        t_volume hull;
        {
            hull.points.reserve(std::size(sun::corners));
            hull.polys.reserve(std::size(sun::facetable));

            for (const auto& corner : sun::corners)
            {
                Fvector3 xf = wform(ex_full_inverse, corner);
                hull.points.emplace_back(xf);
            }
            for (const auto& plane : sun::facetable)
            {
                auto& poly = hull.polys.emplace_back();
                poly.points.reserve(std::size(plane));
                for (const int pt : plane)
                    poly.points.emplace_back(pt);
            }
        }
        hull.compute_caster_model(cull_planes, sun->direction);
#ifdef _DEBUG
        for (u32 it = 0; it < cull_planes.size(); it++)
            RImplementation.Target->dbg_addplane(cull_planes[it], 0xffffffff);
#endif

        // COP - 100 km away
        cull_COP.mad(Device.vCameraPosition, sun->direction, -tweak_COP_initial_offs);

        // Create frustum for query
        cull_frustum._clear();
        for (auto& cull_plane : cull_planes)
            cull_frustum._add(cull_plane);

        // Create approximate ortho-xform
        // view: auto find 'up' and 'right' vectors
        glm::mat4 mdir_View, mdir_Project;
        glm::vec3 L_dir, L_up, L_right, L_pos;
        L_pos = glm::vec3(sun->position.x, sun->position.y, sun->position.z);
        L_dir = glm::normalize(glm::vec3(sun->direction.x, sun->direction.y, sun->direction.z));
        L_right = glm::vec3(1.f, 0.f, 0.f);
        if (_abs(glm::dot(L_right, L_dir)) > .99f) L_right = glm::vec3(0.f, 0.f, 1.f);
        L_up = glm::normalize(glm::cross(L_dir, L_right));
        L_right = glm::normalize(glm::cross(L_up, L_dir));
        mdir_View = glm::lookAt(L_pos, L_dir, L_up);

        //	Simple
        Fbox frustum_bb;
        frustum_bb.invalidate();
        for (const auto& point : hull.points)
        {
            Fvector xf = wform(mdir_View, point);
            frustum_bb.modify(xf);
        }
        Fbox& bb = frustum_bb;
        bb.grow(EPS);
        mdir_Project = glm::ortho(bb.vMin.x, bb.vMax.x, bb.vMin.y, bb.vMax.y,
                                  bb.vMin.z - tweak_ortho_xform_initial_offs, bb.vMax.z);

        // build viewport xform
        const float view_dim = float(RImplementation.o.smapsize);
        glm::mat4 m_viewport =
        {
            view_dim / 2.f, 0.0f, 0.0f, 0.0f,
            0.0f, -view_dim / 2.f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            view_dim / 2.f, view_dim / 2.f, 0.0f, 1.0f
        };
        glm::mat4 m_viewport_inv = glm::inverse(m_viewport);

        // snap view-position to pixel
        cull_xform = mdir_Project * mdir_View;
        Fvector cam_proj = wform(cull_xform, Device.vCameraPosition);
        Fvector cam_pixel = wform(m_viewport, cam_proj);
        cam_pixel.x = floorf(cam_pixel.x);
        cam_pixel.y = floorf(cam_pixel.y);
        Fvector cam_snapped = wform(m_viewport_inv, cam_pixel);
        Fvector diff;
        diff.sub(cam_snapped, cam_proj);
        glm::mat4 adjust;
        adjust = glm::translate(adjust, glm::vec3(diff.x, diff.y, diff.z));
        cull_xform *= adjust;

        // calculate scissor
        Fbox scissor;
        scissor.invalidate();
        glm::mat4 scissor_xf;
        scissor_xf = m_viewport * cull_xform;
        for (const auto& point : hull.points)
        {
            Fvector xf = wform(scissor_xf, point);
            scissor.modify(xf);
        }
        s32 limit = RImplementation.o.smapsize - 1;
        sun->X.D[0].minX = clampr(iFloor(scissor.vMin.x), 0, limit);
        sun->X.D[0].maxX = clampr(iCeil(scissor.vMax.x), 0, limit);
        sun->X.D[0].minY = clampr(iFloor(scissor.vMin.y), 0, limit);
        sun->X.D[0].maxY = clampr(iCeil(scissor.vMax.y), 0, limit);

        // full-xform
    }

    // Begin SMAP-render
    auto& dsgraph = RImplementation.get_context(context_id);
    {
        //		sun->svis.begin					();
        dsgraph.o.use_hom = false;
        dsgraph.o.phase = CRender::PHASE_SMAP;
        dsgraph.r_pmask(true, RImplementation.o.Tshadows);
        dsgraph.o.sector_id = RImplementation.get_largest_sector();
        dsgraph.o.xform = *(Fmatrix*)glm::value_ptr(cull_xform);
        dsgraph.o.view_frustum = cull_frustum;
        dsgraph.o.view_pos = cull_COP;
        dsgraph.o.mt_calculate = o.mt_calc_enabled;

        // Fill the database
        dsgraph.build_subspace();
    }

    // Finalize & Cleanup
    sun->X.D[0].combine = *(Fmatrix*)glm::value_ptr(cull_xform);

    // Render shadow-map
    //. !!! We should clip based on shrinked frustum (again)
    {
        bool bNormal = !dsgraph.mapNormalPasses[0][0].empty() || !dsgraph.mapMatrixPasses[0][0].empty();
        bool bSpecial = !dsgraph.mapNormalPasses[1][0].empty() || !dsgraph.mapMatrixPasses[1][0].empty() ||
            !dsgraph.mapSorted.empty();
        if (bNormal || bSpecial)
        {
            RImplementation.Target->phase_smap_direct(RCache, sun, SE_SUN_NEAR);
            RCache.set_xform_world(Fidentity);
            RCache.set_xform_view(Fidentity);
            RCache.set_xform_project(sun->X.D[0].combine);
            dsgraph.render_graph(0);
            if (ps_r2_ls_flags.test(R2FLAG_SUN_DETAILS))
                RImplementation.Details->Render(RCache);
            sun->X.D[0].transluent = FALSE;
            if (bSpecial)
            {
                sun->X.D[0].transluent = TRUE;
                RImplementation.Target->phase_smap_direct_tsh(RCache, sun, SE_SUN_NEAR);
                dsgraph.render_graph(1); // normal level, secondary priority
                dsgraph.render_sorted(); // strict-sorted geoms
            }
        }
    }

    // End SMAP-render
    {
        //		sun->svis.end					();
        dsgraph.r_pmask(true, false);
    }

    // Accumulate
    RImplementation.Target->phase_accumulator(RCache);

    if (RImplementation.Target->use_minmax_sm_this_frame())
    {
        PIX_EVENT(SE_SUN_NEAR_MINMAX_GENERATE);
        RImplementation.Target->create_minmax_SM(RCache);
    }

    PIX_EVENT(SE_SUN_NEAR);
    RImplementation.Target->accum_direct(RCache, SE_SUN_NEAR);

    // Restore XForms
    RCache.set_xform_world(Fidentity);
    RCache.set_xform_view(Device.mView);
    RCache.set_xform_project(Device.mProject);
}

void render_sun_old::render_sun_filtered() const
{
    if (!RImplementation.o.sunfilter)
        return;
    RImplementation.Target->phase_accumulator(RCache);
    PIX_EVENT(SE_SUN_LUMINANCE);
    RImplementation.Target->accum_direct(RCache, SE_SUN_LUMINANCE);
}
