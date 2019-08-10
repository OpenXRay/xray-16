#include "stdafx.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/IRenderable.h"
#include "Layers/xrRender/FBasicVisual.h"
#include "r3_R_sun_support.h"

#include <d3dx9math.h>

const float tweak_COP_initial_offs = 1200.f;
const float tweak_ortho_xform_initial_offs = 1000.f; //. ?
const float tweak_guaranteed_range = 20.f; //. ?

// float			OLES_SUN_LIMIT_27_01_07			= 180.f		;
float OLES_SUN_LIMIT_27_01_07 = 100.f;

const float MAP_SIZE_START = 6.f;
const float MAP_GROW_FACTOR = 4.f;

//////////////////////////////////////////////////////////////////////////
// tables to calculate view-frustum bounds in world space
// note: D3D uses [0..1] range for Z
static Fvector3 corners[8] = {
    {-1, -1, 0}, {-1, -1, +1}, {-1, +1, +1}, {-1, +1, 0}, {+1, +1, +1}, {+1, +1, 0}, {+1, -1, +1}, {+1, -1, 0}};
static int facetable[6][4] = {
    {6, 7, 5, 4}, {1, 0, 7, 6}, {1, 2, 3, 0}, {3, 2, 4, 5},
    // near and far planes
    {0, 3, 5, 7}, {1, 6, 4, 2},
};
//////////////////////////////////////////////////////////////////////////
#define DW_AS_FLT(DW) (*(FLOAT*)&(DW))
#define FLT_AS_DW(F) (*(DWORD*)&(F))
#define FLT_SIGN(F) ((FLT_AS_DW(F) & 0x80000000L))
#define ALMOST_ZERO(F) ((FLT_AS_DW(F) & 0x7f800000L) == 0)
#define IS_SPECIAL(F) ((FLT_AS_DW(F) & 0x7f800000L) == 0x7f800000L)

//////////////////////////////////////////////////////////////////////////
struct Frustum
{
    Frustum();
    Frustum(const D3DXMATRIX* matrix);

    D3DXPLANE camPlanes[6];
    int nVertexLUT[6];
    D3DXVECTOR3 pntList[8];
};
struct BoundingBox
{
    D3DXVECTOR3 minPt;
    D3DXVECTOR3 maxPt;

    BoundingBox() : minPt(1e33f, 1e33f, 1e33f), maxPt(-1e33f, -1e33f, -1e33f) {}
    BoundingBox(const BoundingBox& other) : minPt(other.minPt), maxPt(other.maxPt) {}
    explicit BoundingBox(const D3DXVECTOR3* points, UINT n) : minPt(1e33f, 1e33f, 1e33f), maxPt(-1e33f, -1e33f, -1e33f)
    {
        for (unsigned int i = 0; i < n; i++)
            Merge(&points[i]);
    }

    explicit BoundingBox(const std::vector<D3DXVECTOR3>* points)
        : minPt(1e33f, 1e33f, 1e33f), maxPt(-1e33f, -1e33f, -1e33f)
    {
        for (unsigned int i = 0; i < points->size(); i++)
            Merge(&(*points)[i]);
    }
    explicit BoundingBox(const std::vector<BoundingBox>* boxes)
        : minPt(1e33f, 1e33f, 1e33f), maxPt(-1e33f, -1e33f, -1e33f)
    {
        for (unsigned int i = 0; i < boxes->size(); i++)
        {
            Merge(&(*boxes)[i].maxPt);
            Merge(&(*boxes)[i].minPt);
        }
    }
    void Centroid(D3DXVECTOR3* vec) const { *vec = 0.5f * (minPt + maxPt); }
    void Merge(const D3DXVECTOR3* vec)
    {
        minPt.x = std::min(minPt.x, vec->x);
        minPt.y = std::min(minPt.y, vec->y);
        minPt.z = std::min(minPt.z, vec->z);
        maxPt.x = std::max(maxPt.x, vec->x);
        maxPt.y = std::max(maxPt.y, vec->y);
        maxPt.z = std::max(maxPt.z, vec->z);
    }
    D3DXVECTOR3 Point(int i) const
    {
        return D3DXVECTOR3((i & 1) ? minPt.x : maxPt.x, (i & 2) ? minPt.y : maxPt.y, (i & 4) ? minPt.z : maxPt.z);
    }
};

