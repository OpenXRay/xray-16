#include "stdafx.h"
#include "Layers/xrRender/du_cone.h"

// extern Fvector du_cone_vertices[DU_CONE_NUMVERTEX];

void CRenderTarget::accum_spot(CBackend& cmd_list, light* L)
{
    phase_accumulator(cmd_list);
    RImplementation.Stats.l_visible++;

    // *** assume accumulator already setup ***
    // *****************************	Mask by stencil		*************************************
    ref_shader shader;
    ref_shader* shader_msaa;
    if (IRender_Light::OMNIPART == L->flags.type)
    {
        shader = L->s_point;
        shader_msaa = L->s_point_msaa;
        if (!shader)
        {
            shader = s_accum_point;
            shader_msaa = s_accum_point_msaa;
        }
    }
    else
    {
        shader = L->s_spot;
        shader_msaa = L->s_spot_msaa;
        if (!shader)
        {
            shader = s_accum_spot;
            shader_msaa = s_accum_spot_msaa;
        }
    }

    {
        // setup xform
        L->xform_calc();
        cmd_list.set_xform_world(L->m_xform);
        cmd_list.set_xform_view(Device.mView);
        cmd_list.set_xform_project(Device.mProject);
        enable_scissor(L);
        enable_dbt_bounds(L);

        // *** similar to "Carmack's reverse", but assumes convex, non intersecting objects,
        // *** thus can cope without stencil clear with 127 lights
        // *** in practice, 'cause we "clear" it back to 0x1 it usually allows us to > 200 lights :)
        //	Done in blender!
        // cmd_list.set_ColorWriteEnable		(FALSE);
        cmd_list.set_Element(s_accum_mask->E[SE_MASK_SPOT]); // masker

        // backfaces: if (stencil>=1 && zfail)			stencil = light_id
        cmd_list.set_CullMode(CULL_CW);
        if (!RImplementation.o.msaa)
        {
            cmd_list.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0x01, 0xff,
                D3DSTENCILOP_KEEP, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE);
        }
        else
        {
            cmd_list.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0x01, 0x7f,
                D3DSTENCILOP_KEEP, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE);
        }
        draw_volume(cmd_list, L);

        // frontfaces: if (stencil>=light_id && zfail)	stencil = 0x1
        cmd_list.set_CullMode(CULL_CCW);
        if (!RImplementation.o.msaa)
        {
            cmd_list.set_Stencil(TRUE, D3DCMP_LESSEQUAL, 0x01, 0xff, 0xff,
                D3DSTENCILOP_KEEP, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE);
        }
        else
        {
            cmd_list.set_Stencil(TRUE, D3DCMP_LESSEQUAL, 0x01, 0x7f, 0x7f,
                D3DSTENCILOP_KEEP, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE);
        }
        draw_volume(cmd_list, L);
    }

    // nv-stencil recompression
    if (RImplementation.o.nvstencil)
        u_stencil_optimize(cmd_list);

    // *****************************	Minimize overdraw	*************************************
    // Select shader (front or back-faces), *** back, if intersect near plane
    cmd_list.set_ColorWriteEnable();
    cmd_list.set_CullMode(CULL_CW); // back

    // 2D texgens
    Fmatrix m_Texgen;
    u_compute_texgen_screen(cmd_list, m_Texgen);
    Fmatrix m_Texgen_J;
    u_compute_texgen_jitter(cmd_list, m_Texgen_J);

    // Shadow xform (+texture adjustment matrix)
    Fmatrix m_Shadow, m_Lmap;
    {
        float smapsize = float(RImplementation.o.smapsize);
        float fTexelOffs = (.5f / smapsize);
        float view_dim = float(L->X.S.size - 2) / smapsize;
        float view_sx = float(L->X.S.posX + 1) / smapsize;
        float view_sy = float(L->X.S.posY + 1) / smapsize;
        float fRange = float(1.f) * ps_r2_ls_depth_scale;
        float fBias = ps_r2_ls_depth_bias;
#ifdef USE_DX11
        Fmatrix m_TexelAdjust =
        {
            view_dim / 2.f, 0.0f, 0.0f, 0.0f,
            0.0f, -view_dim / 2.f, 0.0f, 0.0f,
            0.0f, 0.0f, fRange, 0.0f,
            view_dim / 2.f + view_sx + fTexelOffs, view_dim / 2.f + view_sy + fTexelOffs, fBias, 1.0f
        };
#elif defined(USE_OGL)
        Fmatrix m_TexelAdjust =
        {
            view_dim / 2.f, 0.0f, 0.0f, 0.0f,
            0.0f, view_dim / 2.f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.5f * fRange, 0.0f,
            view_dim / 2.f + view_sx + fTexelOffs, view_dim / 2.f + view_sy + fTexelOffs, 0.5f + fBias, 1.0f
        };
#else
#   error No graphics API selected or enabled!
#endif
        // compute xforms
        Fmatrix xf_view = L->X.S.view;
        Fmatrix xf_project;
        xf_project.mul(m_TexelAdjust, L->X.S.project);
        m_Shadow.mul(xf_view, Device.mInvView);
        m_Shadow.mulA_44(xf_project);

        // lmap
        view_dim = 1.f;
        view_sx = 0.f;
        view_sy = 0.f;
#ifdef USE_DX11
        Fmatrix m_TexelAdjust2 =
        {
            view_dim / 2.f, 0.0f, 0.0f, 0.0f,
            0.0f, -view_dim / 2.f, 0.0f, 0.0f,
            0.0f, 0.0f, fRange, 0.0f,
            view_dim / 2.f + view_sx + fTexelOffs, view_dim / 2.f + view_sy + fTexelOffs, fBias, 1.0f
        };
#elif defined(USE_OGL)
        Fmatrix m_TexelAdjust2 =
        {
            view_dim / 2.f, 0.0f, 0.0f, 0.0f,
            0.0f, view_dim / 2.f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.5f * fRange, 0.0f,
            view_dim / 2.f + view_sx + fTexelOffs, view_dim / 2.f + view_sy + fTexelOffs, 0.5f + fBias, 1.0f
        };
#else
#   error No graphics API selected or enabled!
#endif

        // compute xforms
        xf_project.mul(m_TexelAdjust2, L->X.S.project);
        m_Lmap.mul(xf_view, Device.mInvView);
        m_Lmap.mulA_44(xf_project);
    }

    // Common constants
    Fvector L_clr, L_pos; // L_dir
    float L_spec;
    L_clr.set(L->color.r, L->color.g, L->color.b);
    L_clr.mul(L->get_LOD());
    L_spec = u_diffuse2s(L_clr);
    Device.mView.transform_tiny(L_pos, L->position);
    //Device.mView.transform_dir(L_dir, L->direction);
    //L_dir.normalize();

    // Draw volume with projective texgen
    {
        // Select shader
        u32 _id = 0;
        if (L->flags.bShadow)
        {
            bool bFullSize = (L->X.S.size == RImplementation.o.smapsize);
            if (L->X.S.transluent)
                _id = SE_L_TRANSLUENT;
            else if (bFullSize)
                _id = SE_L_FULLSIZE;
            else
                _id = SE_L_NORMAL;
        }
        else
        {
            _id = SE_L_UNSHADOWED;
            m_Shadow = m_Lmap;
        }
        cmd_list.set_Element(shader->E[_id]);

        cmd_list.set_CullMode(CULL_CW); // back

        // Constants
        float att_R = L->range * .95f;
        float att_factor = 1.f / (att_R * att_R);
        cmd_list.set_c("Ldynamic_pos", L_pos.x, L_pos.y, L_pos.z, att_factor);
        cmd_list.set_c("Ldynamic_color", L_clr.x, L_clr.y, L_clr.z, L_spec);
        cmd_list.set_c("m_texgen", m_Texgen);
        cmd_list.set_c("m_texgen_J", m_Texgen_J);
        cmd_list.set_c("m_shadow", m_Shadow);
        cmd_list.set_ca("m_lmap", 0, m_Lmap._11, m_Lmap._21, m_Lmap._31, m_Lmap._41);
        cmd_list.set_ca("m_lmap", 1, m_Lmap._12, m_Lmap._22, m_Lmap._32, m_Lmap._42);

        // Fetch4 : enable
        //		if (RImplementation.o.HW_smap_FETCH4)	{
        //. we hacked the shader to force smap on S0
        //#			define FOURCC_GET4  MAKEFOURCC('G','E','T','4')
        //			HW.pDevice->SetSamplerState	( 0, D3DSAMP_MIPMAPLODBIAS, FOURCC_GET4 );
        //		}

        if (!RImplementation.o.msaa)
        {
            cmd_list.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0xff, 0x00);
            draw_volume(cmd_list, L);
        }
        else
        {
            // per pixel
            cmd_list.set_Element(shader->E[_id]);
            cmd_list.set_Stencil(TRUE, D3DCMP_EQUAL, dwLightMarkerID, 0xff, 0x00);
            cmd_list.set_CullMode(D3DCULL_CW);
            draw_volume(cmd_list, L);

            // per sample
            if (RImplementation.o.msaa_opt)
            {
                cmd_list.set_Element(shader_msaa[0]->E[_id]);
                cmd_list.set_Stencil(TRUE, D3DCMP_EQUAL, dwLightMarkerID | 0x80, 0xff, 0x00);
                cmd_list.set_CullMode(D3DCULL_CW);
                draw_volume(cmd_list, L);
            }
            else // checked Holger
            {
#ifdef USE_DX11
                for (u32 i = 0; i < RImplementation.o.msaa_samples; ++i)
                {
                    cmd_list.set_Element(shader_msaa[i]->E[_id]);
                    cmd_list.StateManager.SetSampleMask(u32(1) << i);
                    cmd_list.set_Stencil(TRUE, D3DCMP_EQUAL, dwLightMarkerID | 0x80, 0xff, 0x00);
                    cmd_list.set_CullMode(D3DCULL_CW);
                    draw_volume(cmd_list, L);
                }
                cmd_list.StateManager.SetSampleMask(0xffffffff);
#elif defined(USE_OGL)
                VERIFY(!"Only optimized MSAA is supported in OpenGL");
#else
#   error No graphics API selected or enabled!
#endif
            }
            cmd_list.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0xff, 0x00);
        }

        // Fetch4 : disable
        //		if (RImplementation.o.HW_smap_FETCH4)	{
        //. we hacked the shader to force smap on S0
        //#			define FOURCC_GET1  MAKEFOURCC('G','E','T','1')
        //			HW.pDevice->SetSamplerState	( 0, D3DSAMP_MIPMAPLODBIAS, FOURCC_GET1 );
        //		}
    }

    // blend-copy
    if (!RImplementation.o.fp16_blend)
    {
        u_setrt(cmd_list, rt_Accumulator, nullptr, nullptr, rt_MSAADepth);
        cmd_list.set_Element(s_accum_mask->E[SE_MASK_ACCUM_VOL]);
        cmd_list.set_c("m_texgen", m_Texgen);
        cmd_list.set_c("m_texgen_J", m_Texgen_J);
        if (!RImplementation.o.msaa)
        {
            cmd_list.set_Stencil(TRUE, D3DCMP_EQUAL, dwLightMarkerID, 0xff, 0x00);
            draw_volume(cmd_list, L);
        }
        else // checked Holger
        {
            // per pixel
            cmd_list.set_Element(s_accum_mask->E[SE_MASK_ACCUM_VOL]);
            cmd_list.set_Stencil(TRUE, D3DCMP_EQUAL, dwLightMarkerID, 0xff, 0x00);
            draw_volume(cmd_list, L);
            // per sample
            if (RImplementation.o.msaa_opt)
            {
                cmd_list.set_Element(s_accum_mask_msaa[0]->E[SE_MASK_ACCUM_VOL]);
                cmd_list.set_Stencil(TRUE, D3DCMP_EQUAL, dwLightMarkerID | 0x80, 0xff, 0x00);
                draw_volume(cmd_list, L);
            }
            else // checked Holger
            {
#ifdef USE_DX11
                for (u32 i = 0; i < RImplementation.o.msaa_samples; ++i)
                {
                    cmd_list.set_Element(s_accum_mask_msaa[i]->E[SE_MASK_ACCUM_VOL]);
                    cmd_list.StateManager.SetSampleMask(u32(1) << i);
                    cmd_list.set_Stencil(TRUE, D3DCMP_EQUAL, dwLightMarkerID | 0x80, 0xff, 0x00);
                    draw_volume(cmd_list, L);
                }
                cmd_list.StateManager.SetSampleMask(0xffffffff);
#elif defined(USE_OGL)
                VERIFY(!"Only optimized MSAA is supported in OpenGL");
#else
#   error No graphics API selected or enabled!
#endif
            }
            cmd_list.set_Stencil(TRUE, D3DCMP_EQUAL, dwLightMarkerID, 0xff, 0x00);
        }
    }

    cmd_list.set_Scissor(0);
    // dwLightMarkerID					+=	2;	// keep lowest bit always setted up
    increment_light_marker(cmd_list);

    u_DBT_disable();
}

