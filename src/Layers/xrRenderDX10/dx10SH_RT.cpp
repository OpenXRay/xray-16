#include "stdafx.h"
#pragma hdrstop

#include "Layers/xrRender/ResourceManager.h"
#include "dx10TextureUtils.h"

CRT::CRT()
{
    pSurface = NULL;
    pRT = NULL;
    pZRT = NULL;
#ifdef USE_DX11
    pUAView = NULL;
#endif
    dwWidth = 0;
    dwHeight = 0;
    fmt = D3DFMT_UNKNOWN;
}
CRT::~CRT()
{
    destroy();

    // release external reference
    RImplementation.Resources->_DeleteRT(this);
}

#ifdef USE_DX11
void CRT::create(LPCSTR Name, u32 w, u32 h, D3DFORMAT f, u32 SampleCount, bool useUAV)
#else
void CRT::create(LPCSTR Name, u32 w, u32 h, D3DFORMAT f, u32 SampleCount)
#endif
{
    if (pSurface)
        return;

    R_ASSERT(HW.pDevice && Name && Name[0] && w && h);
    _order = CPU::GetCLK(); // Device.GetTimerGlobal()->GetElapsed_clk();

    dwWidth = w;
    dwHeight = h;
    fmt = f;

    //	DirectX 10 supports non-power of two textures
    // Pow2
    // if (!btwIsPow2(w) || !btwIsPow2(h))
    //{
    //	if (!HW.Caps.raster.bNonPow2)	return;
    //}

    // Check width-and-height of render target surface
    if (w > D3D_REQ_TEXTURE2D_U_OR_V_DIMENSION)
        return;
    if (h > D3D_REQ_TEXTURE2D_U_OR_V_DIMENSION)
        return;

    // Select usage
    u32 usage = 0;
    if (D3DFMT_D24X8 == fmt)
        usage = D3DUSAGE_DEPTHSTENCIL;
    else if (D3DFMT_D15S1 == fmt)
        usage = D3DUSAGE_DEPTHSTENCIL;
    else if ((D3DFORMAT)MAKEFOURCC('D', 'F', '2', '4') == fmt)
        usage = D3DUSAGE_DEPTHSTENCIL;
    else
        usage = D3DUSAGE_RENDERTARGET;

    DXGI_FORMAT dx10FMT;

    switch (fmt)
    {
    case D3DFMT_D32S8X24:
        dx10FMT = DXGI_FORMAT_R32G8X24_TYPELESS;
        usage = D3DUSAGE_DEPTHSTENCIL;
        break;

    case D3DFMT_D24S8:
        dx10FMT = DXGI_FORMAT_R24G8_TYPELESS;
        usage = D3DUSAGE_DEPTHSTENCIL;
        break;

    case D3DFMT_D32F_LOCKABLE:
        dx10FMT = DXGI_FORMAT_R32_TYPELESS;
        usage = D3DUSAGE_DEPTHSTENCIL;
        break;

    case D3DFMT_D16_LOCKABLE:
        dx10FMT = DXGI_FORMAT_R16_TYPELESS;
        usage = D3DUSAGE_DEPTHSTENCIL;
        break;

    default:
        dx10FMT = dx10TextureUtils::ConvertTextureFormat(fmt);
        break;
    }

    const bool useAsDepth = usage != D3DUSAGE_RENDERTARGET;

    // Validate render-target usage
    UINT required = D3D_FORMAT_SUPPORT_TEXTURE2D;

    if (useAsDepth)
        required |= D3D_FORMAT_SUPPORT_DEPTH_STENCIL;
    else
        required |= D3D_FORMAT_SUPPORT_RENDER_TARGET;

    if (!HW.CheckFormatSupport(dx10FMT, required))
        return;

    // Try to create texture/surface
    RImplementation.Resources->Evict();

    // Create the render target texture
    D3D_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Width = dwWidth;
    desc.Height = dwHeight;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = dx10FMT;
    desc.SampleDesc.Count = SampleCount;
    desc.Usage = D3D_USAGE_DEFAULT;
    if (SampleCount <= 1)
        desc.BindFlags = D3D_BIND_SHADER_RESOURCE | (useAsDepth ? D3D_BIND_DEPTH_STENCIL : D3D_BIND_RENDER_TARGET);
    else
    {
        desc.BindFlags = (useAsDepth ? D3D_BIND_DEPTH_STENCIL : (D3D_BIND_SHADER_RESOURCE | D3D_BIND_RENDER_TARGET));
        if (RImplementation.o.dx10_msaa_opt)
        {
            desc.SampleDesc.Quality = UINT(D3D_STANDARD_MULTISAMPLE_PATTERN);
        }
    }

#ifdef USE_DX11
    if (HW.FeatureLevel >= D3D_FEATURE_LEVEL_11_0 && !useAsDepth && SampleCount == 1 && useUAV)
        desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
#endif

    CHK_DX(HW.pDevice->CreateTexture2D(&desc, NULL, &pSurface));
    HW.stats_manager.increment_stats_rtarget(pSurface);
    // OK
#ifdef DEBUG
    Msg("* created RT(%s), %dx%d, format = %d samples = %d", Name, w, h, dx10FMT, SampleCount);
#endif // DEBUG
    // R_CHK		(pSurface->GetSurfaceLevel	(0,&pRT)); // TODO: DX10: check if texture is created?
    if (useAsDepth)
    {
        D3D_DEPTH_STENCIL_VIEW_DESC ViewDesc;
        ZeroMemory(&ViewDesc, sizeof(ViewDesc));

        ViewDesc.Format = DXGI_FORMAT_UNKNOWN;
        if (SampleCount <= 1)
        {
            ViewDesc.ViewDimension = D3D_DSV_DIMENSION_TEXTURE2D;
        }
        else
        {
            ViewDesc.ViewDimension = D3D_DSV_DIMENSION_TEXTURE2DMS;
            ViewDesc.Texture2DMS.UnusedField_NothingToDefine = 0;
        }

        ViewDesc.Texture2D.MipSlice = 0;

        switch (desc.Format)
        {
        case DXGI_FORMAT_R16_TYPELESS:
            ViewDesc.Format = DXGI_FORMAT_D16_UNORM;
            break;

        case DXGI_FORMAT_R24G8_TYPELESS:
            ViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
            break;

        case DXGI_FORMAT_R32_TYPELESS:
            ViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
            break;

        case DXGI_FORMAT_R32G8X24_TYPELESS:
            ViewDesc.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
            break;
        }

        CHK_DX(HW.pDevice->CreateDepthStencilView(pSurface, &ViewDesc, &pZRT));
    }
    else
        CHK_DX(HW.pDevice->CreateRenderTargetView(pSurface, 0, &pRT));

#ifdef USE_DX11
    if (HW.FeatureLevel >= D3D_FEATURE_LEVEL_11_0 && !useAsDepth && SampleCount == 1 && useUAV)
    {
        D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
        ZeroMemory(&UAVDesc, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC));
        UAVDesc.Format = dx10FMT;
        UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
        UAVDesc.Buffer.FirstElement = 0;
        UAVDesc.Buffer.NumElements = dwWidth * dwHeight;
        CHK_DX(HW.pDevice->CreateUnorderedAccessView(pSurface, &UAVDesc, &pUAView));
    }
