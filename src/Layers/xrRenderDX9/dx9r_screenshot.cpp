#include "stdafx.h"
//#include "xr_effgamma.h"
#include "xrCore/Media/Image.hpp"
#include "xrEngine/xrImage_Resampler.h"

using namespace XRay::Media;

#define GAMESAVE_SIZE 128

#define SM_FOR_SEND_WIDTH 640
#define SM_FOR_SEND_HEIGHT 480

void CRender::ScreenshotImpl(ScreenshotMode mode, LPCSTR name, CMemoryWriter* memory_writer)
{
    if (!Device.b_is_Ready)
        return;
    // Create temp-surface
    u32* pPixel = nullptr;
    u32* pEnd = nullptr;
    IDirect3DSurface9* pFB;
    D3DLOCKED_RECT D;
    HRESULT hr;
    hr = HW.pDevice->CreateOffscreenPlainSurface(
        Device.dwWidth, Device.dwHeight, HW.Caps.fTarget, D3DPOOL_SYSTEMMEM, &pFB, nullptr);
    if (FAILED(hr))
        return;
    hr = HW.pDevice->GetRenderTargetData(Target->get_base_rt(), pFB);
    if (FAILED(hr))
        goto _end_;
    hr = pFB->LockRect(&D, nullptr, D3DLOCK_NOSYSLOCK);
    if (FAILED(hr))
        goto _end_;
    // Image processing (gamma-correct)
    pPixel = (u32*)D.pBits;
    pEnd = pPixel + (Device.dwWidth * Device.dwHeight);
    //	IGOR: Remove inverse color correction and kill alpha
    /*
    D3DGAMMARAMP	G;
    dxRenderDeviceRender::Instance().gammaGenLUT(G);
    for (int i=0; i<256; i++) {
        G.red	[i]	/= 256;
        G.green	[i]	/= 256;
        G.blue	[i]	/= 256;
    }
    for (;pPixel!=pEnd; pPixel++)	{
        u32 p = *pPixel;
        *pPixel = color_xrgb	(
            G.red	[color_get_R(p)],
            G.green	[color_get_G(p)],
            G.blue	[color_get_B(p)]
            );
    }
    */

    // Kill alpha
    for (; pPixel != pEnd; pPixel++)
    {
        u32 p = *pPixel;
        *pPixel = color_xrgb(color_get_R(p), color_get_G(p), color_get_B(p));
    }

    hr = pFB->UnlockRect();
    if (hr != D3D_OK)
        goto _end_;

    // Save
    switch (mode)
    {
    case IRender::SM_FOR_GAMESAVE:
    {
        // texture
        ID3DTexture2D* texture = nullptr;
        hr = D3DXCreateTexture(HW.pDevice, GAMESAVE_SIZE, GAMESAVE_SIZE, 1, 0, D3DFMT_DXT1, D3DPOOL_SCRATCH, &texture);
        if (hr != D3D_OK)
            goto _end_;
        if (nullptr == texture)
            goto _end_;

        // resize&convert to surface
        IDirect3DSurface9* surface = nullptr;
        hr = texture->GetSurfaceLevel(0, &surface);
        if (hr != D3D_OK)
            goto _end_;
        VERIFY(surface);
        hr = D3DXLoadSurfaceFromSurface(surface, nullptr, nullptr, pFB, nullptr, nullptr, D3DX_DEFAULT, 0);
        _RELEASE(surface);
        if (hr != D3D_OK)
            goto _end_;

        // save (logical & physical)
        ID3DBlob* saved = nullptr;
        hr = D3DXSaveTextureToFileInMemory(&saved, D3DXIFF_DDS, texture, nullptr);
        if (hr != D3D_OK)
            goto _end_;

        IWriter* fs = FS.w_open(name);
        if (fs)
        {
            fs->w(saved->GetBufferPointer(), saved->GetBufferSize());
            FS.w_close(fs);
        }
        _RELEASE(saved);

        // cleanup
        _RELEASE(texture);
    }
        break;
    case IRender::SM_FOR_MPSENDING:
    {
        // texture
        ID3DTexture2D* texture = nullptr;
        hr = D3DXCreateTexture(
            HW.pDevice, SM_FOR_SEND_WIDTH, SM_FOR_SEND_HEIGHT, 1, 0, D3DFMT_R8G8B8, D3DPOOL_SCRATCH, &texture);
        if (hr != D3D_OK)
            goto _end_;
        if (nullptr == texture)
            goto _end_;

        // resize&convert to surface
        IDirect3DSurface9* surface = nullptr;
        hr = texture->GetSurfaceLevel(0, &surface);
        if (hr != D3D_OK)
            goto _end_;
        VERIFY(surface);
        hr = D3DXLoadSurfaceFromSurface(surface, nullptr, nullptr, pFB, nullptr, nullptr, D3DX_DEFAULT, 0);
        _RELEASE(surface);
        if (hr != D3D_OK)
            goto _end_;

        // save (logical & physical)
        ID3DBlob* saved = nullptr;
        hr = D3DXSaveTextureToFileInMemory(&saved, D3DXIFF_DDS, texture, nullptr);
        if (hr != D3D_OK)
            goto _end_;

        if (!memory_writer)
        {
            IWriter* fs = FS.w_open(name);
            if (fs)
            {
                fs->w(saved->GetBufferPointer(), saved->GetBufferSize());
                FS.w_close(fs);
            }
        }
        else
        {
            memory_writer->w(saved->GetBufferPointer(), saved->GetBufferSize());
        }

        _RELEASE(saved);

        // cleanup
        _RELEASE(texture);
    }
        break;
    case IRender::SM_NORMAL:
    {
        string64 t_stemp;
        string_path buf;
        xr_sprintf(buf, sizeof buf, "ss_%s_%s_(%s).jpg", Core.UserName, timestamp(t_stemp),
                   g_pGameLevel ? g_pGameLevel->name().c_str() : "mainmenu");
        ID3DBlob* saved = nullptr;
        CHK_DX(D3DXSaveSurfaceToFileInMemory(&saved, D3DXIFF_JPG, pFB, nullptr, nullptr));
        IWriter* fs = FS.w_open("$screenshots$", buf);
        R_ASSERT(fs);
        fs->w(saved->GetBufferPointer(), saved->GetBufferSize());
        FS.w_close(fs);
        _RELEASE(saved);
        if (strstr(Core.Params, "-ss_tga"))
        { // hq
            xr_sprintf(buf, sizeof(buf), "ssq_%s_%s_(%s).tga", Core.UserName, timestamp(t_stemp),
                       (g_pGameLevel) ? g_pGameLevel->name().c_str() : "mainmenu");
            ID3DBlob* saved = nullptr;
            CHK_DX(D3DXSaveSurfaceToFileInMemory(&saved, D3DXIFF_TGA, pFB, nullptr, nullptr));
            IWriter* fs = FS.w_open("$screenshots$", buf);
            R_ASSERT(fs);
            fs->w(saved->GetBufferPointer(), saved->GetBufferSize());
            FS.w_close(fs);
            _RELEASE(saved);
        }
    }
        break;
    case IRender::SM_FOR_LEVELMAP:
    case IRender::SM_FOR_CUBEMAP:
    {
        // string64 t_stemp;
        string_path buf;
        VERIFY(name);
        strconcat(sizeof(buf), buf, name, ".tga");
        IWriter* fs = FS.w_open("$screenshots$", buf);
        R_ASSERT(fs);
        // TODO: DX11: This is totally incorrect but mimics
        // original behavior. Fix later.
        hr = pFB->LockRect(&D, nullptr, D3DLOCK_NOSYSLOCK);
        if (hr != D3D_OK)
            return;
        hr = pFB->UnlockRect();
        if (hr != D3D_OK)
            goto _end_;

        // save
        u32* data = (u32*)xr_malloc(Device.dwHeight * Device.dwHeight * 4);
        imf_Process(data, Device.dwHeight, Device.dwHeight, (u32*)D.pBits, Device.dwWidth, Device.dwHeight, imf_lanczos3);
        Image img;
        img.Create(u16(Device.dwHeight), u16(Device.dwHeight), data, ImageDataFormat::RGBA8);
        img.SaveTGA(*fs, true);
        xr_free(data);
        FS.w_close(fs);
    }
        break;
    }

_end_:
    _RELEASE(pFB);
}