void CRenderTarget::accum_volumetric(CBackend& cmd_list, light* L)
{
    // [ SSS ] Fade through distance volumetric lights.
    if (ps_ssfx_volumetric.x > 0)
    {
        float Falloff = ps_ssfx_volumetric.y - std::min(std::max((L->vis.distance - 20) * 0.01f, 0.0f), 1.0f) * ps_ssfx_volumetric.y;
        L->m_volumetric_intensity = Falloff;
        L->flags.bVolumetric = Falloff <= 0 ? false : true;
    }

    // if (L->flags.type != IRender_Light::SPOT) return;
    if (!L->flags.bVolumetric)
        return;

    /*float w = float(Device.dwWidth);
    float h = float(Device.dwHeight);

    if (RImplementation.o.ssfx_volumetric)
        set_viewport_size(HW.pContext, w / ps_ssfx_volumetric.w, h / ps_ssfx_volumetric.w);*/

    phase_vol_accumulator(cmd_list);

    ref_shader shader;
    //ref_shader* shader_msaa;

    shader = L->s_volumetric;
    //shader_msaa = L->s_volumetric_msaa;
    if (!shader)
    {
        shader = s_accum_volume;
        //shader_msaa = s_accum_volume_msaa;
    }

    // *** assume accumulator setted up ***
    // *****************************	Mask by stencil		*************************************
    {
        // setup xform
        L->xform_calc();
        cmd_list.set_xform_world(L->m_xform);
        cmd_list.set_xform_view(Device.mView);
        cmd_list.set_xform_project(Device.mProject);
        enable_scissor(L);

        // enable_dbt_bounds				(L);
    }

    cmd_list.set_ColorWriteEnable();
    cmd_list.set_CullMode(CULL_NONE); // back

    // 2D texgens
    /*Fmatrix m_Texgen;
    u_compute_texgen_screen(cmd_list, m_Texgen);
    Fmatrix m_Texgen_J;
    u_compute_texgen_jitter(cmd_list, m_Texgen_J);*/

    // Shadow xform (+texture adjustment matrix)
    Fmatrix m_Shadow, m_Lmap;
    Fmatrix mFrustumSrc;
    CFrustum ClipFrustum;
    {
        float smapsize = float(RImplementation.o.smapsize);
        float fTexelOffs = (.5f / smapsize);
        float view_dim = float(L->X.S.size - 2) / smapsize;
        float view_sx = float(L->X.S.posX + 1) / smapsize;
        float view_sy = float(L->X.S.posY + 1) / smapsize;
        float fRange = float(1.f) * ps_r2_ls_depth_scale;
        float fBias = ps_r2_ls_depth_bias;
#ifdef USE_DX11
        Fmatrix m_TexelAdjust =
        {
            view_dim / 2.f, 0.0f, 0.0f, 0.0f,
            0.0f, -view_dim / 2.f, 0.0f, 0.0f,
            0.0f, 0.0f, fRange, 0.0f,
            view_dim / 2.f + view_sx + fTexelOffs, view_dim / 2.f + view_sy + fTexelOffs, fBias, 1.0f
        };
#elif defined(USE_OGL)
        Fmatrix m_TexelAdjust =
        {
            view_dim / 2.f, 0.0f, 0.0f, 0.0f,
            0.0f, view_dim / 2.f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.5f * fRange, 0.0f,
            view_dim / 2.f + view_sx + fTexelOffs, view_dim / 2.f + view_sy + fTexelOffs, 0.5f + fBias, 1.0f
        };
#else
#   error No graphics API selected or enabled!
#endif

        // compute xforms
        Fmatrix xf_view = L->X.S.view;
        Fmatrix xf_project;
        xf_project.mul(m_TexelAdjust, L->X.S.project);
        m_Shadow.mul(xf_view, Device.mInvView);
        m_Shadow.mulA_44(xf_project);

        // lmap
        view_dim = 1.f;
        view_sx = 0.f;
        view_sy = 0.f;
#ifdef USE_DX11
        Fmatrix m_TexelAdjust2 =
        {
            view_dim / 2.f, 0.0f, 0.0f, 0.0f,
            0.0f, -view_dim / 2.f, 0.0f, 0.0f,
            0.0f, 0.0f, fRange, 0.0f,
            view_dim / 2.f + view_sx + fTexelOffs, view_dim / 2.f + view_sy + fTexelOffs, fBias, 1.0f
        };
#elif defined(USE_OGL)
        Fmatrix m_TexelAdjust2 =
        {
            view_dim / 2.f, 0.0f, 0.0f, 0.0f,
            0.0f, view_dim / 2.f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.5f * fRange, 0.0f,
            view_dim / 2.f + view_sx + fTexelOffs, view_dim / 2.f + view_sy + fTexelOffs, 0.5f + fBias, 1.0f
        };
#else
#   error No graphics API selected or enabled!
#endif

        // compute xforms
        xf_project.mul(m_TexelAdjust2, L->X.S.project);
        m_Lmap.mul(xf_view, Device.mInvView);
        m_Lmap.mulA_44(xf_project);

        // Compute light frustum in world space
        mFrustumSrc.mul(L->X.S.project, xf_view);
        ClipFrustum.CreateFromMatrix(mFrustumSrc, FRUSTUM_P_ALL);
        //	Adjust frustum far plane
        //	4 - far, 5 - near
        ClipFrustum.planes[4].d -= (ClipFrustum.planes[4].d + ClipFrustum.planes[5].d) * (1 - L->m_volumetric_distance);
    }

    //	Calculate camera space AABB
    // xform BB
    /*
    Fbox	BB;
    Fvector	rr; rr.set(L->spatial.sphere.R,L->spatial.sphere.R,L->spatial.sphere.R);
    BB.setb	(L->spatial.sphere.P, rr);

    Fbox	bbp; bbp.invalidate();
    for (u32 i=0; i<8; i++)		{
        Fvector		pt;
        BB.getpoint	(i,pt);
        //Device.mFullTransform.transform	(pt);
        Device.mFullTransform.transform	(mView);
        bbp.modify	(pt);
    }
    */

    //	Calculate camera space AABB
    //	Adjust AABB according to the adjusted distance for the light volume
    Fbox aabb;

    // float	scaledRadius = L->spatial.sphere.R * (1+L->m_volumetric_distance)*0.5f;
    float scaledRadius = L->spatial.sphere.R * L->m_volumetric_distance;
    Fvector rr = Fvector().set(scaledRadius, scaledRadius, scaledRadius);
    Fvector pt = L->spatial.sphere.P;
    pt.sub(L->position);
    pt.mul(L->m_volumetric_distance);
    pt.add(L->position);
    //	Don't adjust AABB
    // float	scaledRadius = L->spatial.sphere.R;
    // Fvector	rr = Fvector().set(scaledRadius,scaledRadius,scaledRadius);
    // Fvector pt = L->spatial.sphere.P;
    Device.mView.transform(pt);
    aabb.setb(pt, rr);
    /*
        //	Calculate presise AABB assuming we are drawing for the spot light
        {
            aabb.invalidate();
            Fmatrix	transform;
            transform.mul( Device.mView, L->m_xform);
            for (u32 i=0; i<DU_CONE_NUMVERTEX; ++i)
            {
                Fvector		pt = du_cone_vertices[i];
                transform.transform(pt);
                aabb.modify(pt);
            }
        }
    */
    // Common vars
    float fQuality = 0;
    int iNumSlices = 0;

    // Color and intensity vars
    Fvector L_clr, L_pos;
    float L_spec;
    float IntensityMod = 1.0f;
    L_clr.set(L->color.r, L->color.g, L->color.b);
    L_clr.mul(L->m_volumetric_distance);

    if (ps_ssfx_volumetric.x <= 0)
    {
        // Vanilla Method
        fQuality = L->m_volumetric_quality;
        iNumSlices = (int)(VOLUMETRIC_SLICES * fQuality);
        //			min 10 surfaces
        iNumSlices = _max(10, iNumSlices);

        // Set Intensity
        fQuality = ((float)iNumSlices) / VOLUMETRIC_SLICES;
        L_clr.mul(L->m_volumetric_intensity);
        L_clr.mul(1 / fQuality);
        L_clr.mul(L->get_LOD());
    }
    else
    {
        // SSS Method
        fQuality = ps_ssfx_volumetric.z;
        iNumSlices = (int)(24 * fQuality);

        // Intensity mod to OMNIPART && HUD
        if (L->flags.type == IRender_Light::OMNIPART || L->flags.bHudMode)
            IntensityMod = 0.2f;

        // Set Intensity
        L_clr.mul(L->m_volumetric_intensity * IntensityMod);
        L_clr.mul(1.0f / fQuality);
        L_clr.mul(L->get_LOD());
        fQuality = ((float)iNumSlices) / 120; // Max setting ( 24 * 5 )
    }

    L_spec = u_diffuse2s(L_clr);
    Device.mView.transform_tiny(L_pos, L->position);
    //Device.mView.transform_dir(L_dir, L->direction);
    //L_dir.normalize();

    // Draw volume with projective texgen
    {
        /*
        // Select shader
        u32		_id					= 0;
        if (L->flags.bShadow)		{
            bool	bFullSize			= (L->X.S.size == RImplementation.o.smapsize);
            if (L->X.S.transluent)	_id	= SE_L_TRANSLUENT;
            else if		(bFullSize)	_id	= SE_L_FULLSIZE;
            else					_id	= SE_L_NORMAL;
        } else {
            _id						= SE_L_UNSHADOWED;
            m_Shadow				= m_Lmap;
        }
        cmd_list.set_Element			(shader->E[ _id ]	);
        */

        cmd_list.set_Element(shader->E[0]);

        // Constants
        float att_R = L->m_volumetric_distance * L->range * .95f;
        float att_factor = 1.f / (att_R * att_R);
        cmd_list.set_c("Ldynamic_pos", L_pos.x, L_pos.y, L_pos.z, att_factor);
        cmd_list.set_c("Ldynamic_color", L_clr.x, L_clr.y, L_clr.z, L_spec);
        //cmd_list.set_c("m_texgen", m_Texgen);
        //cmd_list.set_c("m_texgen_J", m_Texgen_J);
        cmd_list.set_c("m_shadow", m_Shadow);
        cmd_list.set_ca("m_lmap", 0, m_Lmap._11, m_Lmap._21, m_Lmap._31, m_Lmap._41);
        cmd_list.set_ca("m_lmap", 1, m_Lmap._12, m_Lmap._22, m_Lmap._32, m_Lmap._42);
        cmd_list.set_c("vMinBounds", aabb.x1, aabb.y1, aabb.z1, 0.f);
        //	Increase camera-space aabb z size to compensate decrease of slices number
        cmd_list.set_c("vMaxBounds", aabb.x2, aabb.y2, aabb.z1 + (aabb.z2 - aabb.z1) / fQuality, 0.f);

        //	Set up user clip planes
        {
            static shared_str strFrustumClipPlane("FrustumClipPlane");
            //	TODO: DX11: Check if it's equivalent to the previouse code.
            // cmd_list.set_ClipPlanes (TRUE,ClipFrustum.planes,ClipFrustum.p_count);

            //	Transform frustum to clip space
            Fmatrix PlaneTransform;
            PlaneTransform.transpose(Device.mInvFullTransform);
            // HW.pDevice->SetRenderState(D3DRS_CLIPPLANEENABLE, 0x3F);

            for (int i = 0; i < 6; ++i)
            {
                Fvector4& ClipPlane = *(Fvector4*)&ClipFrustum.planes[i].n.x;
                Fvector4 TransformedPlane;
                PlaneTransform.transform(TransformedPlane, ClipPlane);
                TransformedPlane.mul(-1.0f);
                cmd_list.set_ca(strFrustumClipPlane, i, TransformedPlane);
                // HW.pDevice->SetClipPlane( i, &TransformedPlane.x);
            }
            /*
            for ( int i=0; i<6; ++i)
            {
                Fvector4	TransformedPlane;
                ClipTransform.transform(TransformedPlane, UnitClipPlanes[i]);
                TransformedPlane.normalize_as_plane();
                cmd_list.set_ca(strOOBBClipPlane, i, TransformedPlane);
            }
            */

            /*
            //	Transform frustum to clip space
            Fmatrix PlaneTransform;
            PlaneTransform.transpose(Device.mInvFullTransform);
            HW.pDevice->SetRenderState(D3DRS_CLIPPLANEENABLE, 0x3F);

            for ( int i=0; i<6; ++i)
            {
                Fvector4	&ClipPlane = *(Fvector4*)&ClipFrustum.planes[i].n.x;
                Fvector4	TransformedPlane;
                PlaneTransform.transform(TransformedPlane, ClipPlane);
                TransformedPlane.mul(-1.0f);
                HW.pDevice->SetClipPlane( i, &TransformedPlane.x);
            }
            */
        }

        /*
        float	clip[4];
        clip[0] = 1;
        clip[1] =
        clip[2] =
        clip[3] = 0;
        HW.pDevice->SetClipPlane( 0, clip);
        */

        // Fetch4 : enable
        //		if (RImplementation.o.HW_smap_FETCH4)	{
        //. we hacked the shader to force smap on S0
        //#			define FOURCC_GET4  MAKEFOURCC('G','E','T','4')
        //			HW.pDevice->SetSamplerState	( 0, D3DSAMP_MIPMAPLODBIAS, FOURCC_GET4 );
        //		}

        cmd_list.set_ColorWriteEnable(D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE);

        cmd_list.set_Geometry(g_accum_volumetric);
        //	Igor: no need to do it per sub-sample. Plain AA will go just fine.
        cmd_list.Render(D3DPT_TRIANGLELIST, 0, 0, iNumSlices * 4, 0, iNumSlices * 2);

        /*
        if( !RImplementation.o.msaa )
            cmd_list.Render(D3DPT_TRIANGLELIST,0,0,iNumSlises*4,0,iNumSlises*2);
        else
        {
            // per pixel
            cmd_list.set_Element(shader->E[0]);
            cmd_list.set_Stencil(TRUE,D3DCMP_EQUAL,dwLightMarkerID,0xff,0x00);
            cmd_list.Render(D3DPT_TRIANGLELIST,0,0,iNumSlises*4,0,iNumSlises*2);

            // per sample
            if( RImplementation.o.msaa_opt )
            {
                // per sample
                cmd_list.set_Element	(shader_msaa[0]->E[0]);
                cmd_list.set_Stencil(TRUE,D3DCMP_EQUAL,dwLightMarkerID|0x80,0xff,0x00);
                cmd_list.Render(D3DPT_TRIANGLELIST,0,0,iNumSlises*4,0,iNumSlises*2);
            }
            else
            {
#ifdef USE_DX11
                for( u32 i = 0; i < RImplementation.o.msaa_samples; ++i )
                {
                    cmd_list.set_Element	      (shader_msaa[i]->E[0]);
                    StateManager.SetSampleMask ( u32(1) << i );
                    cmd_list.set_Stencil         (TRUE,D3DCMP_EQUAL,dwLightMarkerID|0x80,0xff,0x00);
                    cmd_list.Render(D3DPT_TRIANGLELIST,0,0,iNumSlises*4,0,iNumSlises*2);
                }
                StateManager.SetSampleMask( 0xffffffff );
#elif defined(USE_OGL)
                VERIFY(!"Only optimized MSAA is supported in OpenGL");
#else
#   error No graphics API selected or enabled!
#endif
            }
        }*/

        cmd_list.set_ColorWriteEnable();

        // Fetch4 : disable
        //		if (RImplementation.o.HW_smap_FETCH4)	{
        //. we hacked the shader to force smap on S0
        //#			define FOURCC_GET1  MAKEFOURCC('G','E','T','1')
        //			HW.pDevice->SetSamplerState	( 0, D3DSAMP_MIPMAPLODBIAS, FOURCC_GET1 );
        //		}

        //	Restore clip planes
        cmd_list.set_ClipPlanes(FALSE, (Fmatrix*)0, 0);
    }
    /*
        // blend-copy
        if (!RImplementation.o.fp16_blend)	{
            u_setrt						(rt_Accumulator,NULL,NULL,get_base_zb());
            cmd_list.set_Element			(s_accum_mask->E[SE_MASK_ACCUM_VOL]	);
            cmd_list.set_c				("m_texgen",		m_Texgen);
            cmd_list.set_c				("m_texgen_J",		m_Texgen_J	);
            draw_volume					(L);
        }
    */
    cmd_list.set_Scissor(0);

    /*if (RImplementation.o.ssfx_volumetric)
    set_viewport_size(HW.pContext, w, h);*/
}
