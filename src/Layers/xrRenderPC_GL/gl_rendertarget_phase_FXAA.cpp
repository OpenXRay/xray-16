#include "stdafx.h"
#include "gl_rendertarget.h"

void CRenderTarget::phase_fxaa()
{
    u32 Offset = 0;
    const float _w = float(Device.dwWidth);
    const float _h = float(Device.dwHeight);
    const float du = ps_r1_pps_u, dv = ps_r1_pps_v;

    u_setrt(rt_Generic, nullptr, nullptr, HW.pBaseZB);

    FVF::V* pv = (FVF::V*)RCache.Vertex.Lock(4, g_fxaa->vb_stride, Offset);
    pv->set(du + 0, dv + float(_h), 0, 0, 1);
    pv++;
    pv->set(du + 0, dv + 0, 0, 0, 0);
    pv++;
    pv->set(du + float(_w), dv + float(_h), 0, 1, 1);
    pv++;
    pv->set(du + float(_w), dv + 0, 0, 1, 0);
    pv++;
    RCache.Vertex.Unlock(4, g_fxaa->vb_stride);

    RCache.set_Element(s_fxaa->E[0]);
    RCache.set_Geometry(g_fxaa);
    RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

    //HW.pDevice->CopyResource(rt_Generic_0->pTexture->surface_get(), rt_Generic->pTexture->surface_get());
    CHK_GL(glCopyImageSubData(rt_Generic->pTexture->surface_get(), GL_TEXTURE_2D, 0, 0, 0, 0, rt_Generic_0->pTexture->surface_get(), GL_TEXTURE_2D, 
        0, 0, 0, 0, rt_Generic_0->pTexture->get_Width(), rt_Generic_0->pTexture->get_Height(), 1));
}