#endif

    pTexture = RImplementation.Resources->_CreateTexture(Name);
    pTexture->surface_set(pSurface);
}

void CRT::destroy()
{
    if (pTexture._get())
    {
        pTexture->surface_set(0);
        pTexture = NULL;
    }
    _RELEASE(pRT);
    _RELEASE(pZRT);

    HW.stats_manager.decrement_stats_rtarget(pSurface);
    _RELEASE(pSurface);
#ifdef USE_DX11
    _RELEASE(pUAView);
#endif
}
void CRT::reset_begin() { destroy(); }
void CRT::reset_end() { create(*cName, dwWidth, dwHeight, fmt); }
#ifdef USE_DX11
void resptrcode_crt::create(LPCSTR Name, u32 w, u32 h, D3DFORMAT f, u32 SampleCount, bool useUAV)
{
    _set(RImplementation.Resources->_CreateRT(Name, w, h, f, SampleCount, useUAV));
}
#else
void resptrcode_crt::create(LPCSTR Name, u32 w, u32 h, D3DFORMAT f, u32 SampleCount)
{
    _set(RImplementation.Resources->_CreateRT(Name, w, h, f, SampleCount));
}
#endif

//////////////////////////////////////////////////////////////////////////
/*	DX10 cut
CRTC::CRTC			()
{
    if (pSurface)	return;

    pSurface									= NULL;
    pRT[0]=pRT[1]=pRT[2]=pRT[3]=pRT[4]=pRT[5]	= NULL;
    dwSize										= 0;
    fmt											= D3DFMT_UNKNOWN;
}
CRTC::~CRTC			()
{
    destroy			();

    // release external reference
    DEV->_DeleteRTC	(this);
}

void CRTC::create	(LPCSTR Name, u32 size,	D3DFORMAT f)
{
    R_ASSERT	(HW.pDevice && Name && Name[0] && size && btwIsPow2(size));
    _order		= CPU::GetCLK();	//Device.GetTimerGlobal()->GetElapsed_clk();

    HRESULT		_hr;

    dwSize		= size;
    fmt			= f;

    // Get caps
    //D3DCAPS9	caps;
    //R_CHK		(HW.pDevice->GetDeviceCaps(&caps));

    //	DirectX 10 supports non-power of two textures
    // Pow2
    //if (!btwIsPow2(size))
    //{
    //	if (!HW.Caps.raster.bNonPow2)	return;
    //}

    // Check width-and-height of render target surface
    if (size>D3Dxx_REQ_TEXTURECUBE_DIMENSION)		return;

    //	TODO: DX10: Validate cube texture format
    // Validate render-target usage
    //_hr = HW.pD3D->CheckDeviceFormat(
    //	HW.DevAdapter,
    //	HW.m_DriverType,
    //	HW.Caps.fTarget,
    //	D3DUSAGE_RENDERTARGET,
    //	D3DRTYPE_CUBETEXTURE,
    //	f
    //	);
    //if (FAILED(_hr))					return;

    // Try to create texture/surface
    DEV->Evict					();
    _hr = HW.pDevice->CreateCubeTexture	(size, 1, D3DUSAGE_RENDERTARGET, f, D3DPOOL_DEFAULT, &pSurface,NULL);
    if (FAILED(_hr) || (0==pSurface))	return;

    // OK
    Msg			("* created RTc(%s), 6(%d)",Name,size);
    for (u32 face=0; face<6; face++)
        R_CHK	(pSurface->GetCubeMapSurface	((D3DCUBEMAP_FACES)face, 0, pRT+face));
    pTexture	= DEV->_CreateTexture	(Name);
    pTexture->surface_set						(pSurface);
}

void CRTC::destroy		()
{
    pTexture->surface_set	(0);
    pTexture				= NULL;
    for (u32 face=0; face<6; face++)
        _RELEASE	(pRT[face]	);
    _RELEASE	(pSurface	);
}
void CRTC::reset_begin	()
{
    destroy		();
}
void CRTC::reset_end	()
{
    create		(*cName,dwSize,fmt);
}

void resptrcode_crtc::create(LPCSTR Name, u32 size, D3DFORMAT f)
{
    _set		(DEV->_CreateRTC(Name,size,f));
}
*/