void CRender::ScreenshotAsyncEnd(CMemoryWriter& memory_writer)
{
    if (!Device.b_is_Ready)
        return;

    VERIFY(!m_bMakeAsyncSS);

    IDirect3DSurface9* pFB = Target->rt_async_ss->pRT;

    D3DLOCKED_RECT D;
    const HRESULT hr = pFB->LockRect(&D, nullptr, D3DLOCK_NOSYSLOCK);
    if (hr != D3D_OK)
        return;

#if RENDER == R_R1
    u32 rtWidth = Target->get_rtwidth();
    u32 rtHeight = Target->get_rtheight();
#else // RENDER != R_R1
    u32 rtWidth = Device.dwWidth;
    u32 rtHeight = Device.dwHeight;
#endif // RENDER != R_R1

    // Image processing (gamma-correct)
    u32* pPixel = (u32*)D.pBits;
    u32* pOrigin = pPixel;
    u32* pEnd = pPixel + (rtWidth * rtHeight);

    // Kill alpha
#if RENDER != R_R1
    if (Target->rt_Color->fmt == D3DFMT_A16B16G16R16F)
    {
        static const int iMaxPixelsInARow = 1024;
        D3DXFLOAT16* pPixelElement16 = (D3DXFLOAT16*)pPixel;

        float tmpArray[4 * iMaxPixelsInARow];
        while (pPixel != pEnd)
        {
            const int iProcessPixels = _min(iMaxPixelsInARow, (s32)(pEnd - pPixel));

            D3DXFloat16To32Array(tmpArray, pPixelElement16, iProcessPixels * 4);

            for (int i = 0; i < iProcessPixels; ++i)
            {
                *pPixel = color_argb_f(1.0f, tmpArray[i * 4], tmpArray[i * 4 + 1], tmpArray[i * 4 + 2]);

                ++pPixel;
            }

            pPixelElement16 += iProcessPixels * 4;
        }
    }
    else
#endif // RENDER != R_R1
    {
        for (; pPixel != pEnd; pPixel++)
        {
            u32 p = *pPixel;
            *pPixel = color_xrgb(color_get_R(p), color_get_G(p), color_get_B(p));
        }
    }

    {
        memory_writer.w(&rtWidth, sizeof(rtWidth));
        memory_writer.w(&rtHeight, sizeof(rtHeight));
        memory_writer.w(pOrigin, (rtWidth * rtHeight) * 4);
    }

    CHK_DX(pFB->UnlockRect());
}
