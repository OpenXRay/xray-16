#include "stdafx.h"

void CRenderTarget::phase_smap_spot_clear(CBackend& cmd_list)
{
    rt_smap_depth->set_slice_write(cmd_list.context_id, 0);
    cmd_list.set_pass_targets(
        rt_smap_surf,
        nullptr,
        nullptr,
        rt_smap_depth
    );
    cmd_list.ClearZB(rt_smap_depth, 1.0f);

#if defined(USE_DX11) || defined(USE_DX12)
    HW.get_context(CHW::IMM_CTX_ID)->ClearState();
#endif
}

void CRenderTarget::phase_smap_spot(CBackend& cmd_list, light* L)
{
    rt_smap_depth->set_slice_write(cmd_list.context_id, 0); // TODO: it is possible to increase lights batch size
                                                            // by rendering into different smap array slices in parallel
    cmd_list.set_pass_targets(
        rt_smap_surf,
        nullptr,
        nullptr,
        rt_smap_depth
    );
    const D3D_VIEWPORT viewport = { L->X.S.posX, L->X.S.posY, L->X.S.size, L->X.S.size, 0.f, 1.f };
    cmd_list.SetViewport(viewport);

    // Misc		- draw only front-faces //back-faces
    cmd_list.set_CullMode(CULL_CCW);
    cmd_list.set_Stencil(FALSE);
    // no transparency
#pragma todo("can optimize for multi-lights covering more than say 50%...")
    if (RImplementation.o.HW_smap)
        cmd_list.set_ColorWriteEnable(FALSE);

    // For DX11 do it once per smap generation pass in phase_smap_spot_clear
#ifdef USE_DX9
    cmd_list.ClearZB(rt_smap_depth, 1.0f);
#endif
}

void CRenderTarget::phase_smap_spot_tsh(CBackend& cmd_list, light* L)
{
    VERIFY(!"Implement clear of the buffer for tsh!");
    VERIFY(RImplementation.o.Tshadows);
    cmd_list.set_ColorWriteEnable();
    if (IRender_Light::OMNIPART == L->flags.type)
    {
        // omni-part
        cmd_list.ClearRT(cmd_list.get_RT(), { 1.0f, 1.0f, 1.0f, 1.0f });
    }
    else
    {
        // real-spot
        // Select color-mask
        ref_shader shader = L->s_spot;
        if (!shader)
            shader = s_accum_spot;
        cmd_list.set_Element(shader->E[SE_L_FILL]);

        // Fill vertex buffer
        Fvector2 p0, p1;
        u32 Offset;
        u32 C = color_rgba(255, 255, 255, 255);
        float _w = float(L->X.S.size);
        float _h = float(L->X.S.size);
        float d_Z = EPS_S;
        float d_W = 1.f;
        p0.set(.5f / _w, .5f / _h);
        p1.set((_w + .5f) / _w, (_h + .5f) / _h);

        FVF::TL* pv = (FVF::TL*)RImplementation.Vertex.Lock(4, g_combine->vb_stride, Offset);
#if defined(USE_DX9) || defined(USE_DX11) || defined(USE_DX12)
        pv->set(EPS, float(_h + EPS), d_Z, d_W, C, p0.x, p1.y);
        pv++;
        pv->set(EPS, EPS, d_Z, d_W, C, p0.x, p0.y);
        pv++;
        pv->set(float(_w + EPS), float(_h + EPS), d_Z, d_W, C, p1.x, p1.y);
        pv++;
        pv->set(float(_w + EPS), EPS, d_Z, d_W, C, p1.x, p0.y);
        pv++;
#elif defined(USE_OGL)
        pv->set(EPS, EPS, d_Z, d_W, C, p0.x, p0.y);
        pv++;
        pv->set(EPS, float(_h + EPS), d_Z, d_W, C, p0.x, p1.y);
        pv++;
        pv->set(float(_w + EPS), EPS, d_Z, d_W, C, p1.x, p0.y);
        pv++;
        pv->set(float(_w + EPS), float(_h + EPS), d_Z, d_W, C, p1.x, p1.y);
        pv++;
#else
#   error No graphics API selected or enabled!
#endif // USE_DX9 || USE_DX11
        RImplementation.Vertex.Unlock(4, g_combine->vb_stride);
        cmd_list.set_Geometry(g_combine);

        // draw
        cmd_list.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
    }
}
