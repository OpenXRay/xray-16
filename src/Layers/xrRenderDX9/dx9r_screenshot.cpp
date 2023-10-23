#include "stdafx.h"

//#include "xr_effgamma.h"
#include "xrEngine/xrImage_Resampler.h"

#include <DirectXPackedVector.h>
#include <DirectXTex.h>
#include <wincodec.h>

#define GAMESAVE_SIZE 128

void CRender::Screenshot(ScreenshotMode mode /*= SM_NORMAL*/, pcstr name /*= nullptr*/)
{
    if (!Device.b_is_Ready)
        return;

    // Create temp-surface
    DirectX::ScratchImage image;
    u32* pPixel = nullptr;
    u32* pEnd = nullptr;
    u32* pDst = nullptr;
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
    hr = image.Initialize2D(DXGI_FORMAT_B8G8R8A8_UNORM, Device.dwWidth, Device.dwHeight, 1, 1);
    if (FAILED(hr))
        goto _end_;
    // Image processing (gamma-correct)
    pDst = (u32*)image.GetPixels();
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
    for (; pPixel != pEnd; pPixel++, pDst++)
    {
        u32 p = *pPixel;
        *pDst = color_xrgb(color_get_R(p), color_get_G(p), color_get_B(p));
    }

    hr = pFB->UnlockRect();
    if (hr != D3D_OK)
        goto _end_;

    // Save
    switch (mode)
    {
    case IRender::SM_FOR_GAMESAVE:
    {
        // resize
        DirectX::ScratchImage resized;
        hr = Resize(*image.GetImage(0, 0, 0), GAMESAVE_SIZE, GAMESAVE_SIZE,
            DirectX::TEX_FILTER_BOX, resized);
        if (FAILED(hr))
            goto _end_;

        // compress
        DirectX::ScratchImage compressed;
        hr = Compress(*resized.GetImage(0, 0, 0), DXGI_FORMAT_BC1_UNORM,
            DirectX::TEX_COMPRESS_DEFAULT | DirectX::TEX_COMPRESS_PARALLEL, 0.0f, compressed);
        if (FAILED(hr))
            goto _end_;

        // save (logical & physical)
        DirectX::Blob saved;
        hr = SaveToDDSMemory(*compressed.GetImage(0, 0, 0), DirectX::DDS_FLAGS_FORCE_DX9_LEGACY, saved);
        if (FAILED(hr))
            goto _end_;

        if (IWriter* fs = FS.w_open(name))
        {
            fs->w(saved.GetBufferPointer(), saved.GetBufferSize());
            FS.w_close(fs);
        }
        break;
    }
    case IRender::SM_NORMAL:
    {
        string64 t_stemp;
        string_path buf;
        xr_sprintf(buf, "ss_%s_%s_(%s).jpg", Core.UserName, timestamp(t_stemp), g_pGameLevel ? g_pGameLevel->name().c_str() : "mainmenu");

        DirectX::Blob saved;
        hr = SaveToWICMemory(*image.GetImage(0, 0, 0), DirectX::WIC_FLAGS_NONE, GUID_ContainerFormatJpeg, saved);
        if (SUCCEEDED(hr))
        {
            if (IWriter* fs = FS.w_open("$screenshots$", buf))
            {
                fs->w(saved.GetBufferPointer(), saved.GetBufferSize());
                FS.w_close(fs);
            }
        }

        // hq
        if (strstr(Core.Params, "-ss_tga"))
        {
            xr_sprintf(buf, "ssq_%s_%s_(%s).tga", Core.UserName, timestamp(t_stemp), g_pGameLevel ? g_pGameLevel->name().c_str() : "mainmenu");

            hr = SaveToTGAMemory(*image.GetImage(0, 0, 0), saved);
            if (FAILED(hr))
                goto _end_;

            if (IWriter* fs = FS.w_open("$screenshots$", buf))
            {
                fs->w(saved.GetBufferPointer(), saved.GetBufferSize());
                FS.w_close(fs);
            }
        }
        break;
    }
    case IRender::SM_FOR_LEVELMAP:
    case IRender::SM_FOR_CUBEMAP:
    {
        string_path buf;
        VERIFY(name);
        strconcat(sizeof(buf), buf, name, ".tga");

        DirectX::ScratchImage img;
        hr = img.Initialize2D(image.GetMetadata().format, Device.dwHeight, Device.dwHeight, 1, 1);
        if (FAILED(hr))
            goto _end_;

        imf_Process((u32*)img.GetPixels(), Device.dwHeight, Device.dwHeight, (u32*)image.GetPixels(), Device.dwWidth, Device.dwHeight, imf_lanczos3);

        DirectX::Blob saved;
        hr = DirectX::SaveToTGAMemory(*img.GetImage(0, 0, 0), saved);
        if (FAILED(hr))
            goto _end_;

        if (IWriter* fs = FS.w_open("$screenshots$", buf))
        {
            fs->w(saved.GetBufferPointer(), saved.GetBufferSize());
            FS.w_close(fs);
        }
        break;
    }
    } // switch (mode)

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
        auto* pPixelElement16 = (DirectX::PackedVector::HALF*)pPixel;

        float tmpArray[4 * iMaxPixelsInARow];
        while (pPixel != pEnd)
        {
            const int iProcessPixels = _min(iMaxPixelsInARow, (s32)(pEnd - pPixel));

            DirectX::PackedVector::XMConvertHalfToFloatStream(tmpArray, sizeof(float),
                pPixelElement16, sizeof(DirectX::PackedVector::HALF), iProcessPixels * 4);

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