///////////////////////////////////////////////////////////////////////////
BOOL LineIntersection2D(D3DXVECTOR2* result, const D3DXVECTOR2* lineA, const D3DXVECTOR2* lineB)
{
    //  if the lines are parallel, the lines will not intersect in a point
    //  NOTE: assumes the rays are already normalized!!!!
    VERIFY(_abs(D3DXVec2Dot(&lineA[1], &lineB[1])) < 1.f);

    float x[2] = {lineA[0].x, lineB[0].x};
    float y[2] = {lineA[0].y, lineB[0].y};
    float dx[2] = {lineA[1].x, lineB[1].x};
    float dy[2] = {lineA[1].y, lineB[1].y};

    float x_diff = x[0] - x[1];
    float y_diff = y[0] - y[1];

    float s = (x_diff - (dx[1] / dy[1]) * y_diff) / ((dx[1] * dy[0] / dy[1]) - dx[0]);
    // float t	= (x_diff + s*dx[0]) / dx[1];

    *result = lineA[0] + s * lineA[1];
    return TRUE;
}
///////////////////////////////////////////////////////////////////////////
//  PlaneIntersection
//    computes the point where three planes intersect
//    returns whether or not the point exists.
static inline BOOL PlaneIntersection(
    D3DXVECTOR3* intersectPt, const D3DXPLANE* p0, const D3DXPLANE* p1, const D3DXPLANE* p2)
{
    D3DXVECTOR3 n0(p0->a, p0->b, p0->c);
    D3DXVECTOR3 n1(p1->a, p1->b, p1->c);
    D3DXVECTOR3 n2(p2->a, p2->b, p2->c);

    D3DXVECTOR3 n1_n2, n2_n0, n0_n1;

    D3DXVec3Cross(&n1_n2, &n1, &n2);
    D3DXVec3Cross(&n2_n0, &n2, &n0);
    D3DXVec3Cross(&n0_n1, &n0, &n1);

    float cosTheta = D3DXVec3Dot(&n0, &n1_n2);

    if (ALMOST_ZERO(cosTheta) || IS_SPECIAL(cosTheta))
        return FALSE;

    float secTheta = 1.f / cosTheta;

    n1_n2 = n1_n2 * p0->d;
    n2_n0 = n2_n0 * p1->d;
    n0_n1 = n0_n1 * p2->d;

    *intersectPt = -(n1_n2 + n2_n0 + n0_n1) * secTheta;
    return TRUE;
}

Frustum::Frustum()
{
    for (int i = 0; i < 6; i++)
        camPlanes[i] = D3DXPLANE(0.f, 0.f, 0.f, 0.f);
}

//  build a frustum from a camera (projection, or viewProjection) matrix
Frustum::Frustum(const D3DXMATRIX* matrix)
{
    //  build a view frustum based on the current view & projection matrices...
    D3DXVECTOR4 column4(matrix->_14, matrix->_24, matrix->_34, matrix->_44);
    D3DXVECTOR4 column1(matrix->_11, matrix->_21, matrix->_31, matrix->_41);
    D3DXVECTOR4 column2(matrix->_12, matrix->_22, matrix->_32, matrix->_42);
    D3DXVECTOR4 column3(matrix->_13, matrix->_23, matrix->_33, matrix->_43);

    D3DXVECTOR4 planes[6];
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
        float dot = planes[p].x * planes[p].x + planes[p].y * planes[p].y + planes[p].z * planes[p].z;
        dot = 1.f / _sqrt(dot);
        planes[p] = planes[p] * dot;
    }

    for (p = 0; p < 6; p++)
        camPlanes[p] = D3DXPLANE(planes[p].x, planes[p].y, planes[p].z, planes[p].w);

    //  build a bit-field that will tell us the indices for the nearest and farthest vertices from each plane...
    for (int i = 0; i < 6; i++)
        nVertexLUT[i] = ((planes[i].x < 0.f) ? 1 : 0) | ((planes[i].y < 0.f) ? 2 : 0) | ((planes[i].z < 0.f) ? 4 : 0);

    for (int i = 0; i < 8; i++) // compute extrema
    {
        const D3DXPLANE& p0 = (i & 1) ? camPlanes[4] : camPlanes[5];
        const D3DXPLANE& p1 = (i & 2) ? camPlanes[3] : camPlanes[2];
        const D3DXPLANE& p2 = (i & 4) ? camPlanes[0] : camPlanes[1];
        PlaneIntersection(&pntList[i], &p0, &p1, &p2);
    }
}

