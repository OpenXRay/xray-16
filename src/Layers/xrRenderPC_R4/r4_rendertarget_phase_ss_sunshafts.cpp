#include "stdafx.h"

void CRenderTarget::phase_sunshafts()
{
    CEnvDescriptor &env = *g_pGamePersistent->Environment().CurrentEnv;
    //if (env.m_fSunShaftsIntensity <= 0.001) return;

    u32 Offset;
    Fvector2 p0, p1;

    // common 
    struct v_aa
    {
        Fvector4 p;
        Fvector2 uv0;
    };

    float _w = float(Device.dwWidth);
    float _h = float(Device.dwHeight);
    //float	ddw = 1.f / _w;
    //float	ddh = 1.f / _h;
    p0.set(.5f / _w, .5f / _h);
    p1.set((_w + .5f) / _w, (_h + .5f) / _h);

    // constants

    Fvector4 params = { 0,0,0,0 };
    params.x = 1.f;//env.m_fSunShaftsIntensity;
    params.y = ps_r2_ss_sunshafts_length;
    params.w = ps_r2_ss_sunshafts_radius;

    //***MASK GENERATION***
    /*
    In this pass generates geometry mask
    */
    // Set RT's
    u_setrt(rt_sunshafts_0, 0, 0, HW.pBaseZB);
    RCache.set_CullMode(CULL_NONE);
    RCache.set_Stencil(FALSE);

    // Fill vertex buffer
    v_aa* pv = (v_aa*)RCache.Vertex.Lock(4, g_KD->vb_stride, Offset);
    pv->p.set(EPS, float(_h + EPS), EPS, 1.f); pv->uv0.set(p0.x, p1.y); pv++;
    pv->p.set(EPS, EPS, EPS, 1.f); pv->uv0.set(p0.x, p0.y); pv++;
    pv->p.set(float(_w + EPS), float(_h + EPS), EPS, 1.f); pv->uv0.set(p1.x, p1.y); pv++;
    pv->p.set(float(_w + EPS), EPS, EPS, 1.f); pv->uv0.set(p1.x, p0.y); pv++;
    RCache.Vertex.Unlock(4, g_KD->vb_stride);

    // Draw COLOR
    RCache.set_Element(s_sunshafts->E[0]);
    RCache.set_Geometry(g_KD);
    RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

    //***FIRST PASS***
    /*
    first blurring pass
    */
    // Set RT's
    u_setrt(rt_sunshafts_1, 0, 0, HW.pBaseZB);
    RCache.set_CullMode(CULL_NONE);
    RCache.set_Stencil(FALSE);

    // Fill vertex buffer
    pv = (v_aa*)RCache.Vertex.Lock(4, g_KD->vb_stride, Offset);
    pv->p.set(EPS, float(_h + EPS), EPS, 1.f); pv->uv0.set(p0.x, p1.y); pv++;
    pv->p.set(EPS, EPS, EPS, 1.f); pv->uv0.set(p0.x, p0.y); pv++;
    pv->p.set(float(_w + EPS), float(_h + EPS), EPS, 1.f); pv->uv0.set(p1.x, p1.y); pv++;
    pv->p.set(float(_w + EPS), EPS, EPS, 1.f); pv->uv0.set(p1.x, p0.y); pv++;
    RCache.Vertex.Unlock(4, g_KD->vb_stride);

    params.z = 1.0;

    // Draw COLOR
    RCache.set_Element(s_sunshafts->E[1]);
    RCache.set_c("c_sunshafts", params);
    RCache.set_Geometry(g_KD);
    RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

    //***SECOND PASS***
    /*
    second blurring pass
    */
    // Set RT's
    u_setrt(rt_sunshafts_0, 0, 0, HW.pBaseZB);
    RCache.set_CullMode(CULL_NONE);
    RCache.set_Stencil(FALSE);

    // Fill vertex buffer
    pv = (v_aa*)RCache.Vertex.Lock(4, g_KD->vb_stride, Offset);
    pv->p.set(EPS, float(_h + EPS), EPS, 1.f); pv->uv0.set(p0.x, p1.y); pv++;
    pv->p.set(EPS, EPS, EPS, 1.f); pv->uv0.set(p0.x, p0.y); pv++;
    pv->p.set(float(_w + EPS), float(_h + EPS), EPS, 1.f); pv->uv0.set(p1.x, p1.y); pv++;
    pv->p.set(float(_w + EPS), EPS, EPS, 1.f); pv->uv0.set(p1.x, p0.y); pv++;
    RCache.Vertex.Unlock(4, g_KD->vb_stride);

    params.z = 0.7;

    // Draw COLOR
    RCache.set_Element(s_sunshafts->E[2]);
    RCache.set_c("c_sunshafts", params);
    RCache.set_Geometry(g_KD);
    RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

    //***THIRD PASS***
    /*
    third blurring pass
    */
    // Set RT's
    u_setrt(rt_sunshafts_1, 0, 0, HW.pBaseZB);
    RCache.set_CullMode(CULL_NONE);
    RCache.set_Stencil(FALSE);

    // Fill vertex buffer
    pv = (v_aa*)RCache.Vertex.Lock(4, g_KD->vb_stride, Offset);
    pv->p.set(EPS, float(_h + EPS), EPS, 1.f); pv->uv0.set(p0.x, p1.y); pv++;
    pv->p.set(EPS, EPS, EPS, 1.f); pv->uv0.set(p0.x, p0.y); pv++;
    pv->p.set(float(_w + EPS), float(_h + EPS), EPS, 1.f); pv->uv0.set(p1.x, p1.y); pv++;
    pv->p.set(float(_w + EPS), EPS, EPS, 1.f); pv->uv0.set(p1.x, p0.y); pv++;
    RCache.Vertex.Unlock(4, g_KD->vb_stride);

    params.z = 0.3;

    // Draw COLOR
    RCache.set_Element(s_sunshafts->E[3]);
    RCache.set_c("c_sunshafts", params);
    RCache.set_Geometry(g_KD);
    RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

    RCache.set_Stencil(FALSE);

    //***BLEND PASS***
    /*
    combining sunshafts texture and image fo further processing
    */
    // Set RT's
    u_setrt(rt_Generic, 0, 0, HW.pBaseZB);
    //u_setrt(rt_Generic_0, 0, 0, HW.pBaseZB);
    RCache.set_CullMode(CULL_NONE);
    RCache.set_Stencil(FALSE);

    // Fill vertex buffer
    pv = (v_aa*)RCache.Vertex.Lock(4, g_KD->vb_stride, Offset);
    pv->p.set(EPS, float(_h + EPS), EPS, 1.f); pv->uv0.set(p0.x, p1.y); pv++;
    pv->p.set(EPS, EPS, EPS, 1.f); pv->uv0.set(p0.x, p0.y); pv++;
    pv->p.set(float(_w + EPS), float(_h + EPS), EPS, 1.f); pv->uv0.set(p1.x, p1.y); pv++;
    pv->p.set(float(_w + EPS), EPS, EPS, 1.f); pv->uv0.set(p1.x, p0.y); pv++;
    RCache.Vertex.Unlock(4, g_KD->vb_stride);

    params.z = 0.0;

    // Draw COLOR
    RCache.set_Element(s_sunshafts->E[4]);
    RCache.set_c("c_sunshafts", params);
    RCache.set_Geometry(g_KD);
    RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);

    RCache.set_Stencil(FALSE);

    HW.pContext->CopyResource(rt_Generic_0->pTexture->surface_get(), rt_Generic->pTexture->surface_get());
};
