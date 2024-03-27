#include "stdafx.h"

//#include "xr_effgamma.h"

#include "xrCore/Media/Image.hpp"
#include "xrEngine/xrImage_Resampler.h"

#include <DirectXTex.h>
#include <wincodec.h>

#define GAMESAVE_SIZE 128

void CRender::Screenshot(ScreenshotMode mode /*= SM_NORMAL*/, pcstr name /*= nullptr*/)
{
    ID3DResource* pSrcTexture;
    Target->get_base_rt()->GetResource(&pSrcTexture);

    if (!pSrcTexture)
    {
        Log("! Failed to make a screenshot: couldn't obtain base RT resource");
        return;
    }

    // Load source texture
    DirectX::ScratchImage image;
#if defined(USE_DX12)
    if (FAILED(CaptureTextureInDX12(HW.pDevice, HW.get_context(CHW::IMM_CTX_ID), pSrcTexture, image)))
#else
    if (FAILED(CaptureTexture(HW.pDevice, HW.get_context(CHW::IMM_CTX_ID), pSrcTexture, image)))
#endif
    {
        Log("! Failed to make a screenshot: couldn't capture texture");
        _RELEASE(pSrcTexture);
        return;
    }

    // Save
    switch (mode)
    {
    case IRender::SM_FOR_GAMESAVE:
    {
        // resize
        DirectX::ScratchImage resized;
        auto hr = Resize(*image.GetImage(0, 0, 0), GAMESAVE_SIZE, GAMESAVE_SIZE,
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
        auto hr = SaveToWICMemory(*image.GetImage(0, 0, 0), DirectX::WIC_FLAGS_NONE, GUID_ContainerFormatJpeg, saved);
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

        ID3DTexture2D* pTex = Target->t_ss_async;
        HW.get_context(CHW::IMM_CTX_ID)->CopyResource(pTex, pSrcTexture);

        D3D_MAPPED_TEXTURE2D MappedData;
        HW.get_context(CHW::IMM_CTX_ID)->Map(pTex, 0, D3D_MAP_READ, 0, &MappedData);
        // Swap r and b, but don't kill alpha
        {
            u32* pPixel = (u32*)MappedData.pData;
            u32* pEnd = pPixel + (Device.dwWidth * Device.dwHeight);

            for (; pPixel != pEnd; pPixel++)
            {
                u32 p = *pPixel;
                *pPixel = color_argb(color_get_A(p), color_get_B(p), color_get_G(p), color_get_R(p));
            }
        }
        // save
        u32* data = (u32*)xr_malloc(Device.dwHeight * Device.dwHeight * 4);
        imf_Process(data, Device.dwHeight, Device.dwHeight, (u32*)MappedData.pData, Device.dwWidth, Device.dwHeight, imf_lanczos3);
        HW.get_context(CHW::IMM_CTX_ID)->Unmap(pTex, 0);

        if (IWriter* fs = FS.w_open("$screenshots$", buf))
        {
            XRay::Media::Image img;
            img.Create(u16(Device.dwHeight), u16(Device.dwHeight), data, XRay::Media::ImageDataFormat::RGBA8);
            img.SaveTGA(*fs, true);
            FS.w_close(fs);
        }
        xr_free(data);
        break;
    }
    } // switch (mode)

_end_:
    _RELEASE(pSrcTexture);
}

void CRender::ScreenshotAsyncEnd(CMemoryWriter& memory_writer)
{
    VERIFY(!m_bMakeAsyncSS);

    // Don't own. No need to release.
    ID3DTexture2D* pTex = Target->t_ss_async;

    D3D_MAPPED_TEXTURE2D MappedData;

    HW.get_context(CHW::IMM_CTX_ID)->Map(pTex, 0, D3D_MAP_READ, 0, &MappedData);
    {
        u32* pPixel = (u32*)MappedData.pData;
        u32* pEnd = pPixel + (Device.dwWidth * Device.dwHeight);

        // Kill alpha and swap r and b.
        for (; pPixel != pEnd; pPixel++)
        {
            u32 p = *pPixel;
            *pPixel = color_xrgb(color_get_B(p), color_get_G(p), color_get_R(p));
        }

        memory_writer.w(&Device.dwWidth, sizeof(Device.dwWidth));
        memory_writer.w(&Device.dwHeight, sizeof(Device.dwHeight));
        memory_writer.w(MappedData.pData, (Device.dwWidth * Device.dwHeight) * 4);
    }
    HW.get_context(CHW::IMM_CTX_ID)->Unmap(pTex, 0);
}
