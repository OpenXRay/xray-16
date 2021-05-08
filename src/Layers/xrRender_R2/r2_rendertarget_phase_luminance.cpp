#include "stdafx.h"

#pragma pack(push, 4)
struct v_build
{
    Fvector4 p;
    Fvector2 uv0;
    Fvector2 uv1;
    Fvector2 uv2;
    Fvector2 uv3;
};

struct v_filter
{
    Fvector4 p;
    Fvector4 uv[8];
};
#pragma pack(pop)

void CRenderTarget::phase_luminance()
{
    u32 Offset = 0;
#ifdef USE_DX9 // XXX: check why eps is 0 for other renderers
    float eps = EPS_S;
#else
    float eps = 0;
#endif

    // Targets
    RCache.set_Stencil(FALSE);
    RCache.set_CullMode(CULL_NONE);
    RCache.set_ColorWriteEnable();
    RCache.set_Z(false);

    // 000: Perform LUM-SAT, pass 0, 256x256 => 64x64
    u_setrt(rt_LUM_64, 0, 0, 0);
    {
        float ts = 64;
        float _w = float(BLOOM_size_X);
        float _h = float(BLOOM_size_Y);
        Fvector2 one = {2.f / _w, 2.f / _h}; // two, infact
        Fvector2 half = {1.f / _w, 1.f / _h}; // one, infact
        Fvector2 a_0 = {half.x + 0, half.y + 0};
        Fvector2 a_1 = {half.x + one.x, half.y + 0};
        Fvector2 a_2 = {half.x + 0, half.y + one.y};
        Fvector2 a_3 = {half.x + one.x, half.y + one.y};
        Fvector2 b_0 = {1 + a_0.x, 1 + a_0.y};
        Fvector2 b_1 = {1 + a_1.x, 1 + a_1.y};
        Fvector2 b_2 = {1 + a_2.x, 1 + a_2.y};
        Fvector2 b_3 = {1 + a_3.x, 1 + a_3.y};

        // Fill vertex buffer
        v_build* pv = (v_build*)RCache.Vertex.Lock(4, g_bloom_build->vb_stride, Offset);
#ifdef USE_OGL
        pv->p.set(eps, eps, eps, 1.f);
        pv->uv0.set(a_0.x, a_0.y);
        pv->uv1.set(a_1.x, a_1.y);
        pv->uv2.set(a_2.x, a_2.y);
        pv->uv3.set(a_3.x, a_3.y);
        pv++;
        pv->p.set(eps, float(ts + eps), eps, 1.f);
        pv->uv0.set(a_0.x, b_0.y);
        pv->uv1.set(a_1.x, b_1.y);
        pv->uv2.set(a_2.x, b_2.y);
        pv->uv3.set(a_3.x, b_3.y);
        pv++;
        pv->p.set(float(ts + eps), eps, eps, 1.f);
        pv->uv0.set(b_0.x, a_0.y);
        pv->uv1.set(b_1.x, a_1.y);
        pv->uv2.set(b_2.x, a_2.y);
        pv->uv3.set(b_3.x, a_3.y);
        pv++;
        pv->p.set(float(ts + eps), float(ts + eps), eps, 1.f);
        pv->uv0.set(b_0.x, b_0.y);
        pv->uv1.set(b_1.x, b_1.y);
        pv->uv2.set(b_2.x, b_2.y);
        pv->uv3.set(b_3.x, b_3.y);
        pv++;
#else
        pv->p.set(eps, float(ts + eps), eps, 1.f);
        pv->uv0.set(a_0.x, b_0.y);
        pv->uv1.set(a_1.x, b_1.y);
        pv->uv2.set(a_2.x, b_2.y);
        pv->uv3.set(a_3.x, b_3.y);
        pv++;
        pv->p.set(eps, eps, eps, 1.f);
        pv->uv0.set(a_0.x, a_0.y);
        pv->uv1.set(a_1.x, a_1.y);
        pv->uv2.set(a_2.x, a_2.y);
        pv->uv3.set(a_3.x, a_3.y);
        pv++;
        pv->p.set(float(ts + eps), float(ts + eps), eps, 1.f);
        pv->uv0.set(b_0.x, b_0.y);
        pv->uv1.set(b_1.x, b_1.y);
        pv->uv2.set(b_2.x, b_2.y);
        pv->uv3.set(b_3.x, b_3.y);
        pv++;
        pv->p.set(float(ts + eps), eps, eps, 1.f);
        pv->uv0.set(b_0.x, a_0.y);
        pv->uv1.set(b_1.x, a_1.y);
        pv->uv2.set(b_2.x, a_2.y);
        pv->uv3.set(b_3.x, a_3.y);
        pv++;
#endif // USE_OGL
        RCache.Vertex.Unlock(4, g_bloom_build->vb_stride);
        RCache.set_Element(s_luminance->E[0]);
        RCache.set_Geometry(g_bloom_build);
        RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
    }

    // 111: Perform LUM-SAT, pass 1, 64x64 => 8x8
    u_setrt(rt_LUM_8, 0, 0, 0);
    {
        // Build filter-kernel
        float _ts = 8;
        float _src = float(64);
        Fvector2 a[16], b[16];
        for (int k = 0; k < 16; k++)
        {
            int _x = (k * 2 + 1) % 8; // 1,3,5,7
            int _y = ((k / 4) * 2 + 1); // 1,1,1,1 ~ 3,3,3,3 ~...etc...
            a[k].set(_x, _y).div(_src);
            b[k].set(a[k]).add(1);
        }

        // Fill vertex buffer
        v_filter* pv = (v_filter*)RCache.Vertex.Lock(4, g_bloom_filter->vb_stride, Offset);
#ifdef USE_OGL
        pv->p.set(eps, eps, eps, 1.f);
        for (int t = 0; t < 8; t++)
            pv->uv[t].set(a[t].x, a[t].y, a[t + 8].y, a[t + 8].x); // xy/yx	- left+up
        pv++;
        pv->p.set(eps, float(_ts + eps), eps, 1.f);
        for (int t = 0; t < 8; t++)
            pv->uv[t].set(a[t].x, b[t].y, b[t + 8].y, a[t + 8].x); // xy/yx	- left+down
        pv++;
        pv->p.set(float(_ts + eps), eps, eps, 1.f);
        for (int t = 0; t < 8; t++)
            pv->uv[t].set(b[t].x, a[t].y, a[t + 8].y, b[t + 8].x); // xy/yx	- right+up
        pv++;
        pv->p.set(float(_ts + eps), float(_ts + eps), eps, 1.f);
        for (int t = 0; t < 8; t++)
            pv->uv[t].set(b[t].x, b[t].y, b[t + 8].y, b[t + 8].x); // xy/yx	- right+down
        pv++;
#else
        pv->p.set(eps, float(_ts + eps), eps, 1.f);
        for (int t = 0; t < 8; t++)
            pv->uv[t].set(a[t].x, b[t].y, b[t + 8].y, a[t + 8].x); // xy/yx	- left+down
        pv++;
        pv->p.set(eps, eps, eps, 1.f);
        for (int t = 0; t < 8; t++)
            pv->uv[t].set(a[t].x, a[t].y, a[t + 8].y, a[t + 8].x); // xy/yx	- left+up
        pv++;
        pv->p.set(float(_ts + eps), float(_ts + eps), eps, 1.f);
        for (int t = 0; t < 8; t++)
            pv->uv[t].set(b[t].x, b[t].y, b[t + 8].y, b[t + 8].x); // xy/yx	- right+down
        pv++;
        pv->p.set(float(_ts + eps), eps, eps, 1.f);
        for (int t = 0; t < 8; t++)
            pv->uv[t].set(b[t].x, a[t].y, a[t + 8].y, b[t + 8].x); // xy/yx	- right+up
        pv++;
#endif // USE_OGL
        RCache.Vertex.Unlock(4, g_bloom_filter->vb_stride);
        RCache.set_Element(s_luminance->E[1]);
        RCache.set_Geometry(g_bloom_filter);
        RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
    }

    // 222: Perform LUM-SAT, pass 2, 8x8 => 1x1
    u32 gpu_id = Device.dwFrame % HW.Caps.iGPUNum;
    u_setrt(rt_LUM_pool[gpu_id * 2 + 1], 0, 0, 0);
    {
        // Build filter-kernel
        float _ts = 1;
        float _src = float(8);
        Fvector2 a[16], b[16];
        for (int k = 0; k < 16; k++)
        {
            int _x = (k * 2 + 1) % 8; // 1,3,5,7
            int _y = ((k / 4) * 2 + 1); // 1,1,1,1 ~ 3,3,3,3 ~...etc...
            a[k].set(_x, _y).div(_src);
            b[k].set(a[k]).add(1);
        }

        // Fill vertex buffer
        v_filter* pv = (v_filter*)RCache.Vertex.Lock(4, g_bloom_filter->vb_stride, Offset);
#ifdef USE_OGL
        pv->p.set(eps, eps, eps, 1.f);
        for (int t = 0; t < 8; t++)
            pv->uv[t].set(a[t].x, a[t].y, a[t + 8].y, a[t + 8].x); // xy/yx	- left+up
        pv++;
        pv->p.set(eps, float(_ts + eps), eps, 1.f);
        for (int t = 0; t < 8; t++)
            pv->uv[t].set(a[t].x, b[t].y, b[t + 8].y, a[t + 8].x); // xy/yx	- left+down
        pv++;
        pv->p.set(float(_ts + eps), eps, eps, 1.f);
        for (int t = 0; t < 8; t++)
            pv->uv[t].set(b[t].x, a[t].y, a[t + 8].y, b[t + 8].x); // xy/yx	- right+up
        pv++;
        pv->p.set(float(_ts + eps), float(_ts + eps), eps, 1.f);
        for (int t = 0; t < 8; t++)
            pv->uv[t].set(b[t].x, b[t].y, b[t + 8].y, b[t + 8].x); // xy/yx	- right+down
        pv++;
#else
        pv->p.set(eps, float(_ts + eps), eps, 1.f);
        for (int t = 0; t < 8; t++)
            pv->uv[t].set(a[t].x, b[t].y, b[t + 8].y, a[t + 8].x); // xy/yx	- left+down
        pv++;
        pv->p.set(eps, eps, eps, 1.f);
        for (int t = 0; t < 8; t++)
            pv->uv[t].set(a[t].x, a[t].y, a[t + 8].y, a[t + 8].x); // xy/yx	- left+up
        pv++;
        pv->p.set(float(_ts + eps), float(_ts + eps), eps, 1.f);
        for (int t = 0; t < 8; t++)
            pv->uv[t].set(b[t].x, b[t].y, b[t + 8].y, b[t + 8].x); // xy/yx	- right+down
        pv++;
        pv->p.set(float(_ts + eps), eps, eps, 1.f);
        for (int t = 0; t < 8; t++)
            pv->uv[t].set(b[t].x, a[t].y, a[t + 8].y, b[t + 8].x); // xy/yx	- right+up
        pv++;
#endif // USE_OGL
        RCache.Vertex.Unlock(4, g_bloom_filter->vb_stride);

        f_luminance_adapt = .9f * f_luminance_adapt + .1f * Device.fTimeDelta * ps_r2_tonemap_adaptation;
        float amount = ps_r2_ls_flags.test(R2FLAG_TONEMAP) ? ps_r2_tonemap_amount : 0;
        Fvector3 _none, _full, _result;
        _none.set(1, 0, 1);
        _full.set(ps_r2_tonemap_middlegray, 1.f, ps_r2_tonemap_low_lum);
        _result.lerp(_none, _full, amount);

        RCache.set_Element(s_luminance->E[2]);
        RCache.set_Geometry(g_bloom_filter);
        RCache.set_c("MiddleGray", _result.x, _result.y, _result.z, f_luminance_adapt);
        RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
    }

    // Cleanup states
    RCache.set_Z(true);
}
