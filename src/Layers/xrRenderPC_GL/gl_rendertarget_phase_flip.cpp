#include "stdafx.h"

#if 0 // kept for historical reasons
void CRenderTarget::phase_flip()
{
    PIX_EVENT(phase_flip);

    // Switch to backbuffer
    RCache.set_FB();

    // common calc for quad-rendering
    u32 Offset;
    u32 C = color_rgba(255, 255, 255, 255);
    float _w = float(Device.dwWidth);
    float _h = float(Device.dwHeight);
    Fvector2 p0, p1;
    p0.set(.5f / _w, .5f / _h);
    p1.set((_w + .5f) / _w, (_h + .5f) / _h);
    float d_Z = EPS_S, d_W = 1.f;

    // Fill vertex buffer
    FVF::TL* pv = (FVF::TL*)RCache.Vertex.Lock(4, g_flip->vb_stride, Offset);
    pv->set(EPS, float(_h + EPS), d_Z, d_W, C, p0.x, p0.y);
    pv++;
    pv->set(EPS, EPS, d_Z, d_W, C, p0.x, p1.y);
    pv++;
    pv->set(float(_w + EPS), float(_h + EPS), d_Z, d_W, C, p1.x, p0.y);
    pv++;
    pv->set(float(_w + EPS), EPS, d_Z, d_W, C, p1.x, p1.y);
    pv++;
    RCache.Vertex.Unlock(4, g_flip->vb_stride);

    // Draw
    RCache.set_Element(s_flip->E[0]);
    RCache.set_Geometry(g_flip);

    RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

    // Switch to framebuffer
    RCache.set_FB(HW.pFB);
}
#endif
