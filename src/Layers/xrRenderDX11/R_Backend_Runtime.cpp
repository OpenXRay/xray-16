#include "stdafx.h"
#pragma hdrstop

#include "Layers/xrRender/LightTrack.h"
#include "xrEngine/IRenderable.h"

#if defined(USE_DX11)
#include <DirectXMath.h>
#endif


namespace xray::render::fg
{
void CBackend::OnFrameEnd()
{
    if (GEnv.Render->IsEnabled())
        return;

    if (!GEnv.isDedicatedServer)
    {
        Invalidate();
    }
}

void CBackend::OnFrameBegin()
{
    if (GEnv.Render->IsEnabled())
        return;

    if (!GEnv.isDedicatedServer)
    {
        PGO(Msg("PGO:*****frame[%d]*****", Device.dwFrame));

#ifndef USE_DX9
        Invalidate();
        // FrameGraph handles render target setup
#endif

        ZeroMemory(&stat, sizeof(stat));
        set_Stencil(FALSE);
    }
}

void CBackend::Invalidate()
{
    pRT[0] = 0;
    pRT[1] = 0;
    pRT[2] = 0;
    pRT[3] = 0;
    pZB = 0;
#if defined(USE_OGL)
    pFB = 0;
    pp = 0;
#endif

    decl = nullptr;
    vb = nullptr;
    ib = nullptr;
    vb_stride = 0;

    state = nullptr;
    ps = 0;
    vs = 0;
    DX11_ONLY(gs = NULL);
#ifdef USE_DX11
    hs = 0;
    ds = 0;
    cs = 0;
#endif
    ctable = nullptr;

    T = nullptr;
    M = nullptr;
    C = nullptr;

    stencil_enable = u32(-1);
    stencil_func = u32(-1);
    stencil_ref = u32(-1);
    stencil_mask = u32(-1);
    stencil_writemask = u32(-1);
    stencil_fail = u32(-1);
    stencil_pass = u32(-1);
    stencil_zfail = u32(-1);
    cull_mode = u32(-1);
    fill_mode = u32(-1);
    z_enable = u32(-1);
    z_func = u32(-1);
    alpha_ref = u32(-1);
    colorwrite_mask = u32(-1);

    // Since constant buffers are unmapped (for DirecX 10)
    // transform setting handlers should be unmapped too.
    xforms.unmap();

#if defined(USE_DX11)
    m_pInputLayout = NULL;
    m_PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    m_bChangedRTorZB = false;
    m_pInputSignature = NULL;
    for (int i = 0; i < MaxCBuffers; ++i)
    {
        m_aPixelConstants[i] = 0;
        m_aVertexConstants[i] = 0;
        m_aGeometryConstants[i] = 0;
        m_aHullConstants[i] = 0;
        m_aDomainConstants[i] = 0;
        m_aComputeConstants[i] = 0;
    }
    StateManager.Reset();
    // Redundant call. Just no note that we need to unmap const
    // if we create dedicated class.
    StateManager.UnmapConstants();
    SRVSManager.ResetDeviceState();

    for (u32 gs_it = 0; gs_it < CTexture::mtMaxGeometryShaderTextures;)
        textures_gs[gs_it++] = 0;
    for (u32 hs_it = 0; hs_it < CTexture::mtMaxHullShaderTextures;)
        textures_hs[hs_it++] = 0;
    for (u32 ds_it = 0; ds_it < CTexture::mtMaxDomainShaderTextures;)
        textures_ds[ds_it++] = 0;
    for (u32 cs_it = 0; cs_it < CTexture::mtMaxComputeShaderTextures;)
        textures_cs[cs_it++] = 0;

    context_id = CHW::IMM_CTX_ID;
#endif // USE_DX11

    for (u32 ps_it = 0; ps_it < CTexture::mtMaxPixelShaderTextures;)
        textures_ps[ps_it++] = nullptr;
    for (u32 vs_it = 0; vs_it < CTexture::mtMaxVertexShaderTextures;)
        textures_vs[vs_it++] = nullptr;
    for (auto& matrix : matrices)
        matrix = nullptr;
}

void CBackend::set_ClipPlanes(u32 _enable, Fplane* _planes /*=NULL */, u32 count /* =0*/)
{
#if defined(USE_DX11) || defined(USE_OGL)
    // TODO: DX11: Implement in the corresponding vertex shaders
    // Use this to set up location, were shader setup code will get data
    // VERIFY(!"CBackend::set_ClipPlanes not implemented!");
    UNUSED(_enable);
    UNUSED(_planes);
    UNUSED(count);
    return;
#else
#   error No graphics API selected or enabled!
#endif
}

#ifndef DEDICATED_SREVER
void CBackend::set_ClipPlanes(u32 _enable, Fmatrix* _xform /*=NULL */, u32 fmask /* =0xff */)
{
    if (0 == GEnv.Backend->GetCapabilities().geometry.dwClipPlanes)
        return;
    if (!_enable)
    {
#if defined(USE_DX11) || defined(USE_OGL)
    // TODO: DX11: Implement in the corresponding vertex shaders
    // Use this to set up location, were shader setup code will get data
    // VERIFY(!"CBackend::set_ClipPlanes not implemented!");
#else
#   error No graphics API selected or enabled!
#endif
        return;
    }
    VERIFY(_xform && fmask);
    CFrustum F;
    F.CreateFromMatrix(*_xform, fmask);
    set_ClipPlanes(_enable, F.planes, F.p_count);
}

void CBackend::set_Textures(STextureList* textures_list)
{
    T = textures_list;
}

#else

void CBackend::set_ClipPlanes(u32 _enable, Fmatrix* _xform /*=NULL */, u32 fmask /* =0xff */) {}
void CBackend::set_Textures(STextureList* textures_list) {}

#endif // DEDICATED SERVER

void CBackend::SetupStates()
{
    set_CullMode(CULL_CCW);
#if defined(USE_DX11)
    SSManager.SetMaxAnisotropy(ps_r__tf_Anisotropic);
    SSManager.SetMipLODBias(ps_r__tf_Mipbias);
#elif defined(USE_OGL)
    // TODO: OGL: Implement SetupStates().
#else
#   error No graphics API selected or enabled!
#endif
}


// Device dependance
void CBackend::OnDeviceCreate()
{
    ZoneScoped;

//#if defined(USE_DX11)
    //HW.get_context(context_id)->QueryInterface(__uuidof(ID3DUserDefinedAnnotation), reinterpret_cast<void**>(&pAnnotation));
//#endif

    // Debug Draw
    //InitializeDebugDraw();

    // invalidate caching
    Invalidate();
}

void CBackend::OnDeviceDestroy()
{
#if defined(USE_DX11)
    //  Destroy state managers
    StateManager.Reset();
#endif

#if defined(USE_DX11)
    _RELEASE(pAnnotation);
#endif
}

} // namespace xray::render::fg
