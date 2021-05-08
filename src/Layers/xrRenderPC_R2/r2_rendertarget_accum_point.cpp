#include "stdafx.h"

void CRenderTarget::accum_point(light* L)
{
    phase_accumulator();
    RImplementation.Stats.l_visible++;

    ref_shader shader = L->s_point;
    if (!shader)
        shader = s_accum_point;

    Fmatrix Pold = Fidentity;
    Fmatrix FTold = Fidentity;

    if (L->flags.bHudMode)
    {
        extern ENGINE_API float psHUD_FOV;
        Pold = Device.mProject;
        FTold = Device.mFullTransform;
        Device.mProject.build_projection(deg2rad(psHUD_FOV * Device.fFOV /* *Device.fASPECT*/), Device.fASPECT,
            VIEWPORT_NEAR, g_pGamePersistent->Environment().CurrentEnv->far_plane);

        Device.mFullTransform.mul(Device.mProject, Device.mView);
        RCache.set_xform_project(Device.mProject);
        RImplementation.rmNear();
    }

    // Common
    Fvector L_pos;
    float L_spec;
    // float		L_R					= L->range;
    float L_R = L->range * 0.95f;
    Fvector L_clr;
    L_clr.set(L->color.r, L->color.g, L->color.b);
    L_spec = u_diffuse2s(L_clr);
    Device.mView.transform_tiny(L_pos, L->position);

    // Xforms
    L->xform_calc();
    RCache.set_xform_world(L->m_xform);
    RCache.set_xform_view(Device.mView);
    RCache.set_xform_project(Device.mProject);
    enable_scissor(L);
    enable_dbt_bounds(L);

    // *****************************	Mask by stencil		*************************************
    // *** similar to "Carmack's reverse", but assumes convex, non intersecting objects,
    // *** thus can cope without stencil clear with 127 lights
    // *** in practice, 'cause we "clear" it back to 0x1 it usually allows us to > 200 lights :)
    RCache.set_Element(s_accum_mask->E[SE_MASK_POINT]); // masker
    RCache.set_ColorWriteEnable(FALSE);

    // backfaces: if (stencil>=1 && zfail)	stencil = light_id
    RCache.set_CullMode(CULL_CW);
    RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0x01, 0xff, D3DSTENCILOP_KEEP, D3DSTENCILOP_KEEP,
        D3DSTENCILOP_REPLACE);
    draw_volume(L);

    // frontfaces: if (stencil>=light_id && zfail)	stencil = 0x1
    RCache.set_CullMode(CULL_CCW);
    RCache.set_Stencil(
        TRUE, D3DCMP_LESSEQUAL, 0x01, 0xff, 0xff, D3DSTENCILOP_KEEP, D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE);
    draw_volume(L);

    // nv-stencil recompression
    if (RImplementation.o.nvstencil)
        u_stencil_optimize();

    // *****************************	Minimize overdraw	*************************************
    // Select shader (front or back-faces), *** back, if intersect near plane
    RCache.set_ColorWriteEnable();
    RCache.set_CullMode(CULL_CW); // back
    /*
    if (bIntersect)	RCache.set_CullMode		(CULL_CW);		// back
    else			RCache.set_CullMode		(CULL_CCW);		// front
    */

    // 2D texgens
    Fmatrix m_Texgen;
    u_compute_texgen_screen(m_Texgen);
    Fmatrix m_Texgen_J;
    u_compute_texgen_jitter(m_Texgen_J);

    // Draw volume with projective texgen
    {
        // Select shader
        u32 _id = 0;
        if (L->flags.bShadow)
        {
            bool bFullSize = (L->X.S.size == u32(RImplementation.o.smapsize));
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
            // m_Shadow				= m_Lmap;
        }
        RCache.set_Element(shader->E[_id]);

        // Constants
        RCache.set_c("Ldynamic_pos", L_pos.x, L_pos.y, L_pos.z, 1 / (L_R * L_R));
        RCache.set_c("Ldynamic_color", L_clr.x, L_clr.y, L_clr.z, L_spec);
        RCache.set_c("m_texgen", m_Texgen);

        // Fetch4 : enable
        if (RImplementation.o.HW_smap_FETCH4)
        {
//. we hacked the shader to force smap on S0
#define FOURCC_GET4 MAKEFOURCC('G', 'E', 'T', '4')
            HW.pDevice->SetSamplerState(0, D3DSAMP_MIPMAPLODBIAS, FOURCC_GET4);
        }

        // Render if (stencil >= light_id && z-pass)
        RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0xff, 0x00, D3DSTENCILOP_KEEP, D3DSTENCILOP_KEEP,
            D3DSTENCILOP_KEEP);
        draw_volume(L);

        // Fetch4 : disable
        if (RImplementation.o.HW_smap_FETCH4)
        {
//. we hacked the shader to force smap on S0
#define FOURCC_GET1 MAKEFOURCC('G', 'E', 'T', '1')
            HW.pDevice->SetSamplerState(0, D3DSAMP_MIPMAPLODBIAS, FOURCC_GET1);
        }
    }

    // blend-copy
    if (!RImplementation.o.fp16_blend)
    {
        u_setrt(rt_Accumulator, NULL, NULL, get_base_zb());
        RCache.set_Element(s_accum_mask->E[SE_MASK_ACCUM_VOL]);
        RCache.set_c("m_texgen", m_Texgen);
        draw_volume(L);
    }

    RCache.set_Scissor(0);

    // dwLightMarkerID					+=	2;	// keep lowest bit always setted up
    increment_light_marker();

    u_DBT_disable();

    if (L->flags.bHudMode)
    {
        RImplementation.rmNormal();
        // Restore projection
        Device.mProject = Pold;
        Device.mFullTransform = FTold;
        RCache.set_xform_project(Device.mProject);
    }
}