//////////////////////////////////////////////////////////////////////////
Fvector3 wform(Fmatrix& m, Fvector3 const& v)
{
    Fvector4 r;
    r.x = v.x * m._11 + v.y * m._21 + v.z * m._31 + m._41;
    r.y = v.x * m._12 + v.y * m._22 + v.z * m._32 + m._42;
    r.z = v.x * m._13 + v.y * m._23 + v.z * m._33 + m._43;
    r.w = v.x * m._14 + v.y * m._24 + v.z * m._34 + m._44;
    // VERIFY		(r.w>0.f);
    float invW = 1.0f / r.w;
    Fvector3 r3 = {r.x * invW, r.y * invW, r.z * invW};
    return r3;
}

void CRender::init_cacades()
{
    u32 cascade_count = 3;
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
}

void CRender::render_sun_cascades()
{
    bool b_need_to_render_sunshafts = RImplementation.Target->need_to_render_sunshafts();
    bool last_cascade_chain_mode = m_sun_cascades.back().reset_chain;
    if (b_need_to_render_sunshafts)
        m_sun_cascades[m_sun_cascades.size() - 1].reset_chain = true;

    for (u32 i = 0; i < m_sun_cascades.size(); ++i)
        render_sun_cascade(i);

    if (b_need_to_render_sunshafts)
        m_sun_cascades[m_sun_cascades.size() - 1].reset_chain = last_cascade_chain_mode;
}

