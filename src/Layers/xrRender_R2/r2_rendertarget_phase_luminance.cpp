#include "stdafx.h"

#if defined(USE_DX11)
#include <DirectXPackedVector.h>
#endif

namespace xray::render::RENDER_NAMESPACE
{
namespace phase_luminance
{
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
}

void CRenderTarget::phase_luminance()
{
    using namespace phase_luminance;

    // Freeze HDR auto-exposure while the in-session-load precache light is on.
    // restart_simulator() (F9/console/menu load of a same-level save) keeps the level
    // loaded and turns on an artificial white light at the camera during PreCache; metering
    // that would latch the exposure dark for seconds after the load. Skipping adaptation here
    // preserves the correct pre-load exposure across the reload. (A full level load shows a
    // loading screen instead and does NOT set this flag, so its exposure ramp is unaffected.)
    if (Device.dwPrecacheFrame && Device.b_precacheLight)
        return;

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
    u_setrt(RCache, rt_LUM_64, 0, 0, 0);
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
        v_build* pv = (v_build*)RImplementation.Vertex.Lock(4, g_bloom_build->vb_stride, Offset);

#if defined(USE_DX11)
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
#elif defined(USE_OGL)
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
#   error No graphics API selected or enabled!
#endif
        RImplementation.Vertex.Unlock(4, g_bloom_build->vb_stride);
        RCache.set_Element(s_luminance->E[0]);
        RCache.set_Geometry(g_bloom_build);
        RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
    }

    // 111: Perform LUM-SAT, pass 1, 64x64 => 8x8
    u_setrt(RCache, rt_LUM_8, 0, 0, 0);
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
        v_filter* pv = (v_filter*)RImplementation.Vertex.Lock(4, g_bloom_filter->vb_stride, Offset);
#if defined(USE_DX11)
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
#elif defined(USE_OGL)
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
#   error No graphics API selected or enabled!
#endif
        RImplementation.Vertex.Unlock(4, g_bloom_filter->vb_stride);
        RCache.set_Element(s_luminance->E[1]);
        RCache.set_Geometry(g_bloom_filter);
        RCache.Render(D3DPT_TRIANGLELIST, Offset, 0, 4, 0, 2);
    }

    // 222: Perform LUM-SAT, pass 2, 8x8 => 1x1
    u32 gpu_id = Device.dwFrame % HW.Caps.iGPUNum;
    u_setrt(RCache, rt_LUM_pool[gpu_id * 2 + 1], 0, 0, 0);
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
        v_filter* pv = (v_filter*)RImplementation.Vertex.Lock(4, g_bloom_filter->vb_stride, Offset);
#if defined(USE_DX11)
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
#elif defined(USE_OGL)
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
#   error No graphics API selected or enabled!
#endif
        RImplementation.Vertex.Unlock(4, g_bloom_filter->vb_stride);

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

    // Exposure sanitizer. dbg_exposure >= 0 pins rt_LUM to that value (manual debug override).
    // Otherwise, guard auto-exposure against a blown/NaN metered luminance: after an in-session
    // load a bad HDR value can drive the adapted exposure to ~0 (or NaN), crushing the whole
    // scene to black. We read the 1x1 exposure back periodically and, if it is non-finite or
    // below a visible floor, pin it to the last healthy exposure so the image can't go black.
    if (ps_dbg_exposure >= 0.f)
    {
        Fcolor c;
        c.set(ps_dbg_exposure, ps_dbg_exposure, ps_dbg_exposure, ps_dbg_exposure);
        RCache.ClearRT(rt_LUM_pool[gpu_id * 2 + 0], c);
        RCache.ClearRT(rt_LUM_pool[gpu_id * 2 + 1], c);
    }
