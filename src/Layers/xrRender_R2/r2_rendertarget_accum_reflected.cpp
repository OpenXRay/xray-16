#include "stdafx.h"

void CRenderTarget::accum_reflected(CBackend& cmd_list, light* L)
{
    phase_accumulator(cmd_list);
    RImplementation.Stats.l_visible++;

    // *** assume accumulator setted up ***
    // *****************************	Mask by stencil		*************************************

    bool bIntersect = false; // enable_scissor(L);
    L->xform_calc();
    cmd_list.set_xform_world(L->m_xform);
    cmd_list.set_xform_view(Device.mView);
    cmd_list.set_xform_project(Device.mProject);
    bIntersect = enable_scissor(L);
    enable_dbt_bounds(L);

    // *****************************	Minimize overdraw	*************************************
    // Select shader (front or back-faces), *** back, if intersect near plane
    cmd_list.set_ColorWriteEnable();
    if (bIntersect)
        cmd_list.set_CullMode(CULL_CW); // back
    else
        cmd_list.set_CullMode(CULL_CCW); // front

    // 2D texgen (texture adjustment matrix)
    Fmatrix m_Texgen;
    {
        float _w = float(Device.dwWidth);
        float _h = float(Device.dwHeight);
        float o_w = (.5f / _w);
        float o_h = (.5f / _h);
#if defined(USE_DX11)
        Fmatrix m_TexelAdjust =
        {
            0.5f, 0.0f, 0.0f, 0.0f,
            0.0f, -0.5f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.5f + o_w, 0.5f + o_h, 0.0f, 1.0f
        };
#elif defined(USE_OGL)
        Fmatrix m_TexelAdjust =
        {
            0.5f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.5f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.5f + o_w, 0.5f + o_h, 0.0f, 1.0f
        };
#else
#   error No graphics API selected or enabled!
#endif
        m_Texgen.mul(m_TexelAdjust, cmd_list.xforms.m_wvp);
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
        cmd_list.set_Shader(s_accum_reflected);

        // Constants
        cmd_list.set_c("Ldynamic_pos", L_pos.x, L_pos.y, L_pos.z, 1 / (L->range * L->range));
        cmd_list.set_c("Ldynamic_color", L_clr.x, L_clr.y, L_clr.z, L_spec);
        cmd_list.set_c("direction", L_dir.x, L_dir.y, L_dir.z, 0.f);
        cmd_list.set_c("m_texgen", m_Texgen);

        if (!RImplementation.o.msaa)
        {
            cmd_list.set_Stencil(TRUE, D3DCMP_LESSEQUAL, 0x01, 0xff, 0x00);
            draw_volume(cmd_list, L);
        }
        else // checked Holger
        {
            // per pixel
            cmd_list.set_Stencil(TRUE, D3DCMP_EQUAL, 0x01, 0x81, 0x00);
            draw_volume(cmd_list, L);

            // per sample
            if (RImplementation.o.msaa_opt)
            {
                cmd_list.set_Shader(s_accum_reflected_msaa[0]);
                cmd_list.set_Stencil(TRUE, D3DCMP_EQUAL, 0x81, 0x81, 0x00);
                if (bIntersect)
                    cmd_list.set_CullMode(CULL_CW); // back
                else
                    cmd_list.set_CullMode(CULL_CCW); // front
                draw_volume(cmd_list, L);
            }
            else // checked Holger
            {
#   if defined(USE_DX11)
                for (u32 i = 0; i < RImplementation.o.msaa_samples; ++i)
                {
                    cmd_list.set_Shader(s_accum_reflected_msaa[i]);
                    cmd_list.set_Stencil(TRUE, D3DCMP_EQUAL, 0x81, 0x81, 0x00);
                    if (bIntersect)
                        cmd_list.set_CullMode(CULL_CW); // back
                    else
                        cmd_list.set_CullMode(CULL_CCW); // front
                    cmd_list.StateManager.SetSampleMask(u32(1) << i);
                    draw_volume(cmd_list, L);
                }
                cmd_list.StateManager.SetSampleMask(0xffffffff);
#   elif defined(USE_OGL)
                VERIFY(!"Only optimized MSAA is supported in OpenGL");
#   endif // USE_DX11
            }
        }
    }

    // blend-copy
    if (!RImplementation.o.fp16_blend)
    {
        u_setrt(cmd_list, rt_Accumulator, nullptr, nullptr, rt_MSAADepth);
        cmd_list.set_Element(s_accum_mask->E[SE_MASK_ACCUM_VOL]);
        cmd_list.set_c("m_texgen", m_Texgen);
        if (!RImplementation.o.msaa)
        {
            // per pixel
            cmd_list.set_Stencil(TRUE, D3DCMP_LESSEQUAL, 0x01, 0xff, 0x00);
            draw_volume(cmd_list, L);
        }
        else // checked holger
        {
            // per pixel
            cmd_list.set_Stencil(TRUE, D3DCMP_EQUAL, 0x01, 0x81, 0x00);
            draw_volume(cmd_list, L);
            // per sample
            if (RImplementation.o.msaa_opt)
            {
                cmd_list.set_Element(s_accum_mask_msaa[0]->E[SE_MASK_ACCUM_VOL]);
                cmd_list.set_Stencil(TRUE, D3DCMP_EQUAL, 0x81, 0x81, 0x00);
                draw_volume(cmd_list, L);
            }
            else // checked holger
            {
#   if defined(USE_DX11)
                for (u32 i = 0; i < RImplementation.o.msaa_samples; ++i)
                {
                    cmd_list.set_Element(s_accum_mask_msaa[i]->E[SE_MASK_ACCUM_VOL]);
                    cmd_list.set_Stencil(TRUE, D3DCMP_EQUAL, 0x81, 0x81, 0x00);
                    cmd_list.StateManager.SetSampleMask(u32(1) << i);
                    draw_volume(cmd_list, L);
                }
                cmd_list.StateManager.SetSampleMask(0xffffffff);
#   elif defined(USE_OGL)
                VERIFY(!"Only optimized MSAA is supported in OpenGL");
#   endif // USE_DX11
            }
#   if defined(USE_DX11) // XXX: not sure why this is needed. Just preserving original behaviour
            cmd_list.set_Stencil(TRUE, D3DCMP_LESSEQUAL, 0x01, 0xff, 0x00);
#   endif // !USE_OGL
        }
    }

    //
    u_DBT_disable();
}
