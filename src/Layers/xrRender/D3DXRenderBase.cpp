#include "stdafx.h"

#include "D3DXRenderBase.h"
#include "xrEngine/GameFont.h"
#include "xrEngine/PerformanceAlert.hpp"

D3DXRenderBase::D3DXRenderBase()
{
    val_pObject = nullptr;
    val_pTransform = nullptr;
    val_bHUD = FALSE;
    val_bInvisible = FALSE;
    val_bRecordMP = FALSE;
    val_feedback = nullptr;
    val_feedback_breakp = 0;
    val_recorder = nullptr;
    marker = 0;
    r_pmask(true, true);
    b_loaded = FALSE;
    Resources = nullptr;
}

void D3DXRenderBase::Copy(IRender& _in)
{
    *this = *(D3DXRenderBase*)&_in;
}
void D3DXRenderBase::setGamma(float fGamma)
{
#ifndef USE_OGL
    m_Gamma.Gamma(fGamma);
#else
    UNUSED(fGamma);
#endif
}

void D3DXRenderBase::setBrightness(float fGamma)
{
#ifndef USE_OGL
    m_Gamma.Brightness(fGamma);
#else
    UNUSED(fGamma);
#endif
}

void D3DXRenderBase::setContrast(float fGamma)
{
#ifndef USE_OGL
    m_Gamma.Contrast(fGamma);
#else
    UNUSED(fGamma);
#endif
}

void D3DXRenderBase::updateGamma()
{
#ifndef USE_OGL
    m_Gamma.Update();
#endif
}

void D3DXRenderBase::OnDeviceDestroy(bool bKeepTextures)
{
    m_WireShader.destroy();
    m_SelectionShader.destroy();
    Resources->OnDeviceDestroy(bKeepTextures);
    RCache.OnDeviceDestroy();
}

void D3DXRenderBase::ValidateHW()
{
    HW.Validate();
}
void D3DXRenderBase::DestroyHW()
{
    xr_delete(Resources);
    HW.DestroyDevice();
}

void D3DXRenderBase::Reset(SDL_Window* hWnd, u32& dwWidth, u32& dwHeight, float& fWidth_2, float& fHeight_2)
{
#if defined(DEBUG) && !defined(USE_OGL)
    _SHOW_REF("*ref -CRenderDevice::ResetTotal: DeviceREF:", HW.pDevice);
#endif // DEBUG

    Resources->reset_begin();
    Memory.mem_compact();
    HW.Reset();

#if defined(USE_OGL)
    dwWidth = psCurrentVidMode[0];
    dwHeight = psCurrentVidMode[1];
#elif defined(USE_DX10) || defined(USE_DX11)
    dwWidth = HW.m_ChainDesc.BufferDesc.Width;
    dwHeight = HW.m_ChainDesc.BufferDesc.Height;
#else //    USE_DX10
    dwWidth = HW.DevPP.BackBufferWidth;
    dwHeight = HW.DevPP.BackBufferHeight;
#endif //   USE_DX10

    fWidth_2 = float(dwWidth / 2);
    fHeight_2 = float(dwHeight / 2);
    Resources->reset_end();

#if defined(DEBUG) && !defined(USE_OGL)
    _SHOW_REF("*ref +CRenderDevice::ResetTotal: DeviceREF:", HW.pDevice);
#endif
}

