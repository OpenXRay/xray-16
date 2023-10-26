#include "stdafx.h"

void CRenderTarget::mark_msaa_edges()
{
    u32 Offset;
    float d_Z = EPS_S, d_W = 1.f;
    u32 C = color_rgba(255, 255, 255, 255);

    // Fill vertex buffer
    FVF::TL2uv* pv = (FVF::TL2uv*)RImplementation.Vertex.Lock(4, g_combine_2UV->vb_stride, Offset);
#if defined(USE_DX11)
    pv->set(-1, -1,   0, d_W, C, 0, 1, 0, 0);
    pv++;
    pv->set(-1,  1, d_Z, d_W, C, 0, 0, 0, 0);
    pv++;
    pv->set( 1, -1, d_Z, d_W, C, 1, 1, 0, 0);
    pv++;
    pv->set( 1,  1, d_Z, d_W, C, 1, 0, 0, 0);
    pv++;
#elif defined(USE_OGL)
    pv->set(-1, -1,   0, d_W, C, 0, 0, 0, 0);
    pv++;
    pv->set(-1,  1, d_Z, d_W, C, 0, 1, 0, 0);
    pv++;
    pv->set( 1, -1, d_Z, d_W, C, 1, 0, 0, 0);
    pv++;
    pv->set( 1,  1, d_Z, d_W, C, 1, 1, 0, 0);
    pv++;
#else
#   error No graphics API selected or enabled!
#endif
    RImplementation.Vertex.Unlock(4, g_combine_2UV->vb_stride);
#if defined(USE_DX11) // XXX: remove this difference
    u_setrt(RCache, nullptr, nullptr, nullptr, rt_MSAADepth);
#elif defined(USE_OGL)
    u_setrt(RCache, Device.dwWidth, Device.dwHeight, 0, 0, 0, rt_MSAADepth->pZRT);
#endif
    RCache.set_Element(s_mark_msaa_edges->E[0]);
    RCache.set_Geometry(g_combine_2UV);
    RCache.set_Stencil(TRUE, D3DCMP_ALWAYS, 0x80, 0xFF, 0x80,
        D3DSTENCILOP_KEEP, D3DSTENCILOP_REPLACE, D3DSTENCILOP_KEEP);
    RCache.set_ColorWriteEnable(FALSE);
    RCache.set_ZFunc(D3DCMP_ALWAYS);
    RCache.set_Z(FALSE);
    RCache.set_CullMode(D3DCULL_NONE);
    RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
    RCache.set_ColorWriteEnable();
}
