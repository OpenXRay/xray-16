#include "stdafx.h"

void CRenderTarget::phase_blur()
{
    //Get common data
    u32 Offset = 0;
    float d_Z = EPS_S;
    float d_W = 1.0f;
    u32 C = color_rgba(0, 0, 0, 255);

    //Full resolution
    float w = float(Device.dwWidth);
    float h = float(Device.dwHeight);

    Fvector2 p0, p1;
    p0.set(0.0f, 0.0f);
    p1.set(1.0f, 1.0f);

///////////////////////////////////////////////////////////////////////////////////
////Horizontal blur
///////////////////////////////////////////////////////////////////////////////////
    w = float(Device.dwWidth) * 0.5f;
    h = float(Device.dwHeight) * 0.5f;

    u_setrt(RCache, rt_blur_h_2, 0, 0, get_base_zb());
    RCache.set_CullMode(CULL_NONE);
    RCache.set_Stencil(FALSE);

    // Fill vertex buffer
    FVF::TL* pv = (FVF::TL*)RImplementation.Vertex.Lock(4, g_combine->vb_stride, Offset);
    pv->set(0, float(h), d_Z, d_W, C, p0.x, p1.y); pv++;
    pv->set(0, 0, d_Z, d_W, C, p0.x, p0.y); pv++;
    pv->set(float(w), float(h), d_Z, d_W, C, p1.x, p1.y); pv++;
    pv->set(float(w), 0, d_Z, d_W, C, p1.x, p0.y); pv++;
    RImplementation.Vertex.Unlock(4, g_combine->vb_stride);

    // Draw COLOR
    RCache.set_Element(s_blur->E[0]);
    RCache.set_c("blur_params", 1.f, 0.f, w, h);
    RCache.set_Geometry(g_combine);
    RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
///////////////////////////////////////////////////////////////////////////////////
////Final blur
///////////////////////////////////////////////////////////////////////////////////
    u_setrt(RCache, rt_blur_2, 0, 0, get_base_zb());
    RCache.set_CullMode(CULL_NONE);
    RCache.set_Stencil(FALSE);

    // Fill vertex buffer
    pv = (FVF::TL*)RImplementation.Vertex.Lock(4, g_combine->vb_stride, Offset);
    pv->set(0, float(h), d_Z, d_W, C, p0.x, p1.y); pv++;
    pv->set(0, 0, d_Z, d_W, C, p0.x, p0.y); pv++;
    pv->set(float(w), float(h), d_Z, d_W, C, p1.x, p1.y); pv++;
    pv->set(float(w), 0, d_Z, d_W, C, p1.x, p0.y); pv++;
    RImplementation.Vertex.Unlock(4, g_combine->vb_stride);

    // Draw COLOR
    RCache.set_Element(s_blur->E[1]);
    RCache.set_c("blur_params", 0.f, 1.f, w, h);
    RCache.set_Geometry(g_combine);
    RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
///////////////////////////////////////////////////////////////////////////////////
////Horizontal blur / Half res
///////////////////////////////////////////////////////////////////////////////////
    w = float(Device.dwWidth) * 0.25f;
    h = float(Device.dwHeight) * 0.25f;

    u_setrt(RCache, rt_blur_h_4, 0, 0, get_base_zb());
    RCache.set_CullMode(CULL_NONE);
    RCache.set_Stencil(FALSE);

    // Fill vertex buffer
    pv = (FVF::TL*)RImplementation.Vertex.Lock(4, g_combine->vb_stride, Offset);
    pv->set(0, float(h), d_Z, d_W, C, p0.x, p1.y); pv++;
    pv->set(0, 0, d_Z, d_W, C, p0.x, p0.y); pv++;
    pv->set(float(w), float(h), d_Z, d_W, C, p1.x, p1.y); pv++;
    pv->set(float(w), 0, d_Z, d_W, C, p1.x, p0.y); pv++;
    RImplementation.Vertex.Unlock(4, g_combine->vb_stride);

    // Draw COLOR
    RCache.set_Element(s_blur->E[2]);
    RCache.set_c("blur_params", 1.f, 0.f, w, h);
    RCache.set_Geometry(g_combine);
    RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
///////////////////////////////////////////////////////////////////////////////////
////Final blur
///////////////////////////////////////////////////////////////////////////////////
    u_setrt(RCache, rt_blur_4, 0, 0, get_base_zb());
    RCache.set_CullMode(CULL_NONE);
    RCache.set_Stencil(FALSE);

    // Fill vertex buffer
    pv = (FVF::TL*)RImplementation.Vertex.Lock(4, g_combine->vb_stride, Offset);
    pv->set(0, float(h), d_Z, d_W, C, p0.x, p1.y); pv++;
    pv->set(0, 0, d_Z, d_W, C, p0.x, p0.y); pv++;
    pv->set(float(w), float(h), d_Z, d_W, C, p1.x, p1.y); pv++;
    pv->set(float(w), 0, d_Z, d_W, C, p1.x, p0.y); pv++;
    RImplementation.Vertex.Unlock(4, g_combine->vb_stride);

    // Draw COLOR
    RCache.set_Element(s_blur->E[3]);
    RCache.set_c("blur_params", 0.f, 1.f, w, h);
    RCache.set_Geometry(g_combine);
    RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
///////////////////////////////////////////////////////////////////////////////////
////Horizontal blur
///////////////////////////////////////////////////////////////////////////////////
    w = float(Device.dwWidth) * 0.125f;
    h = float(Device.dwHeight) * 0.125f;

    u_setrt(RCache, rt_blur_h_8, 0, 0, get_base_zb());
    RCache.set_CullMode(CULL_NONE);
    RCache.set_Stencil(FALSE);

    // Fill vertex buffer
    pv = (FVF::TL*)RImplementation.Vertex.Lock(4, g_combine->vb_stride, Offset);
    pv->set(0, float(h), d_Z, d_W, C, p0.x, p1.y); pv++;
    pv->set(0, 0, d_Z, d_W, C, p0.x, p0.y); pv++;
    pv->set(float(w), float(h), d_Z, d_W, C, p1.x, p1.y); pv++;
    pv->set(float(w), 0, d_Z, d_W, C, p1.x, p0.y); pv++;
    RImplementation.Vertex.Unlock(4, g_combine->vb_stride);

    // Draw COLOR
    RCache.set_Element(s_blur->E[4]);
    RCache.set_c("blur_params", 1.f, 0.f, w, h);
    RCache.set_Geometry(g_combine);
    RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
///////////////////////////////////////////////////////////////////////////////////
////Final blur
///////////////////////////////////////////////////////////////////////////////////
    u_setrt(RCache, rt_blur_8, 0, 0, get_base_zb());
    RCache.set_CullMode(CULL_NONE);
    RCache.set_Stencil(FALSE);

    // Fill vertex buffer
    pv = (FVF::TL*)RImplementation.Vertex.Lock(4, g_combine->vb_stride, Offset);
    pv->set(0, float(h), d_Z, d_W, C, p0.x, p1.y); pv++;
    pv->set(0, 0, d_Z, d_W, C, p0.x, p0.y); pv++;
    pv->set(float(w), float(h), d_Z, d_W, C, p1.x, p1.y); pv++;
    pv->set(float(w), 0, d_Z, d_W, C, p1.x, p0.y); pv++;
    RImplementation.Vertex.Unlock(4, g_combine->vb_stride);

    // Draw COLOR
    RCache.set_Element(s_blur->E[5]);
    RCache.set_c("blur_params", 0.f, 1.f, w, h);
    RCache.set_Geometry(g_combine);
    RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
///////////////////////////////////////////////////////////////////////////////////
};