void D3DXRenderBase::SetupStates()
{
    HW.Caps.Update();
#if defined(USE_OGL)
    // TODO: OGL: Implement SetupStates().
#elif defined(USE_DX10) || defined(USE_DX11)
    SSManager.SetMaxAnisotropy(ps_r__tf_Anisotropic);
    SSManager.SetMipLODBias(ps_r__tf_Mipbias);
#else //    USE_DX10
    for (u32 i = 0; i < HW.Caps.raster.dwStages; i++)
    {
        CHK_DX(HW.pDevice->SetSamplerState(i, D3DSAMP_MAXANISOTROPY, ps_r__tf_Anisotropic));
        CHK_DX(HW.pDevice->SetSamplerState(i, D3DSAMP_MIPMAPLODBIAS, *(LPDWORD)&ps_r__tf_Mipbias));
        CHK_DX(HW.pDevice->SetSamplerState(i, D3DSAMP_MINFILTER, D3DTEXF_LINEAR));
        CHK_DX(HW.pDevice->SetSamplerState(i, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR));
        CHK_DX(HW.pDevice->SetSamplerState(i, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR));
    }
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_DITHERENABLE, TRUE));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_COLORVERTEX, TRUE));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_ZENABLE, TRUE));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_LOCALVIEWER, TRUE));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_SPECULARMATERIALSOURCE, D3DMCS_MATERIAL));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_MATERIAL));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_COLOR1));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, FALSE));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_NORMALIZENORMALS, TRUE));
    if (psDeviceFlags.test(rsWireframe))
        CHK_DX(HW.pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME));
    else
        CHK_DX(HW.pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID));
    // ******************** Fog parameters
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_FOGCOLOR, 0));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_RANGEFOGENABLE, FALSE));
    if (HW.Caps.bTableFog)
    {
        CHK_DX(HW.pDevice->SetRenderState(D3DRS_FOGTABLEMODE, D3DFOG_LINEAR));
        CHK_DX(HW.pDevice->SetRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_NONE));
    }
    else
    {
        CHK_DX(HW.pDevice->SetRenderState(D3DRS_FOGTABLEMODE, D3DFOG_NONE));
        CHK_DX(HW.pDevice->SetRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR));
    }
#endif // USE_DX10
}

void D3DXRenderBase::OnDeviceCreate(const char* shName)
{
    // Signal everyone - device created
    RCache.OnDeviceCreate();
#ifndef USE_OGL
    m_Gamma.Update();
#endif
    Resources->OnDeviceCreate(shName);
    create();
    if (!GEnv.isDedicatedServer)
    {
        m_WireShader.create("editor" DELIMITER "wire");
        m_SelectionShader.create("editor" DELIMITER "selection");
        DUImpl.OnDeviceCreate();
    }
}

void D3DXRenderBase::Create(SDL_Window* hWnd, u32& dwWidth, u32& dwHeight, float& fWidth_2, float& fHeight_2)
{
    HW.CreateDevice(hWnd);
#if defined(USE_OGL)
    dwWidth = psCurrentVidMode[0];
    dwHeight = psCurrentVidMode[1];
#elif defined(USE_DX10) || defined(USE_DX11)
    dwWidth = HW.m_ChainDesc.BufferDesc.Width;
    dwHeight = HW.m_ChainDesc.BufferDesc.Height;
#else
    dwWidth = HW.DevPP.BackBufferWidth;
    dwHeight = HW.DevPP.BackBufferHeight;
#endif
    fWidth_2 = float(dwWidth / 2);
    fHeight_2 = float(dwHeight / 2);
    Resources = new CResourceManager();
}

void D3DXRenderBase::SetupGPU(bool bForceGPU_SW, bool bForceGPU_NonPure, bool bForceGPU_REF)
{
    HW.Caps.bForceGPU_SW = bForceGPU_SW;
    HW.Caps.bForceGPU_NonPure = bForceGPU_NonPure;
    HW.Caps.bForceGPU_REF = bForceGPU_REF;
}

void D3DXRenderBase::overdrawBegin()
{
#ifndef USE_DX9
    //  TODO: DX10: Implement overdrawBegin
    VERIFY(!"D3DXRenderBase::overdrawBegin not implemented.");
#else
    // Turn stenciling
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILENABLE, TRUE));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILREF, 0));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILMASK, 0x00000000));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILWRITEMASK, 0xffffffff));
    // Increment the stencil buffer for each pixel drawn
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_INCRSAT));
    if (1 == HW.Caps.SceneMode)
        CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP)); // Overdraw
    else
        CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_INCRSAT)); // ZB access
#endif
}

