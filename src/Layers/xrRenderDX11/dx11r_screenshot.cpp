#include "stdafx.h"
//#include "xr_effgamma.h"
#include "xrCore/Media/Image.hpp"
#include "xrEngine/xrImage_Resampler.h"

#include "d3dx11tex.h"

using namespace XRay::Media;

#define GAMESAVE_SIZE 128

#define SM_FOR_SEND_WIDTH 640
#define SM_FOR_SEND_HEIGHT 480

void CRender::ScreenshotImpl(ScreenshotMode mode, LPCSTR name, CMemoryWriter* memory_writer)
{
    ID3DResource* pSrcTexture;
    Target->get_base_rt()->GetResource(&pSrcTexture);

    VERIFY(pSrcTexture);

    // Save
    switch (mode)
    {
    case IRender::SM_FOR_GAMESAVE:
    {
        ID3DTexture2D* pSrcSmallTexture;

        D3D_TEXTURE2D_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Width = GAMESAVE_SIZE;
        desc.Height = GAMESAVE_SIZE;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_BC1_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D_USAGE_DEFAULT;
        desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
        CHK_DX(HW.pDevice->CreateTexture2D(&desc, NULL, &pSrcSmallTexture));

        CHK_DX(D3DX11LoadTextureFromTexture(HW.get_context(CHW::IMM_CTX_ID), pSrcTexture, NULL, pSrcSmallTexture));

        // save (logical & physical)
        ID3DBlob* saved = nullptr;
        HRESULT hr = D3DX11SaveTextureToMemory(HW.get_context(CHW::IMM_CTX_ID), pSrcSmallTexture, D3DX11_IFF_DDS, &saved, 0);
        if (hr == D3D_OK)
        {
            IWriter* fs = FS.w_open(name);
            if (fs)
            {
                fs->w(saved->GetBufferPointer(), saved->GetBufferSize());
                FS.w_close(fs);
            }
        }
        _RELEASE(saved);

        // cleanup
        _RELEASE(pSrcSmallTexture);
    }
    break;
    case IRender::SM_FOR_MPSENDING:
    {
        ID3DTexture2D* pSrcSmallTexture;

        D3D_TEXTURE2D_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Width = SM_FOR_SEND_WIDTH;
        desc.Height = SM_FOR_SEND_HEIGHT;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_BC1_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D_USAGE_DEFAULT;
        desc.BindFlags = D3D_BIND_SHADER_RESOURCE;
        CHK_DX(HW.pDevice->CreateTexture2D(&desc, NULL, &pSrcSmallTexture));

        CHK_DX(D3DX11LoadTextureFromTexture(HW.get_context(CHW::IMM_CTX_ID), pSrcTexture, NULL, pSrcSmallTexture));

        // save (logical & physical)
        ID3DBlob* saved = nullptr;
        HRESULT hr = D3DX11SaveTextureToMemory(HW.get_context(CHW::IMM_CTX_ID), pSrcSmallTexture, D3DX11_IFF_DDS, &saved, 0);
        if (hr == D3D_OK)
        {
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
        }
        _RELEASE(saved);

        // cleanup
        _RELEASE(pSrcSmallTexture);
    }
        break;
    case IRender::SM_NORMAL:
    {
        string64 t_stemp;
        string_path buf;
        xr_sprintf(buf, sizeof buf, "ss_%s_%s_(%s).jpg", Core.UserName, timestamp(t_stemp),
                   g_pGameLevel ? g_pGameLevel->name().c_str() : "mainmenu");

        ID3DBlob* saved = nullptr;
        CHK_DX(D3DX11SaveTextureToMemory(HW.get_context(CHW::IMM_CTX_ID), pSrcTexture, D3DX11_IFF_JPG, &saved, 0));

        IWriter* fs = FS.w_open("$screenshots$", buf);
        R_ASSERT(fs);
        fs->w(saved->GetBufferPointer(), saved->GetBufferSize());
        FS.w_close(fs);
        _RELEASE(saved);

        if (strstr(Core.Params, "-ss_tga"))
        { // hq
            xr_sprintf(buf, sizeof buf, "ssq_%s_%s_(%s).tga", Core.UserName, timestamp(t_stemp),
                       g_pGameLevel ? g_pGameLevel->name().c_str() : "mainmenu");

            ID3DBlob* saved2 = nullptr;
            CHK_DX(D3DX11SaveTextureToMemory(HW.get_context(CHW::IMM_CTX_ID), pSrcTexture, D3DX11_IFF_BMP, &saved2, 0));

            IWriter* fs2 = FS.w_open("$screenshots$", buf);
            R_ASSERT(fs2);
            fs2->w(saved2->GetBufferPointer(), saved2->GetBufferSize());
            FS.w_close(fs2);
            _RELEASE(saved2);
        }
    }
        break;
    case IRender::SM_FOR_LEVELMAP:
    case IRender::SM_FOR_CUBEMAP:
    {
        string_path buf;
        VERIFY(name);
        strconcat(sizeof(buf), buf, name, ".tga");
        IWriter* fs = FS.w_open("$screenshots$", buf);
        R_ASSERT(fs);

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

        Image img;
        img.Create(u16(Device.dwHeight), u16(Device.dwHeight), data, ImageDataFormat::RGBA8);
        img.SaveTGA(*fs, true);
        xr_free(data);
        FS.w_close(fs);
    }
        break;
    }

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