void CRender::render_sun_cascade(u32 cascade_ind)
{
    light* fuckingsun = (light*)Lights.sun_adapted._get();

    // calculate view-frustum bounds in world space
    Fmatrix ex_project, ex_full, ex_full_inverse;
    {
        ex_project = Device.mProject;
        ex_full.mul(ex_project, Device.mView);
        D3DXMatrixInverse((D3DXMATRIX*)&ex_full_inverse, 0, (D3DXMATRIX*)&ex_full);
    }

    // Compute volume(s) - something like a frustum for infinite directional light
    // Also compute virtual light position and sector it is inside
    CFrustum cull_frustum;
    xr_vector<Fplane> cull_planes;
    Fvector3 cull_COP;
    CSector* cull_sector;
    Fmatrix cull_xform;
    {
        FPU::m64r();
        // Lets begin from base frustum
        Fmatrix fullxform_inv = ex_full_inverse;
#ifdef _DEBUG
        typedef DumbConvexVolume<true> t_volume;
#else
        typedef DumbConvexVolume<false> t_volume;
#endif

        //******************************* Need to be placed after cuboid built **************************
        // Search for default sector - assume "default" or "outdoor" sector is the largest one
        //. hack: need to know real outdoor sector
        CSector* largest_sector = 0;
        float largest_sector_vol = 0;
        for (u32 s = 0; s < Sectors.size(); s++)
        {
            CSector* S = (CSector*)Sectors[s];
            dxRender_Visual* V = S->root();
            float vol = V->vis.box.getvolume();
            if (vol > largest_sector_vol)
            {
                largest_sector_vol = vol;
                largest_sector = S;
            }
        }
        cull_sector = largest_sector;

        // COP - 100 km away
        cull_COP.mad(Device.vCameraPosition, fuckingsun->direction, -tweak_COP_initial_offs);

        // Create approximate ortho-xform
        // view: auto find 'up' and 'right' vectors
        Fmatrix mdir_View, mdir_Project;
        Fvector L_dir, L_up, L_right, L_pos;
        L_pos.set(fuckingsun->position);
        L_dir.set(fuckingsun->direction).normalize();
        L_right.set(1, 0, 0);
        if (_abs(L_right.dotproduct(L_dir)) > .99f)
            L_right.set(0, 0, 1);
        L_up.crossproduct(L_dir, L_right).normalize();
        L_right.crossproduct(L_up, L_dir).normalize();
        mdir_View.build_camera_dir(L_pos, L_dir, L_up);

//////////////////////////////////////////////////////////////////////////
#ifdef _DEBUG
        typedef FixedConvexVolume<true> t_cuboid;
#else
        typedef FixedConvexVolume<false> t_cuboid;
#endif

        t_cuboid light_cuboid;
        {
            // Initialize the first cascade rays, then each cascade will initialize rays for next one.
            if (cascade_ind == 0 || m_sun_cascades[cascade_ind].reset_chain)
            {
                Fvector3 near_p, edge_vec;
                for (int p = 0; p < 4; p++)
                {
                    // 					Fvector asd = Device.vCameraDirection;
                    // 					asd.mul(-2);
                    // 					asd.add(Device.vCameraPosition);
                    // 					near_p		= Device.vCameraPosition;//wform		(fullxform_inv,asd); //
                    near_p = wform(fullxform_inv, corners[facetable[4][p]]);

                    edge_vec = wform(fullxform_inv, corners[facetable[5][p]]);
                    edge_vec.sub(near_p);
                    edge_vec.normalize();

                    light_cuboid.view_frustum_rays.push_back(sun::ray(near_p, edge_vec));
                }
            }
            else
                light_cuboid.view_frustum_rays = m_sun_cascades[cascade_ind].rays;

            light_cuboid.view_ray.P = Device.vCameraPosition;
            light_cuboid.view_ray.D = Device.vCameraDirection;
            light_cuboid.light_ray.P = L_pos;
            light_cuboid.light_ray.D = L_dir;
        }

        // THIS NEED TO BE A CONSTATNT
        Fplane light_top_plane;
        light_top_plane.build_unit_normal(L_pos, L_dir);
        float dist = light_top_plane.classify(Device.vCameraPosition);

        float map_size = m_sun_cascades[cascade_ind].size;
        D3DXMatrixOrthoOffCenterLH((D3DXMATRIX*)&mdir_Project, -map_size * 0.5f, map_size * 0.5f, -map_size * 0.5f,
            map_size * 0.5f, 0.1, dist + /*sqrt(2)*/ 1.41421f * map_size);

        //////////////////////////////////////////////////////////////////////////

        /**/

        // build viewport xform
        float view_dim = float(RImplementation.o.smapsize);
        Fmatrix m_viewport = {view_dim / 2.f, 0.0f, 0.0f, 0.0f, 0.0f, -view_dim / 2.f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            0.0f, view_dim / 2.f, view_dim / 2.f, 0.0f, 1.0f};
        Fmatrix m_viewport_inv;
        D3DXMatrixInverse((D3DXMATRIX*)&m_viewport_inv, 0, (D3DXMATRIX*)&m_viewport);

        // snap view-position to pixel
        cull_xform.mul(mdir_Project, mdir_View);
        Fmatrix cull_xform_inv;
        cull_xform_inv.invert(cull_xform);

        //		light_cuboid.light_cuboid_points.reserve		(9);
        for (int p = 0; p < 8; p++)
        {
            Fvector3 xf = wform(cull_xform_inv, corners[p]);
            light_cuboid.light_cuboid_points[p] = xf;
        }

        // only side planes
        for (int plane = 0; plane < 4; plane++)
            for (int pt = 0; pt < 4; pt++)
            {
                int asd = facetable[plane][pt];
                light_cuboid.light_cuboid_polys[plane].points[pt] = asd;
            }

        Fvector lightXZshift;
        light_cuboid.compute_caster_model_fixed(
            cull_planes, lightXZshift, m_sun_cascades[cascade_ind].size, m_sun_cascades[cascade_ind].reset_chain);
        Fvector proj_view = Device.vCameraDirection;
        proj_view.y = 0;
        proj_view.normalize();
        //			lightXZshift.mad(proj_view, 20);

        // Initialize rays for the next cascade
        if (cascade_ind < m_sun_cascades.size() - 1)
            m_sun_cascades[cascade_ind + 1].rays = light_cuboid.view_frustum_rays;

        // #ifdef	_DEBUG

        static bool draw_debug = false;
        if (draw_debug && cascade_ind == 0)
            for (u32 it = 0; it < cull_planes.size(); it++)
                RImplementation.Target->dbg_addplane(cull_planes[it], it * 0xFFF);
        //#endifDDS

        Fvector cam_shifted = L_pos;
        cam_shifted.add(lightXZshift);

        // rebuild the view transform with the shift.
        mdir_View.identity();
        mdir_View.build_camera_dir(cam_shifted, L_dir, L_up);
        cull_xform.identity();
        cull_xform.mul(mdir_Project, mdir_View);
        cull_xform_inv.invert(cull_xform);

        // Create frustum for query
        cull_frustum._clear();
        for (u32 p = 0; p < cull_planes.size(); p++)
            cull_frustum._add(cull_planes[p]);

        {
            Fvector cam_proj = Device.vCameraPosition;
            const float align_aim_step_coef = 4.f;
            cam_proj.set(floorf(cam_proj.x / align_aim_step_coef) + align_aim_step_coef / 2,
                floorf(cam_proj.y / align_aim_step_coef) + align_aim_step_coef / 2,
                floorf(cam_proj.z / align_aim_step_coef) + align_aim_step_coef / 2);
            cam_proj.mul(align_aim_step_coef);
            Fvector cam_pixel = wform(cull_xform, cam_proj);
            cam_pixel = wform(m_viewport, cam_pixel);
            Fvector shift_proj = lightXZshift;
            cull_xform.transform_dir(shift_proj);
            m_viewport.transform_dir(shift_proj);

            const float align_granularity = 4.f;
            shift_proj.x = shift_proj.x > 0 ? align_granularity : -align_granularity;
            shift_proj.y = shift_proj.y > 0 ? align_granularity : -align_granularity;
            shift_proj.z = 0;

            cam_pixel.x = cam_pixel.x / align_granularity - floorf(cam_pixel.x / align_granularity);
            cam_pixel.y = cam_pixel.y / align_granularity - floorf(cam_pixel.y / align_granularity);
            cam_pixel.x *= align_granularity;
            cam_pixel.y *= align_granularity;
            cam_pixel.z = 0;

            cam_pixel.sub(shift_proj);

            m_viewport_inv.transform_dir(cam_pixel);
            cull_xform_inv.transform_dir(cam_pixel);
            Fvector diff = cam_pixel;
            static float sign_test = -1.f;
            diff.mul(sign_test);
            Fmatrix adjust;
            adjust.translate(diff);
            cull_xform.mulB_44(adjust);
        }

        m_sun_cascades[cascade_ind].xform = cull_xform;

        s32 limit = RImplementation.o.smapsize - 1;
        fuckingsun->X.D.minX = 0;
        fuckingsun->X.D.maxX = limit;
        fuckingsun->X.D.minY = 0;
        fuckingsun->X.D.maxY = limit;

        // full-xform
        FPU::m24r();
    }

    // Begin SMAP-render
    {
        bool bSpecialFull = mapNormalPasses[1][0].size() || mapMatrixPasses[1][0].size() || mapSorted.size();
        VERIFY(!bSpecialFull);
        HOM.Disable();
        phase = PHASE_SMAP;
        if (RImplementation.o.Tshadows)
            r_pmask(true, true);
        else
            r_pmask(true, false);
        //		fuckingsun->svis.begin					();
    }

    // Fill the database
    r_dsgraph_render_subspace(cull_sector, &cull_frustum, cull_xform, cull_COP, TRUE);

    // Finalize & Cleanup
    fuckingsun->X.D.combine = cull_xform; //*((Fmatrix*)&m_LightViewProj);

    // Render shadow-map
    //. !!! We should clip based on shrinked frustum (again)
    {
        bool bNormal = mapNormalPasses[0][0].size() || mapMatrixPasses[0][0].size();
        bool bSpecial = mapNormalPasses[1][0].size() || mapMatrixPasses[1][0].size() || mapSorted.size();
        if (bNormal || bSpecial)
        {
            Target->phase_smap_direct(fuckingsun, SE_SUN_FAR);
            RCache.set_xform_world(Fidentity);
            RCache.set_xform_view(Fidentity);
            RCache.set_xform_project(fuckingsun->X.D.combine);
            r_dsgraph_render_graph(0);
            if (ps_r2_ls_flags.test(R2FLAG_SUN_DETAILS))
            {
                Details->SetShadowsStage(true);
                Details->Render();
            }

            fuckingsun->X.D.transluent = FALSE;
            if (bSpecial)
            {
                fuckingsun->X.D.transluent = TRUE;
                Target->phase_smap_direct_tsh(fuckingsun, SE_SUN_FAR);
                r_dsgraph_render_graph(1); // normal level, secondary priority
                r_dsgraph_render_sorted(); // strict-sorted geoms
            }
        }
    }

    // End SMAP-render
    {
        //		fuckingsun->svis.end					();
        r_pmask(true, false);
    }

    // Accumulate
    Target->phase_accumulator();

    if (Target->use_minmax_sm_this_frame())
    {
        PIX_EVENT(SE_SUN_NEAR_MINMAX_GENERATE);
        Target->create_minmax_SM();
    }

    PIX_EVENT(SE_SUN_NEAR);

    if (cascade_ind == 0)
        Target->accum_direct_cascade(SE_SUN_NEAR, m_sun_cascades[cascade_ind].xform, m_sun_cascades[cascade_ind].xform,
            m_sun_cascades[cascade_ind].bias);
    else if (cascade_ind < m_sun_cascades.size() - 1)
        Target->accum_direct_cascade(SE_SUN_MIDDLE, m_sun_cascades[cascade_ind].xform,
            m_sun_cascades[cascade_ind - 1].xform, m_sun_cascades[cascade_ind].bias);
    else
        Target->accum_direct_cascade(SE_SUN_FAR, m_sun_cascades[cascade_ind].xform,
            m_sun_cascades[cascade_ind - 1].xform, m_sun_cascades[cascade_ind].bias);

    // Restore XForms
    RCache.set_xform_world(Fidentity);
    RCache.set_xform_view(Device.mView);
    RCache.set_xform_project(Device.mProject);
}
