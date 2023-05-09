#include "stdafx.h"

void CRenderTarget::accum_point(CBackend& cmd_list, light* L)
{
    phase_accumulator(cmd_list);
    RImplementation.Stats.l_visible++;

    ref_shader shader = L->s_point;
    ref_shader* shader_msaa = L->s_point_msaa;
    if (!shader)
    {
        shader = s_accum_point;
        shader_msaa = s_accum_point_msaa;
    }

    // Common
    Fvector L_pos;
    float L_spec;
    // float		L_R					= L->range;
    float L_R = L->range * .95f;
    Fvector L_clr;
    L_clr.set(L->color.r, L->color.g, L->color.b);
    L_spec = u_diffuse2s(L_clr);
    Device.mView.transform_tiny(L_pos, L->position);

    // Xforms
    L->xform_calc();
    cmd_list.set_xform_world(L->m_xform);
    cmd_list.set_xform_view(Device.mView);
    cmd_list.set_xform_project(Device.mProject);
    enable_scissor(L);
    enable_dbt_bounds(L);

    // *****************************	Mask by stencil		*************************************
    // *** similar to "Carmack's reverse", but assumes convex, non intersecting objects,
    // *** thus can cope without stencil clear with 127 lights
    // *** in practice, 'cause we "clear" it back to 0x1 it usually allows us to > 200 lights :)
    cmd_list.set_Element(s_accum_mask->E[SE_MASK_POINT]); // masker
    //	Done in blender!
    // RCache.set_ColorWriteEnable		(FALSE);

    // backfaces: if (1<=stencil && zfail)	stencil = light_id
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

    // frontfaces: if (1<=stencil && zfail)	stencil = 0x1
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

    // nv-stencil recompression
    if (RImplementation.o.nvstencil)
        u_stencil_optimize(cmd_list);

    // *****************************	Minimize overdraw	*************************************
    // Select shader (front or back-faces), *** back, if intersect near plane
    cmd_list.set_ColorWriteEnable();
    cmd_list.set_CullMode(CULL_CW); // back
    /*
    if (bIntersect)	RCache.set_CullMode		(CULL_CW);		// back
    else			RCache.set_CullMode		(CULL_CCW);		// front
    */

    // 2D texgens
    Fmatrix m_Texgen;
    u_compute_texgen_screen(cmd_list, m_Texgen);
    Fmatrix m_Texgen_J;
    u_compute_texgen_jitter(cmd_list, m_Texgen_J);

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
        cmd_list.set_Element(shader->E[_id]);

        // Constants
        cmd_list.set_c("Ldynamic_pos", L_pos.x, L_pos.y, L_pos.z, 1 / (L_R * L_R));
        cmd_list.set_c("Ldynamic_color", L_clr.x, L_clr.y, L_clr.z, L_spec);
        cmd_list.set_c("m_texgen", m_Texgen);

        // Fetch4 : enable
        //		if (RImplementation.o.HW_smap_FETCH4)	{
        //. we hacked the shader to force smap on S0
        //#			define FOURCC_GET4  MAKEFOURCC('G','E','T','4')
        //			HW.pDevice->SetSamplerState	( 0, D3DSAMP_MIPMAPLODBIAS, FOURCC_GET4 );
        //		}

        cmd_list.set_CullMode(CULL_CW); // back
        // Render if (light_id <= stencil && z-pass)
        if (!RImplementation.o.msaa)
        {
            cmd_list.set_Stencil(TRUE, D3DCMP_EQUAL, dwLightMarkerID, 0xff, 0x00);
            draw_volume(cmd_list, L);
        }
        else // checked Holger
        {
            // per pixel
            cmd_list.set_Stencil(TRUE, D3DCMP_EQUAL, dwLightMarkerID, 0xff, 0x00);
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
#if defined(USE_DX11)
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
            cmd_list.set_Stencil(TRUE, D3DCMP_EQUAL, dwLightMarkerID, 0xff, 0x00);
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
        if (!RImplementation.o.msaa)
        {
            cmd_list.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0xff, 0x00);
            draw_volume(cmd_list, L);
        }
        else // checked Holger
        {
            // per pixel
            cmd_list.set_CullMode(D3DCULL_CW);
            cmd_list.set_Stencil(TRUE, D3DCMP_EQUAL, dwLightMarkerID, 0xff, 0x00);
            draw_volume(cmd_list, L);
            if (RImplementation.o.msaa_opt)
            {
                // per sample
                cmd_list.set_Element(s_accum_mask_msaa[0]->E[SE_MASK_ACCUM_VOL]);
                cmd_list.set_CullMode(D3DCULL_CW);
                cmd_list.set_Stencil(TRUE, D3DCMP_EQUAL, dwLightMarkerID | 0x80, 0xff, 0x00);
                draw_volume(cmd_list, L);
            }
            else // checked Holger
            {
#if defined(USE_DX11)
                for (u32 i = 0; i < RImplementation.o.msaa_samples; ++i)
                {
                    cmd_list.set_Element(s_accum_mask_msaa[i]->E[SE_MASK_ACCUM_VOL]);
                    cmd_list.set_CullMode(D3DCULL_CW);
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
            cmd_list.set_Stencil(TRUE, D3DCMP_LESSEQUAL, dwLightMarkerID, 0xff, 0x00);
        }
    }

    cmd_list.set_Scissor(0);

    // dwLightMarkerID					+=	2;	// keep lowest bit always setted up
    increment_light_marker(cmd_list);

    u_DBT_disable();
}