#if defined(USE_DX11)
    else
    {
        static u32 s_rb_last = 0;
        static float s_clamp_to = -1.f; // <0 = healthy, no clamp applied
        static float s_last_good = 2.5f; // fallback exposure if we never saw a healthy value
        constexpr float FLOOR = 0.15f, CAP = 25.f;
        if (Device.dwFrame - s_rb_last >= 8)
        {
            s_rb_last = Device.dwFrame;
            D3D_TEXTURE2D_DESC d = {};
            d.Width = 1;
            d.Height = 1;
            d.MipLevels = 1;
            d.ArraySize = 1;
            d.SampleDesc.Count = 1;
            d.Format = DXGI_FORMAT_R32_FLOAT;
            d.Usage = D3D_USAGE_STAGING;
            d.CPUAccessFlags = D3D_CPU_ACCESS_READ;
            ID3DTexture2D* stage = nullptr;
            if (SUCCEEDED(HW.pDevice->CreateTexture2D(&d, nullptr, &stage)) && stage)
            {
                HW.get_context(CHW::IMM_CTX_ID)->CopyResource(stage, rt_LUM_pool[gpu_id * 2 + 1]->pSurface);
                D3D_MAPPED_TEXTURE2D md = {};
                if (SUCCEEDED(HW.get_context(CHW::IMM_CTX_ID)->Map(stage, 0, D3D_MAP_READ, 0, &md)))
                {
                    const float lum = *(const float*)md.pData;
                    HW.get_context(CHW::IMM_CTX_ID)->Unmap(stage, 0);
                    Msg("[PPDBG] rt_LUM=%g valid=%d adapt=%g paused=%d", lum, _valid(lum) ? 1 : 0,
                        f_luminance_adapt, Device.Paused() ? 1 : 0);
                    if (_valid(lum) && lum >= FLOOR && lum <= CAP)
                    {
                        s_last_good = lum;
                        s_clamp_to = -1.f;
                    }
                    else
                    {
                        s_clamp_to = _valid(lum) ? clampr(lum, FLOOR, CAP) : s_last_good;
                    }
                }
                _RELEASE(stage);
            }
        }
        if (s_clamp_to >= 0.f)
        {
            Fcolor c;
            c.set(s_clamp_to, s_clamp_to, s_clamp_to, s_clamp_to);
            RCache.ClearRT(rt_LUM_pool[gpu_id * 2 + 0], c);
            RCache.ClearRT(rt_LUM_pool[gpu_id * 2 + 1], c);
        }
    }

    // [PPDBG] Localize the HDR blowup: read back the 8x8 luminance grid and log the hottest cell.
    // cell(0,0) = top-left of screen; x grows right, y grows down. A single hot cell = a localized
    // bright source dragging auto-exposure down; a hot row/region points at sky/ground/etc.
    {
        static u32 s_grid_last = 0;
        if (Device.dwFrame - s_grid_last >= 60)
        {
            s_grid_last = Device.dwFrame;
            D3D_TEXTURE2D_DESC d = {};
            d.Width = 8;
            d.Height = 8;
            d.MipLevels = 1;
            d.ArraySize = 1;
            d.SampleDesc.Count = 1;
            d.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
            d.Usage = D3D_USAGE_STAGING;
            d.CPUAccessFlags = D3D_CPU_ACCESS_READ;
            ID3DTexture2D* stage = nullptr;
            if (SUCCEEDED(HW.pDevice->CreateTexture2D(&d, nullptr, &stage)) && stage)
            {
                HW.get_context(CHW::IMM_CTX_ID)->CopyResource(stage, rt_LUM_8->pSurface);
                D3D_MAPPED_TEXTURE2D md = {};
                if (SUCCEEDED(HW.get_context(CHW::IMM_CTX_ID)->Map(stage, 0, D3D_MAP_READ, 0, &md)))
                {
                    float mx = -1e30f, sum = 0.f;
                    int mxx = 0, mxy = 0;
                    for (int y = 0; y < 8; ++y)
                    {
                        const u16* row = (const u16*)((const u8*)md.pData + y * md.RowPitch);
                        for (int x = 0; x < 8; ++x)
                        {
                            const float r = DirectX::PackedVector::XMConvertHalfToFloat(row[x * 4 + 0]);
                            const float g = DirectX::PackedVector::XMConvertHalfToFloat(row[x * 4 + 1]);
                            const float b = DirectX::PackedVector::XMConvertHalfToFloat(row[x * 4 + 2]);
                            const float a = DirectX::PackedVector::XMConvertHalfToFloat(row[x * 4 + 3]);
                            const float lum = (r + g + b + a) * 0.25f;
                            sum += lum;
                            if (lum > mx)
                            {
                                mx = lum;
                                mxx = x;
                                mxy = y;
                            }
                        }
                    }
                    HW.get_context(CHW::IMM_CTX_ID)->Unmap(stage, 0);
                    Msg("[PPDBG] LUM8 max=%g @cell(%d,%d) mean=%g ratio=%.1f", mx, mxx, mxy, sum / 64.f,
                        mx / (sum / 64.f + 1e-6f));
                }
                _RELEASE(stage);
            }
        }
    }
#endif

    // Cleanup states
    RCache.set_Z(true);
}
} // namespace xray::render::RENDER_NAMESPACE