void D3DXRenderBase::overdrawEnd()
{
#ifndef USE_DX9
    // TODO: DX10: Implement overdrawEnd
    VERIFY(!"D3DXRenderBase::overdrawEnd not implemented.");
#else
    // Set up the stencil states
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL));
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILMASK, 0xff));
    // Set the background to black
    CHK_DX(HW.pDevice->Clear(0, nullptr, D3DCLEAR_TARGET, color_xrgb(255, 0, 0), 0, 0));
    // Draw a rectangle wherever the count equal I
    RCache.OnFrameEnd();
    CHK_DX(HW.pDevice->SetFVF(FVF::F_TL));
    // Render gradients
    for (int I = 0; I < 12; I++)
    {
        u32 _c = I * 256 / 13;
        u32 c = color_xrgb(_c, _c, _c);
        FVF::TL pv[4];
        pv[0].set(float(0), float(Device.dwHeight), c, 0, 0);
        pv[1].set(float(0), float(0), c, 0, 0);
        pv[2].set(float(Device.dwWidth), float(Device.dwHeight), c, 0, 0);
        pv[3].set(float(Device.dwWidth), float(0), c, 0, 0);
        CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILREF, I));
        CHK_DX(HW.pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, pv, sizeof(FVF::TL)));
    }
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILENABLE, FALSE));
#endif // USE_DX10
}

void D3DXRenderBase::DeferredLoad(bool E)
{
    Resources->DeferredLoad(E);
}
void D3DXRenderBase::ResourcesDeferredUpload()
{
    Resources->DeferredUpload();
}
void D3DXRenderBase::ResourcesGetMemoryUsage(u32& m_base, u32& c_base, u32& m_lmaps, u32& c_lmaps)
{
    if (Resources)
        Resources->_GetMemoryUsage(m_base, c_base, m_lmaps, c_lmaps);
}

void D3DXRenderBase::ResourcesStoreNecessaryTextures()
{
    Resources->StoreNecessaryTextures();
}
void D3DXRenderBase::ResourcesDumpMemoryUsage()
{
    Resources->_DumpMemoryUsage();
}
DeviceState D3DXRenderBase::GetDeviceState()
{
    HW.Validate();
#ifdef USE_OGL
    //  TODO: OGL: Implement GetDeviceState
#elif !defined(USE_DX9)
    const auto result = HW.m_pSwapChain->Present(0, DXGI_PRESENT_TEST);

    switch (result)
    {
        // Check if the device is ready to be reset
    case DXGI_ERROR_DEVICE_RESET:
        return DeviceState::NeedReset;
    }
#else
    const auto result = HW.pDevice->TestCooperativeLevel();

    switch (result)
    {
        // If the device was lost, do not render until we get it back
    case D3DERR_DEVICELOST:
        return DeviceState::Lost;

        // Check if the device is ready to be reset
    case D3DERR_DEVICENOTRESET:
        return DeviceState::NeedReset;
    }
#endif
    return DeviceState::Normal;
}

bool D3DXRenderBase::GetForceGPU_REF()
{
    return HW.Caps.bForceGPU_REF;
}
u32 D3DXRenderBase::GetCacheStatPolys()
{
    return RCache.stat.polys;
}
void D3DXRenderBase::Begin()
{
#ifdef USE_DX9
    CHK_DX(HW.pDevice->BeginScene());
#endif
    RCache.OnFrameBegin();
    RCache.set_CullMode(CULL_CW);
    RCache.set_CullMode(CULL_CCW);
    if (HW.Caps.SceneMode)
        overdrawBegin();
}

