#include "stdafx.h"
#include "r2_R_sun_support.h"

namespace xray::render::fg
{
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
    sun = (light*)Lights.sun._get();

    const Fcolor sun_color = sun->color;
    o.active = ps_r2_ls_flags.test(R2FLAG_SUN) && (u_diffuse2s(sun_color.r, sun_color.g, sun_color.b) > EPS);
    if (RImplementation.o.sunstatic)
        o.active = false;

    if (!o.active)
        return;

    o.mt_calc_enabled = false;

    // pre-allocate context
    context_id = RImplementation.alloc_context();
    VERIFY(context_id != R_dsgraph_structure::INVALID_CONTEXT_ID);
}

void render_sun_old::render_sun()
{
    PIX_EVENT(render_sun);
    XMMATRIX m_LightViewProj;

    // calculate view-frustum bounds in world space
    Fmatrix ex_project, ex_full;
    XMMATRIX ex_full_inverse;
    {
        float _far_ = std::min(ps_r2_sun_far, g_pGamePersistent->Environment().CurrentEnv.far_plane);
        // ex_project.build_projection	(deg2rad(Device.fFOV/* *Device.fASPECT*/),Device.fASPECT,ps_r2_sun_near,_far_);
        ex_project.build_projection(deg2rad(Device.fFOV /* *Device.fASPECT*/), Device.fASPECT, VIEWPORT_NEAR, _far_);
        // VIEWPORT_NEAR
        ex_full.mul(ex_project, Device.mView);
        ex_full_inverse = XMMatrixInverse(nullptr, XMLoadFloat4x4((XMFLOAT4X4*)&ex_full));
    }

    // Compute volume(s) - something like a frustum for infinite directional light
    // Also compute virtual light position and sector it is inside
    CFrustum cull_frustum;
    xr_vector<Fplane> cull_planes;
    Fvector3 cull_COP;
    Fmatrix cull_xform;
    {
        // Lets begin from base frustum
        Fmatrix fullxform_inv;
        XMStoreFloat4x4((XMFLOAT4X4*)&fullxform_inv, ex_full_inverse);
        DumbConvexVolume<false> hull;
        {
            hull.points.reserve(std::size(sun::corners));
            hull.polys.reserve(std::size(sun::facetable));

            for (const auto& corner : sun::corners)
            {
                Fvector3 xf = wform(fullxform_inv, corner);
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

        // COP - 100 km away
        cull_COP.mad(Device.vCameraPosition, sun->direction, -tweak_COP_initial_offs);

        // Create frustum for query
        cull_frustum._clear();
        for (u32 p = 0; p < cull_planes.size(); p++)
            cull_frustum._add(cull_planes[p]);

        // Create approximate ortho-xform
        // view: auto find 'up' and 'right' vectors
        Fmatrix mdir_View, mdir_Project;
        Fvector L_dir, L_up, L_right, L_pos;
        L_pos.set(sun->position);
        L_dir.set(sun->direction).normalize();
        L_up.set(0, 1, 0);
        if (_abs(L_up.dotproduct(L_dir)) > .99f)
            L_up.set(0, 0, 1);
        L_right.crossproduct(L_up, L_dir).normalize();
        L_up.crossproduct(L_dir, L_right).normalize();
        mdir_View.build_camera_dir(L_pos, L_dir, L_up);

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
        XMStoreFloat4x4((XMFLOAT4X4*)&mdir_Project, XMMatrixOrthographicOffCenterLH(bb.vMin.x, bb.vMax.x, bb.vMin.y, bb.vMax.y,
            bb.vMin.z - tweak_ortho_xform_initial_offs, bb.vMax.z));

        // full-xform
        cull_xform.mul(mdir_Project, mdir_View);
    }

    // Begin SMAP-render
    xr_vector<Fbox3>& s_receivers = g_main_coarse_structure;
    s_casters.reserve(s_receivers.size());

    auto& dsgraph = RImplementation.get_context(context_id);
    {
        //		sun->svis.begin					();
        dsgraph.o.phase = CRender::PHASE_SMAP;
        dsgraph.r_pmask(true, RImplementation.o.Tshadows);
        dsgraph.o.sector_id = g_largest_sector_id;
        dsgraph.o.xform = cull_xform;
        dsgraph.o.view_frustum = cull_frustum;
        dsgraph.o.view_pos = cull_COP;
        dsgraph.set_Recorder(&s_casters);

        // Fill the database
        dsgraph.build_subspace();

        // IGNORE PORTALS
        if (ps_r2_ls_flags.test(R2FLAG_SUN_IGNORE_PORTALS))
        {
            for (u32 s = 0; s < dsgraph.Sectors.size(); s++)
            {
                CSector* S = dsgraph.Sectors[s];
                dxRender_Visual* root = S->root();
                dsgraph.add_static(root, cull_frustum, cull_frustum.getMask());
            }
        }
        dsgraph.set_Recorder(nullptr);
    }

    //	Prepare to interact with D3DX code
    const XMMATRIX m_View = XMLoadFloat4x4((XMFLOAT4X4*)(&Device.mView));
    const XMFLOAT3 m_lightDir =
        XMFLOAT3(-sun->direction.x, -sun->direction.y, -sun->direction.z);

    //  these are the limits specified by the physical camera
    //  gamma is the "tilt angle" between the light and the view direction.
    float m_fCosGamma = m_lightDir.x * Device.mView._13 + m_lightDir.y * Device.mView._23 + m_lightDir.z * Device.mView._33;
    float m_fTSM_Delta = ps_r2_sun_tsm_projection;

    // Compute REAL sheared xform based on receivers/casters information
    if (_abs(m_fCosGamma) < 0.99f && ps_r2_ls_flags.test(R2FLAG_SUN_TSM))
    {
        //  get the near and the far plane (points) in eye space.
        XMFLOAT3 frustumPnts[8];

        Frustum eyeFrustum((XMFLOAT4X4*)&ex_project); // autocomputes all the extrema points

        for (int i = 0; i < 4; i++)
        {
            frustumPnts[i] = eyeFrustum.pntList[(i << 1)]; // far plane
            frustumPnts[i + 4] = eyeFrustum.pntList[(i << 1) | 0x1]; // near plane
        }

        //   we need to transform the eye into the light's post-projective space.
        //   however, the sun is a directional light, so we first need to find an appropriate
        //   rotate/translate matrix, before constructing an ortho projection.
        //   this matrix is a variant of "light space" from LSPSMs, with the Y and Z axes permuted

        XMVECTOR eyeVector = XMVectorSet(0.f, 0.f, -1.f, 0.f); //  eye is always -Z in eye space

        //  code copied straight from BuildLSPSMProjectionMatrix
        XMVECTOR upVector = XMVector3TransformNormal(XMLoadFloat3(&m_lightDir), m_View); // lightDir is defined in eye space, so xform it
        XMVECTOR leftVector = XMVector3Normalize(XMVector3Cross(upVector, eyeVector));
        XMVECTOR viewVector = XMVector3Cross(upVector, leftVector);

        XMMATRIX lightSpaceBasis;
        lightSpaceBasis.r[0] = leftVector;
        lightSpaceBasis.r[1] = viewVector;
        lightSpaceBasis.r[2] = XMVectorNegate(upVector);
        lightSpaceBasis.r[3] = XMVectorSet(0.f, 0.f, 0.f, 1.f);
        lightSpaceBasis = XMMatrixTranspose(lightSpaceBasis);

        //  rotate the view frustum into light space
        XMVector3TransformCoordStream(frustumPnts, sizeof(XMFLOAT3), frustumPnts, sizeof(XMFLOAT3), std::size(frustumPnts), lightSpaceBasis);

        //  build an off-center ortho projection that translates and scales the eye frustum's 3D AABB to the unit cube
        BoundingBox frustumBox(frustumPnts, std::size(frustumPnts));

        //  also - transform the shadow caster bounding boxes into light projective space.  we want to translate along
        //  the Z axis so that
        //  all shadow casters are in front of the near plane.
        XMFLOAT2 depthbounds = BuildTSMProjectionMatrix_caster_depth_bounds(lightSpaceBasis, s_casters);

        float min_z = std::min(depthbounds.x, frustumBox.minPt.z);
        float max_z = std::max(depthbounds.y, frustumBox.maxPt.z);

        if (min_z <= 1.f) //?
        {
            XMMATRIX lightSpaceTranslate = XMMatrixTranslation(0.f, 0.f, -min_z + 1.f);
            max_z = -min_z + max_z + 1.f;
            min_z = 1.f;
            lightSpaceBasis = XMMatrixMultiply(lightSpaceBasis, lightSpaceTranslate);
            XMVector3TransformCoordStream(frustumPnts, sizeof(XMFLOAT3), frustumPnts, sizeof(XMFLOAT3), std::size(frustumPnts), lightSpaceTranslate);
            frustumBox = BoundingBox(frustumPnts, std::size(frustumPnts));
        }

        XMMATRIX lightSpaceOrtho = XMMatrixOrthographicOffCenterLH(frustumBox.minPt.x, frustumBox.maxPt.x, frustumBox.minPt.y,
            frustumBox.maxPt.y, min_z, max_z);

        //  transform the view frustum by the new matrix
        XMVector3TransformCoordStream(frustumPnts, sizeof(XMFLOAT3), frustumPnts, sizeof(XMFLOAT3), std::size(frustumPnts), lightSpaceOrtho);

        XMFLOAT2 centerPts[2];
        //  near plane
        centerPts[0].x = 0.25f * (frustumPnts[4].x + frustumPnts[5].x + frustumPnts[6].x + frustumPnts[7].x);
        centerPts[0].y = 0.25f * (frustumPnts[4].y + frustumPnts[5].y + frustumPnts[6].y + frustumPnts[7].y);
        //  far plane
        centerPts[1].x = 0.25f * (frustumPnts[0].x + frustumPnts[1].x + frustumPnts[2].x + frustumPnts[3].x);
        centerPts[1].y = 0.25f * (frustumPnts[0].y + frustumPnts[1].y + frustumPnts[2].y + frustumPnts[3].y);

        XMFLOAT2 centerOrig((centerPts[0].x + centerPts[1].x) * 0.5f, (centerPts[0].y + centerPts[1].y) * 0.5f);

        XMMATRIX trapezoid_space;

        XMMATRIX xlate_center(
            1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, -centerOrig.x, -centerOrig.y, 0.f, 1.f);

        XMVECTOR center_dirl = XMLoadFloat2(&centerPts[1]) - XMLoadFloat2(&centerOrig);
        float half_center_len = XMVectorGetX(XMVector2Length(center_dirl));
        float x_len = centerPts[1].x - centerOrig.x;
        float y_len = centerPts[1].y - centerOrig.y;

        float cos_theta = x_len / half_center_len;
        float sin_theta = y_len / half_center_len;

        XMMATRIX rot_center(
            cos_theta, -sin_theta, 0.f, 0.f, sin_theta, cos_theta, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);

        //  this matrix transforms the center line to y=0.
        //  since Top and Base are orthogonal to Center, we can skip computing the convex hull, and instead
        //  just find the view frustum X-axis extrema.  The most negative is Top, the most positive is Base
        //  Point Q (trapezoid projection point) will be a point on the y=0 line.
        trapezoid_space = XMMatrixMultiply(xlate_center, rot_center);
        XMVector3TransformCoordStream(frustumPnts, sizeof(XMFLOAT3), frustumPnts, sizeof(XMFLOAT3), std::size(frustumPnts), trapezoid_space);

        BoundingBox frustumAABB2D(frustumPnts, std::size(frustumPnts));

        float x_scale = std::max(_abs(frustumAABB2D.maxPt.x), _abs(frustumAABB2D.minPt.x));
        float y_scale = std::max(_abs(frustumAABB2D.maxPt.y), _abs(frustumAABB2D.minPt.y));
        x_scale = 1.f / x_scale;
        y_scale = 1.f / y_scale;

        //  maximize the area occupied by the bounding box
        XMMATRIX scale_center(x_scale, 0.f, 0.f, 0.f, 0.f, y_scale, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);

        trapezoid_space = XMMatrixMultiply(trapezoid_space, scale_center);

        //  scale the frustum AABB up by these amounts (keep all values in the same space)
        frustumAABB2D.minPt.x *= x_scale;
        frustumAABB2D.maxPt.x *= x_scale;
        frustumAABB2D.minPt.y *= y_scale;
        frustumAABB2D.maxPt.y *= y_scale;

        //  compute eta.
        float lambda = frustumAABB2D.maxPt.x - frustumAABB2D.minPt.x;
        float delta_proj = m_fTSM_Delta * lambda; // focusPt.x - frustumAABB2D.minPt.x;
        const float xi = -0.6f; // - 0.6f;  // 80% line
        float eta = (lambda * delta_proj * (1.f + xi)) / (lambda * (1.f - xi) - 2.f * delta_proj);

        //  compute the projection point a distance eta from the top line.  this point is on the center line, y=0
        XMFLOAT2 projectionPtQ(frustumAABB2D.maxPt.x + eta, 0.f);

        //  find the maximum slope from the projection point to any point in the frustum.  this will be the
        //  projection field-of-view
        float max_slope = -1e32f;
        float min_slope = 1e32f;

        for (const auto& frustumPnt : frustumPnts)
        {
            XMFLOAT2 tmp(frustumPnt.x * x_scale, frustumPnt.y * y_scale);
            float x_dist = tmp.x - projectionPtQ.x;
            if (!(ALMOST_ZERO(tmp.y) || ALMOST_ZERO(x_dist)))
            {
                max_slope = std::max(max_slope, tmp.y / x_dist);
                min_slope = std::min(min_slope, tmp.y / x_dist);
            }
        }

        float xn = eta;
        float xf = lambda + eta;

        XMMATRIX ptQ_xlate(
            -1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, projectionPtQ.x, 0.f, 0.f, 1.f);
        trapezoid_space = XMMatrixMultiply(trapezoid_space, ptQ_xlate);

        //  this shear balances the "trapezoid" around the y=0 axis (no change to the projection pt position)
        //  since we are redistributing the trapezoid, this affects the projection field of view (shear_amt)
        float shear_amt = (max_slope + _abs(min_slope)) * 0.5f - max_slope;
        max_slope = max_slope + shear_amt;

        XMMATRIX trapezoid_shear(
            1.f, shear_amt, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);

        trapezoid_space = XMMatrixMultiply(trapezoid_space, trapezoid_shear);

        float z_aspect = (frustumBox.maxPt.z - frustumBox.minPt.z) / (frustumAABB2D.maxPt.y - frustumAABB2D.minPt.y);

        //  perform a 2DH projection to 'unsqueeze' the top line.
        XMMATRIX trapezoid_projection(xf / (xf - xn), 0.f, 0.f, 1.f, 0.f, 1.f / max_slope, 0.f, 0.f, 0.f, 0.f,
            1.f / (z_aspect * max_slope), 0.f, -xn * xf / (xf - xn), 0.f, 0.f, 0.f);

        trapezoid_space = XMMatrixMultiply(trapezoid_space, trapezoid_projection);

        //  the x axis is compressed to [0..1] as a result of the projection, so expand it to [-1,1]
        XMMATRIX biasedScaleX(2.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, -1.f, 0.f, 0.f, 1.f);
        trapezoid_space = XMMatrixMultiply(trapezoid_space, biasedScaleX);

        m_LightViewProj = XMMatrixMultiply(m_View, lightSpaceBasis);
        m_LightViewProj = XMMatrixMultiply(m_LightViewProj, lightSpaceOrtho);
        m_LightViewProj = XMMatrixMultiply(m_LightViewProj, trapezoid_space);
    }
    else
    {
        m_LightViewProj = XMLoadFloat4x4((XMFLOAT4X4*)(&cull_xform));
    }

    // perform "refit" or "focusing" on relevant
    if (ps_r2_ls_flags.test(R2FLAG_SUN_FOCUS))
    {
        // create clipper
        DumbClipper view_clipper;
        Fmatrix xform;
        XMStoreFloat4x4((XMFLOAT4X4*)&xform, m_LightViewProj);
        view_clipper.frustum.CreateFromMatrix(ex_full, FRUSTUM_P_ALL);
        for (int p = 0; p < view_clipper.frustum.p_count; p++)
        {
            Fplane& P = view_clipper.frustum.planes[p];
            view_clipper.planes.emplace_back(XMFLOAT4(P.n.x, P.n.y, P.n.z, P.d));
        }

        //
        Fbox3 b_casters, b_receivers;
        Fvector3 pt;

        // casters
        b_casters.invalidate();
        for (u32 c = 0; c < s_casters.size(); c++)
        {
            for (int e = 0; e < 8; e++)
            {
                s_casters[c].getpoint(e, pt);
                pt = wform(xform, pt);
                b_casters.modify(pt);
            }
        }

        // receivers
        b_receivers.invalidate();
        b_receivers = view_clipper.clipped_AABB(s_receivers, xform);
        Fmatrix x_project, x_full, x_full_inverse;
        {
            //x_project.build_projection(deg2rad(Device.fFOV /* *Device.fASPECT*/), Device.fASPECT, ps_r2_sun_near,
            //    ps_r2_sun_near + tweak_guaranteed_range);
            x_project.build_projection(deg2rad(Device.fFOV /* *Device.fASPECT*/), Device.fASPECT, VIEWPORT_NEAR,
                ps_r2_sun_near + tweak_guaranteed_range);
            x_full.mul(x_project, Device.mView);
            XMStoreFloat4x4((XMFLOAT4X4*)&x_full_inverse,
                XMMatrixInverse(nullptr, XMLoadFloat4x4((XMFLOAT4X4*)&x_full)));
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
        XMMATRIX trapezoidUnitCube(2.f / boxWidth, 0.f, 0.f, 0.f, 0.f, 2.f / boxHeight, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f,
            -2.f * boxX / boxWidth, -2.f * boxY / boxHeight, 0.f, 1.f);
        m_LightViewProj = XMMatrixMultiply(m_LightViewProj, trapezoidUnitCube);
        // D3DXMatrixMultiply( &trapezoid_space, &trapezoid_space, &trapezoidUnitCube );
    }

    // Finalize & Cleanup
    XMStoreFloat4x4((XMFLOAT4X4*)&sun->X.D[SE_SUN_FAR].combine, m_LightViewProj);
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
            RImplementation.Target->phase_smap_direct(dsgraph.cmd_list, sun, SE_SUN_FAR);
            dsgraph.cmd_list.set_xform_world(Fidentity);
            dsgraph.cmd_list.set_xform_view(Fidentity);
            dsgraph.cmd_list.set_xform_project(sun->X.D[SE_SUN_FAR].combine);
            dsgraph.render_graph(0);
            sun->X.D[SE_SUN_FAR].transluent = FALSE;
            if (bSpecial)
            {
                sun->X.D[SE_SUN_FAR].transluent = TRUE;
                RImplementation.Target->phase_smap_direct_tsh(dsgraph.cmd_list, sun, SE_SUN_FAR);
                dsgraph.render_graph(1); // normal level, secondary priority
                dsgraph.render_sorted(); // strict-sorted geoms
            }
        }
    }

    // Accumulate
    RImplementation.Target->phase_accumulator(dsgraph.cmd_list);

    if (RImplementation.Target->use_minmax_sm_this_frame())
    {
        PIX_EVENT(SE_SUN_FAR_MINMAX_GENERATE);
        RImplementation.Target->create_minmax_SM(dsgraph.cmd_list);
    }

    PIX_EVENT(SE_SUN_FAR);
    RImplementation.Target->rt_smap_depth->set_slice_read(SE_SUN_FAR);
    RImplementation.Target->accum_direct(dsgraph.cmd_list, SE_SUN_FAR);

    // Restore XForms
    dsgraph.cmd_list.set_xform_world(Fidentity);
    dsgraph.cmd_list.set_xform_view(Device.mView);
    dsgraph.cmd_list.set_xform_project(Device.mProject);
}

void render_sun_old::render_sun_near()
{
    // calculate view-frustum bounds in world space
    Fmatrix ex_project, ex_full;
    XMMATRIX ex_full_inverse;
    {
        ex_project.build_projection(
            deg2rad(Device.fFOV /* *Device.fASPECT*/), Device.fASPECT, VIEWPORT_NEAR, ps_r2_sun_near);
        ex_full.mul(ex_project, Device.mView);
        ex_full_inverse = XMMatrixInverse(nullptr, XMLoadFloat4x4((XMFLOAT4X4*)&ex_full));
    }

    // Compute volume(s) - something like a frustum for infinite directional light
    // Also compute virtual light position and sector it is inside
    CFrustum cull_frustum;
    xr_vector<Fplane> cull_planes;
    Fvector3 cull_COP;
    Fmatrix cull_xform;
    {
        // Lets begin from base frustum
        Fmatrix fullxform_inv;
        XMStoreFloat4x4((XMFLOAT4X4*)&fullxform_inv, ex_full_inverse);
#ifdef _DEBUG
        typedef DumbConvexVolume<true> t_volume;
#else
        typedef DumbConvexVolume<false> t_volume;
#endif
        t_volume hull;
        {
            hull.points.reserve(std::size(sun::corners));
            hull.polys.reserve(std::size(sun::facetable));

            for (const auto& corner : sun::corners)
            {
                Fvector3 xf = wform(fullxform_inv, corner);
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
#ifdef _DEBUG
        for (u32 it = 0; it < cull_planes.size(); it++)
            RImplementation.Target->dbg_addplane(cull_planes[it], 0xffffffff);
#endif

        // COP - 100 km away
        cull_COP.mad(Device.vCameraPosition, sun->direction, -tweak_COP_initial_offs);

        // Create frustum for query
        cull_frustum._clear();
        for (u32 p = 0; p < cull_planes.size(); p++)
            cull_frustum._add(cull_planes[p]);

        // Create approximate ortho-xform
        // view: auto find 'up' and 'right' vectors
        Fmatrix mdir_View, mdir_Project;
        Fvector L_dir, L_up, L_right, L_pos;
        L_pos.set(sun->position);
        L_dir.set(sun->direction).normalize();
        L_right.set(1, 0, 0);
        if (_abs(L_right.dotproduct(L_dir)) > .99f)
            L_right.set(0, 0, 1);
        L_up.crossproduct(L_dir, L_right).normalize();
        L_right.crossproduct(L_up, L_dir).normalize();
        mdir_View.build_camera_dir(L_pos, L_dir, L_up);

        // projection: box
        /*
        //	Original
        float	_D					= ps_r2_sun_near;
        float	a0					= deg2rad(Device.fFOV*Device.fASPECT)/2.f;
        float	a1					= deg2rad(Device.fFOV)/2.f;
        float	c0					= _D/_cos(a0);
        float	c1					= _D/_cos(a1);
        float	k0					= 2.f*c0*_sin(a0);
        float	k1					= 2.f*c1*_sin(a1);
        float	borderalpha			= (Device.fFOV-10) / (90-10);

        float	nearborder			= 1*borderalpha + 1.136363636364f*(1-borderalpha);
        float	spherical_range		= ps_r2_sun_near_border * nearborder * _max(_max(c0,c1),
        _max(k0,k1)*1.414213562373f );
        Fbox	frustum_bb;			frustum_bb.invalidate	();
        hull.points.emplace_back		(Device.vCameraPosition);
        for (const auto& point : hull.points)	{
        Fvector	xf	= wform		(mdir_View,point);
        frustum_bb.modify		(xf);
        }
        float	size_x				= frustum_bb.vMax.x - frustum_bb.vMin.x;
        float	size_y				= frustum_bb.vMax.y - frustum_bb.vMin.y;
        float	diff_x				= (spherical_range - size_x)/2.f;	//VERIFY(diff_x>=0);
        float	diff_y				= (spherical_range - size_y)/2.f;	//VERIFY(diff_y>=0);
        frustum_bb.vMin.x -= diff_x; frustum_bb.vMax.x += diff_x;
        frustum_bb.vMin.y -= diff_y; frustum_bb.vMax.y += diff_y;
        Fbox&	bb					= frustum_bb;
        D3DXMatrixOrthoOffCenterLH	((D3DXMATRIX*)&mdir_Project,bb.vMin.x,bb.vMax.x,  bb.vMin.y,bb.vMax.y,
        bb.vMin.z-tweak_ortho_xform_initial_offs,bb.vMax.z);
        */

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
        XMStoreFloat4x4((XMFLOAT4X4*)&mdir_Project, XMMatrixOrthographicOffCenterLH(
            bb.vMin.x, bb.vMax.x, bb.vMin.y, bb.vMax.y,
            bb.vMin.z - tweak_ortho_xform_initial_offs, bb.vMax.z));

        // build viewport xform
        float view_dim = float(RImplementation.o.smapsize);
        Fmatrix m_viewport =
        {
            view_dim / 2.f, 0.0f, 0.0f, 0.0f,
            0.0f, -view_dim / 2.f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            view_dim / 2.f, view_dim / 2.f, 0.0f, 1.0f
        };
        Fmatrix m_viewport_inv;
        XMStoreFloat4x4((XMFLOAT4X4*)&m_viewport_inv,
            XMMatrixInverse(nullptr, XMLoadFloat4x4((XMFLOAT4X4*)&m_viewport)));

        // snap view-position to pixel
        cull_xform.mul(mdir_Project, mdir_View);
        Fvector cam_proj = wform(cull_xform, Device.vCameraPosition);
        Fvector cam_pixel = wform(m_viewport, cam_proj);
        cam_pixel.x = floorf(cam_pixel.x);
        cam_pixel.y = floorf(cam_pixel.y);
        Fvector cam_snapped = wform(m_viewport_inv, cam_pixel);
        Fvector diff;
        diff.sub(cam_snapped, cam_proj);
        Fmatrix adjust;
        adjust.translate(diff);
        cull_xform.mulA_44(adjust);

        // calculate scissor
        Fbox scissor;
        scissor.invalidate();
        Fmatrix scissor_xf;
        scissor_xf.mul(m_viewport, cull_xform);
        for (const auto& point : hull.points)
        {
            Fvector xf = wform(scissor_xf, point);
            scissor.modify(xf);
        }
        s32 limit = RImplementation.o.smapsize - 1;
        sun->X.D[SE_SUN_NEAR].minX = clampr(iFloor(scissor.vMin.x), 0, limit);
        sun->X.D[SE_SUN_NEAR].maxX = clampr(iCeil(scissor.vMax.x), 0, limit);
        sun->X.D[SE_SUN_NEAR].minY = clampr(iFloor(scissor.vMin.y), 0, limit);
        sun->X.D[SE_SUN_NEAR].maxY = clampr(iCeil(scissor.vMax.y), 0, limit);
        sun->X.D[SE_SUN_NEAR].combine = cull_xform;

        // full-xform
    }

    // Begin SMAP-render
    auto& dsgraph = RImplementation.get_context(context_id);
    {
        //		sun->svis.begin					();
        dsgraph.o.use_hom = false;
        dsgraph.o.phase = CRender::PHASE_SMAP;
        dsgraph.r_pmask(true, RImplementation.o.Tshadows);
        dsgraph.o.sector_id = g_largest_sector_id;
        dsgraph.o.xform = cull_xform;
        dsgraph.o.view_frustum = cull_frustum;
        dsgraph.o.view_pos = cull_COP;
        dsgraph.o.mt_calculate = o.mt_calc_enabled;

        // Fill the database
        dsgraph.build_subspace();
    }

    // Render shadow-map
    //. !!! We should clip based on shrinked frustum (again)
    {
        bool bNormal = !dsgraph.mapNormalPasses[0][0].empty() || !dsgraph.mapMatrixPasses[0][0].empty();
        bool bSpecial = !dsgraph.mapNormalPasses[1][0].empty() || !dsgraph.mapMatrixPasses[1][0].empty() ||
            !dsgraph.mapSorted.empty();
        if (bNormal || bSpecial)
        {
            RImplementation.Target->phase_smap_direct(dsgraph.cmd_list, sun, SE_SUN_NEAR);
            dsgraph.cmd_list.set_xform_world(Fidentity);
            dsgraph.cmd_list.set_xform_view(Fidentity);
            dsgraph.cmd_list.set_xform_project(sun->X.D[SE_SUN_NEAR].combine);
            dsgraph.render_graph(0);
            if (ps_r2_ls_flags.test(R2FLAG_SUN_DETAILS))
                g_pDetailManager->Render(dsgraph.cmd_list);
            sun->X.D[SE_SUN_NEAR].transluent = FALSE;
            if (bSpecial)
            {
                sun->X.D[SE_SUN_NEAR].transluent = TRUE;
                RImplementation.Target->phase_smap_direct_tsh(dsgraph.cmd_list, sun, SE_SUN_NEAR);
                dsgraph.render_graph(1); // normal level, secondary priority
                dsgraph.render_sorted(); // strict-sorted geoms
            }
        }
    }

    // Accumulate
    RImplementation.Target->phase_accumulator(dsgraph.cmd_list);

    if (RImplementation.Target->use_minmax_sm_this_frame())
    {
        PIX_EVENT(SE_SUN_NEAR_MINMAX_GENERATE);
        RImplementation.Target->create_minmax_SM(dsgraph.cmd_list);
    }

    PIX_EVENT(SE_SUN_NEAR);
    RImplementation.Target->rt_smap_depth->set_slice_read(SE_SUN_NEAR);
    RImplementation.Target->accum_direct(dsgraph.cmd_list, SE_SUN_NEAR);

    // Restore XForms
    dsgraph.cmd_list.set_xform_world(Fidentity);
    dsgraph.cmd_list.set_xform_view(Device.mView);
    dsgraph.cmd_list.set_xform_project(Device.mProject);
}

void render_sun_old::render_sun_filtered() const
{
    if (!RImplementation.o.sunfilter)
        return;
    auto& dsgraph = RImplementation.get_context(context_id);
    RImplementation.Target->phase_accumulator(dsgraph.cmd_list);
    PIX_EVENT(SE_SUN_LUMINANCE);
    RImplementation.Target->accum_direct(dsgraph.cmd_list, SE_SUN_LUMINANCE);
}

void render_sun_old::render()
{
    if (!o.active)
        return;

    render_sun_near();
    render_sun();
    render_sun_filtered();
}

void render_sun_old::flush()
{
    if (!o.active)
        return;

    auto& dsgraph = RImplementation.get_context(context_id);
    dsgraph.cmd_list.submit();
    RImplementation.release_context(context_id);
    RImplementation.get_imm_command_list().Invalidate();
}
} // namespace xray::render::fg
