#include "stdafx.h"
#pragma hdrstop

#include "Layers/xrRender/ResourceManager.h"

CRT::CRT()
{
    pSurface = NULL;
    pRT = NULL;
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

bool CRT::used_as_depth() const
{
    switch (fmt)
    {
    case D3DFMT_D16:
    case D3DFMT_D16_LOCKABLE:
    case D3DFMT_D24X8:
    case D3DFMT_D32:
    case D3DFMT_D15S1:
    case D3DFMT_D24X4S4:
    case D3DFMT_D24S8:
    case MAKEFOURCC('D', 'F', '2', '4'):
        return true;
    default:
        return false;
    }
}

void CRT::create(LPCSTR Name, u32 w, u32 h, D3DFORMAT f, u32 SampleCount /*= 1*/, Flags32 flags /*= {}*/)
{
    if (pSurface)
        return;

    R_ASSERT(HW.pDevice && Name && Name[0] && w && h);
    _order = CPU::GetCLK(); // RDEVICE.GetTimerGlobal()->GetElapsed_clk();

    HRESULT _hr;

    dwWidth = w;
    dwHeight = h;
    fmt = f;
    sampleCount = SampleCount;

    if (flags.test(CreateBase))
    {
        dwFlags |= CreateBase;
        if (used_as_depth())
            R_CHK(HW.pDevice->GetDepthStencilSurface(&pRT));
        else
        {
            u32 idx;
            char const* str = strrchr(Name, '_');
            sscanf(++str, "%d", &idx);
            R_CHK(HW.pDevice->GetRenderTarget(idx, &pRT));
        }
#ifdef DEBUG
        Msg("* created RT(%s), %dx%d", Name, w, h);
#endif
        return;
    }

    D3DRESOURCETYPE type = D3DRTYPE_TEXTURE;
    if (flags.test(CreateSurface))
    {
        type = D3DRTYPE_SURFACE;
        dwFlags |= CreateSurface;
    }

    // Get caps
    D3DCAPS9 caps;
    R_CHK(HW.pDevice->GetDeviceCaps(&caps));

    // Pow2
    if (!btwIsPow2(w) || !btwIsPow2(h))
    {
        if (!HW.Caps.raster.bNonPow2)
            return;
    }

    // Check width-and-height of render target surface
    if (w > caps.MaxTextureWidth)
        return;
    if (h > caps.MaxTextureHeight)
        return;

    // Select usage
    const bool useAsDepth = used_as_depth();
    const u32 usage = useAsDepth ? D3DUSAGE_DEPTHSTENCIL : D3DUSAGE_RENDERTARGET;

    // Validate render-target usage
    _hr = HW.pD3D->CheckDeviceFormat(HW.DevAdapter, HW.m_DriverType, HW.Caps.fTarget, usage, type, f);
    if (FAILED(_hr))
        return;

    // Try to create texture/surface
    RImplementation.Resources->Evict();

    switch (type)
    {
    case D3DRTYPE_TEXTURE:
    {
        _hr = HW.pDevice->CreateTexture(w, h, 1, usage, f, D3DPOOL_DEFAULT, &pSurface, nullptr);
        break;
    }
    case D3DRTYPE_SURFACE:
    {
        if (useAsDepth)
            _hr = HW.pDevice->CreateDepthStencilSurface(w, h, f, D3DMULTISAMPLE_NONE, 0, TRUE, &pRT, nullptr);
        else
            _hr = HW.pDevice->CreateOffscreenPlainSurface(w, h, f, D3DPOOL_SYSTEMMEM, &pRT, nullptr);
        break;
    }
    default: NODEFAULT;
    }
    if (FAILED(_hr))
        return;

    // OK
    HW.stats_manager.increment_stats_rtarget(pSurface);
#ifdef DEBUG
    Msg("* created RT(%s), %dx%d", Name, w, h);
#endif
    if (!pSurface)
        return; // special case (when type == D3DRTYPE_SURFACE)

    R_CHK(pSurface->GetSurfaceLevel(0, &pRT));
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

    HW.stats_manager.decrement_stats_rtarget(pSurface);
    _RELEASE(pSurface);
}

void CRT::reset_begin() { destroy(); }
void CRT::reset_end() { create(*cName, dwWidth, dwHeight, fmt, sampleCount, { dwFlags }); }

void CRT::resolve_into(CRT& destination) const
{
    const RECT rect    { 0, 0, (int)dwWidth, (int)dwHeight };
    const RECT dstRect { 0, 0, (int)destination.dwWidth, (int)destination.dwHeight };

    HW.pDevice->StretchRect(pRT, &rect, destination.pRT, &dstRect, D3DTEXF_POINT);
}

void resptrcode_crt::create(LPCSTR Name, u32 w, u32 h, D3DFORMAT f, u32 SampleCount /*= 1*/, Flags32 flags /*= {}*/)
{
    _set(RImplementation.Resources->_CreateRT(Name, w, h, f, SampleCount, flags));
}

//////////////////////////////////////////////////////////////////////////
//	DX10 cut
/*
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
    _order		= CPU::GetCLK();	//RDEVICE.GetTimerGlobal()->GetElapsed_clk();

    HRESULT		_hr;

    dwSize		= size;
    fmt			= f;

    // Get caps
    D3DCAPS9	caps;
    R_CHK		(HW.pDevice->GetDeviceCaps(&caps));

    // Check width-and-height of render target surface
    if (size>caps.MaxTextureWidth)		return;
    if (size>caps.MaxTextureHeight)		return;

    // Validate render-target usage
    _hr = HW.pD3D->CheckDeviceFormat(
        HW.DevAdapter,
        HW.m_DriverType,
        HW.Caps.fTarget,
        D3DUSAGE_RENDERTARGET,
        D3DRTYPE_CUBETEXTURE,
        f
        );
    if (FAILED(_hr))					return;

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