void D3DXRenderBase::Clear()
{
#ifndef USE_DX9
    HW.pContext->ClearDepthStencilView(RCache.get_ZB(), D3D_CLEAR_DEPTH | D3D_CLEAR_STENCIL, 1.0f, 0);
    if (psDeviceFlags.test(rsClearBB))
    {
        FLOAT ColorRGBA[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        HW.pContext->ClearRenderTargetView(RCache.get_RT(), ColorRGBA);
    }
#else
    CHK_DX(HW.pDevice->Clear(0, nullptr, D3DCLEAR_ZBUFFER | (psDeviceFlags.test(rsClearBB) ? D3DCLEAR_TARGET : 0) |
        (HW.Caps.bStencil ? D3DCLEAR_STENCIL : 0), color_xrgb(0, 0, 0), 1, 0));
#endif
}

void DoAsyncScreenshot();

void D3DXRenderBase::End()
{
    VERIFY(HW.pDevice);
    if (HW.Caps.SceneMode)
        overdrawEnd();
    RCache.OnFrameEnd();
    DoAsyncScreenshot();
#ifndef USE_DX9
    bool bUseVSync = psDeviceFlags.is(rsFullscreen) && psDeviceFlags.test(rsVSync); //xxx: weird tearing glitches when VSync turned on for windowed mode in DX10\11
    HW.m_pSwapChain->Present(bUseVSync ? 1 : 0, 0);
#else
    CHK_DX(HW.pDevice->EndScene());
    HW.pDevice->Present(nullptr, nullptr, nullptr, nullptr);
#endif
}

void D3DXRenderBase::ResourcesDestroyNecessaryTextures()
{
    Resources->DestroyNecessaryTextures();
}
void D3DXRenderBase::ClearTarget()
{
#ifndef USE_DX9
    FLOAT ColorRGBA[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    HW.pContext->ClearRenderTargetView(RCache.get_RT(), ColorRGBA);
#else
    CHK_DX(HW.pDevice->Clear(0, nullptr, D3DCLEAR_TARGET, color_xrgb(0, 0, 0), 1, 0));
#endif
}

void D3DXRenderBase::SetCacheXform(Fmatrix& mView, Fmatrix& mProject)
{
    RCache.set_xform_view(mView);
    RCache.set_xform_project(mProject);
}

bool D3DXRenderBase::HWSupportsShaderYUV2RGB()
{
    u32 v_dev = CAP_VERSION(HW.Caps.raster_major, HW.Caps.raster_minor);
    u32 v_need = CAP_VERSION(2, 0);
    return v_dev >= v_need;
}

void D3DXRenderBase::OnAssetsChanged()
{
    Resources->m_textures_description.UnLoad();
    Resources->m_textures_description.Load();
}

void D3DXRenderBase::DumpStatistics(IGameFont& font, IPerformanceAlert* alert)
{
    BasicStats.FrameEnd();
    auto renderTotal = Device.GetStats().RenderTotal.result;
#define PPP(a) (100.f * float(a) / renderTotal)
    font.OutNext("*** RENDER:   %2.2fms", renderTotal);
    font.OutNext("Calc:         %2.2fms, %2.1f%%", BasicStats.Culling.result, PPP(BasicStats.Culling.result));
    font.OutNext("Skeletons:    %2.2fms, %d", BasicStats.Animation.result, BasicStats.Animation.count);
    font.OutNext("Primitives:   %2.2fms, %2.1f%%", BasicStats.Primitives.result, PPP(BasicStats.Primitives.result));
    font.OutNext("Wait-L:       %2.2fms", BasicStats.Wait.result);
    font.OutNext("Wait-S:       %2.2fms", BasicStats.WaitS.result);
    font.OutNext("Skinning:     %2.2fms", BasicStats.Skinning.result);
    font.OutNext("DT_Vis/Cnt:   %2.2fms/%d", BasicStats.DetailVisibility.result, BasicStats.DetailCount);
    font.OutNext("DT_Render:    %2.2fms", BasicStats.DetailRender.result);
    font.OutNext("DT_Cache:     %2.2fms", BasicStats.DetailCache.result);
    font.OutNext("Wallmarks:    %2.2fms, %d/%d - %d", BasicStats.Wallmarks.result, BasicStats.StaticWMCount,
        BasicStats.DynamicWMCount, BasicStats.WMTriCount);
    font.OutNext("Glows:        %2.2fms", BasicStats.Glows.result);
    font.OutNext("Lights:       %2.2fms, %d", BasicStats.Lights.result, BasicStats.Lights.count);
    font.OutNext("RT:           %2.2fms, %d", BasicStats.RenderTargets.result, BasicStats.RenderTargets.count);
    font.OutNext("HUD:          %2.2fms", BasicStats.HUD.result);
    font.OutNext("P_calc:       %2.2fms", BasicStats.Projectors.result);
    font.OutNext("S_calc:       %2.2fms", BasicStats.ShadowsCalc.result);
    font.OutNext("S_render:     %2.2fms, %d", BasicStats.ShadowsRender.result, BasicStats.ShadowsRender.count);
    u32 occQs = BasicStats.OcclusionQueries ? BasicStats.OcclusionQueries : 1;
    font.OutNext("Occ-query:    %03.1f", 100.f * f32(BasicStats.OcclusionCulled) / occQs);
    font.OutNext("- queries:    %u", BasicStats.OcclusionQueries);
    font.OutNext("- culled:     %u", BasicStats.OcclusionCulled);
#undef PPP
    font.OutSkip();
    const auto& rcstats = RCache.stat;
    font.OutNext("Vertices:     %d/%d", rcstats.verts, rcstats.calls ? rcstats.verts / rcstats.calls : 0);
    font.OutNext("Polygons:     %d/%d", rcstats.polys, rcstats.calls ? rcstats.polys / rcstats.calls : 0);
    font.OutNext("DIP/DP:       %d", rcstats.calls);
#ifdef DEBUG
    font.OutNext("SH/T/M/C:     %d/%d/%d/%d", rcstats.states, rcstats.textures, rcstats.matrices, rcstats.constants);
    font.OutNext("RT/PS/VS:     %d/%d/%d", rcstats.target_rt, rcstats.ps, rcstats.vs);
    font.OutNext("DECL/VB/IB:   %d/%d/%d", rcstats.decl, rcstats.vb, rcstats.ib);
#endif
    font.OutNext("XForms:       %d", rcstats.xforms);
    font.OutNext("Static:       %3.1f/%d", rcstats.r.s_static.verts / 1024.f, rcstats.r.s_static.dips);
    font.OutNext("Flora:        %3.1f/%d", rcstats.r.s_flora.verts / 1024.f, rcstats.r.s_flora.dips);
    font.OutNext("- lods:       %3.1f/%d", rcstats.r.s_flora_lods.verts / 1024.f, rcstats.r.s_flora_lods.dips);
    font.OutNext("Dynamic:      %3.1f/%d", rcstats.r.s_dynamic.verts / 1024.f, rcstats.r.s_dynamic.dips);
    font.OutNext("- sw:         %3.1f/%d", rcstats.r.s_dynamic_sw.verts / 1024.f, rcstats.r.s_dynamic_sw.dips);
    font.OutNext("- inst:       %3.1f/%d", rcstats.r.s_dynamic_inst.verts / 1024.f, rcstats.r.s_dynamic_inst.dips);
    font.OutNext("- 1B:         %3.1f/%d", rcstats.r.s_dynamic_1B.verts / 1024.f, rcstats.r.s_dynamic_1B.dips);
    font.OutNext("- 2B:         %3.1f/%d", rcstats.r.s_dynamic_2B.verts / 1024.f, rcstats.r.s_dynamic_2B.dips);
    font.OutNext("- 3B:         %3.1f/%d", rcstats.r.s_dynamic_3B.verts / 1024.f, rcstats.r.s_dynamic_3B.dips);
    font.OutNext("- 4B:         %3.1f/%d", rcstats.r.s_dynamic_4B.verts / 1024.f, rcstats.r.s_dynamic_4B.dips);
    font.OutNext("Details:      %3.1f/%d", rcstats.r.s_details.verts / 1024.f, rcstats.r.s_details.dips);
    if (alert)
    {
        if (rcstats.verts > 500000)
            alert->Print(font, "Verts     > 500k: %d", rcstats.verts);
        if (rcstats.calls > 1000)
            alert->Print(font, "DIP/DP    > 1k:   %d", rcstats.calls);
        if (BasicStats.DetailCount > 1000)
            alert->Print(font, "DT_count  > 1000: %u", BasicStats.DetailCount);
    }
    BasicStats.FrameStart();
}
