#include "stdafx.h"

void CRenderTarget::accum_reflected(light* L)
{
    phase_accumulator();
    RImplementation.Stats.l_visible++;

    // *** assume accumulator setted up ***
    // *****************************	Mask by stencil		*************************************

    bool bIntersect = false; // enable_scissor(L);
    L->xform_calc();
    RCache.set_xform_world(L->m_xform);
    RCache.set_xform_view(Device.mView);
    RCache.set_xform_project(Device.mProject);
    bIntersect = enable_scissor(L);
    enable_dbt_bounds(L);

    // *****************************	Minimize overdraw	*************************************
    // Select shader (front or back-faces), *** back, if intersect near plane
    RCache.set_ColorWriteEnable();
    if (bIntersect)
        RCache.set_CullMode(CULL_CW); // back
    else
        RCache.set_CullMode(CULL_CCW); // front

    // 2D texgen (texture adjustment matrix)
    Fmatrix m_Texgen;
    {
        float _w = float(Device.dwWidth);
        float _h = float(Device.dwHeight);
        float o_w = (.5f / _w);
        float o_h = (.5f / _h);
#ifdef USE_OGL
        Fmatrix m_TexelAdjust =
        {
            0.5f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.5f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.5f + o_w, 0.5f + o_h, 0.0f, 1.0f
        };
#else
        Fmatrix m_TexelAdjust =
        {
            0.5f, 0.0f, 0.0f, 0.0f,
            0.0f, -0.5f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.5f + o_w, 0.5f + o_h, 0.0f, 1.0f
        };
#endif // USE_OGL
        m_Texgen.mul(m_TexelAdjust, RCache.xforms.m_wvp);
    }

    // Common constants
    Fvector L_dir, L_clr, L_pos;
    float L_spec;
    L_clr.set(L->color.r, L->color.g, L->color.b);
    L_spec = u_diffuse2s(L_clr);
    Device.mView.transform_tiny(L_pos, L->position);
    Device.mView.transform_dir(L_dir, L->direction);
    L_dir.normalize();

    {
        // Lighting
        RCache.set_Shader(s_accum_reflected);

        // Constants
        RCache.set_c("Ldynamic_pos", L_pos.x, L_pos.y, L_pos.z, 1 / (L->range * L->range));
        RCache.set_c("Ldynamic_color", L_clr.x, L_clr.y, L_clr.z, L_spec);
        RCache.set_c("direction", L_dir.x, L_dir.y, L_dir.z, 0.f);
        RCache.set_c("m_texgen", m_Texgen);

#ifdef USE_DX9
        RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, 0x01, 0xff, 0x00);
        draw_volume(L);
#else
        if (!RImplementation.o.dx10_msaa)
        {
            RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, 0x01, 0xff, 0x00);
            draw_volume(L);
        }
        else // checked Holger
        {
            // per pixel
            RCache.set_Stencil(TRUE, D3DCMP_EQUAL, 0x01, 0x81, 0x00);
            draw_volume(L);

            // per sample
            if (RImplementation.o.dx10_msaa_opt)
            {
                RCache.set_Shader(s_accum_reflected_msaa[0]);
                RCache.set_Stencil(TRUE, D3DCMP_EQUAL, 0x81, 0x81, 0x00);
                if (bIntersect)
                    RCache.set_CullMode(CULL_CW); // back
                else
                    RCache.set_CullMode(CULL_CCW); // front
                draw_volume(L);
            }
            else // checked Holger
            {
#   ifdef USE_OGL
                VERIFY(!"Only optimized MSAA is supported in OpenGL");
#   else
                for (u32 i = 0; i < RImplementation.o.dx10_msaa_samples; ++i)
                {
                    RCache.set_Shader(s_accum_reflected_msaa[i]);
                    RCache.set_Stencil(TRUE, D3DCMP_EQUAL, 0x81, 0x81, 0x00);
                    if (bIntersect)
                        RCache.set_CullMode(CULL_CW); // back
                    else
                        RCache.set_CullMode(CULL_CCW); // front
                    StateManager.SetSampleMask(u32(1) << i);
                    draw_volume(L);
                }
                StateManager.SetSampleMask(0xffffffff);
#   endif // USE_OGL
            }
        }
#endif // USE_DX9
    }

    // blend-copy
    if (!RImplementation.o.fp16_blend)
    {
        u_setrt(rt_Accumulator, nullptr, nullptr, rt_MSAADepth);
        RCache.set_Element(s_accum_mask->E[SE_MASK_ACCUM_VOL]);
        RCache.set_c("m_texgen", m_Texgen);
#ifdef USE_DX9
        draw_volume(L);
#else
        if (!RImplementation.o.dx10_msaa)
        {
            // per pixel
            RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, 0x01, 0xff, 0x00);
            draw_volume(L);
        }
        else // checked holger
        {
            // per pixel
            RCache.set_Stencil(TRUE, D3DCMP_EQUAL, 0x01, 0x81, 0x00);
            draw_volume(L);
            // per sample
            if (RImplementation.o.dx10_msaa_opt)
            {
                RCache.set_Element(s_accum_mask_msaa[0]->E[SE_MASK_ACCUM_VOL]);
                RCache.set_Stencil(TRUE, D3DCMP_EQUAL, 0x81, 0x81, 0x00);
                draw_volume(L);
            }
            else // checked holger
            {
#   ifdef USE_OGL
                VERIFY(!"Only optimized MSAA is supported in OpenGL");
#   else
                for (u32 i = 0; i < RImplementation.o.dx10_msaa_samples; ++i)
                {
                    RCache.set_Element(s_accum_mask_msaa[i]->E[SE_MASK_ACCUM_VOL]);
                    RCache.set_Stencil(TRUE, D3DCMP_EQUAL, 0x81, 0x81, 0x00);
                    StateManager.SetSampleMask(u32(1) << i);
                    draw_volume(L);
                }
                StateManager.SetSampleMask(0xffffffff);
#   endif // USE_OGL
            }
#   ifndef USE_OGL // XXX: not sure why this is needed. Just preserving original behaviour
            RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, 0x01, 0xff, 0x00);
#   endif // !USE_OGL
        }
#endif // USE_DX9
    }

    //
    u_DBT_disable();
}
