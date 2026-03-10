#include "stdafx.h"

#include "D3DXRenderBase.h"
#include "D3DUtils.h"
#include "dxUIRender.h"
#include "xrEngine/GameFont.h"
#include "xrEngine/PerformanceAlert.hpp"

#include <SDL.h>
#include "Layers/xrRender/Backend/D3D12Backend.h"
// Factory function - avoids pulling vulkan/vulkan.h into this TU
IRenderBackend* CreateVulkanBackend(SDL_Window* window, u32 width, u32 height, bool enableValidation);
#include "Layers/xrRender/PBRConverter/PBRTextureConverter.h"  // Phase 2.5.3
#include "xrEngine/IFrameGraphRender.h"

#if defined(XR_PLATFORM_WINDOWS) || defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_APPLE)
#   ifndef MASTER_GOLD
#       define USE_RENDERDOC
#   endif
#endif

#ifdef USE_RENDERDOC
#include <renderdoc/renderdoc_app.h>
#endif

extern ENGINE_API int ps_r4_use_pbr;

namespace xray::render::RENDER_NAMESPACE
{
#ifdef USE_RENDERDOC
RENDERDOC_API_1_0_0* g_renderdoc_api;
#endif

void D3DXRenderBase::setGamma(float fGamma)
{
    m_Gamma.Gamma(fGamma);
}

void D3DXRenderBase::setBrightness(float fGamma)
{
    m_Gamma.Brightness(fGamma);
}

void D3DXRenderBase::setContrast(float fGamma)
{
    m_Gamma.Contrast(fGamma);
}

void D3DXRenderBase::updateGamma()
{
    m_Gamma.Update();
}

void D3DXRenderBase::OnDeviceDestroy(bool bKeepTextures)
{
    if (!GEnv.isDedicatedServer)
    {
        UIRenderImpl.DestroyUIGeom();
        DUImpl.OnDeviceDestroy();
        m_PortalFadeGeom.destroy();
        m_PortalFadeShader.destroy();
        m_SelectionShader.destroy();
        m_WireShader.destroy();
    }
    destroy();

    Resources->OnDeviceDestroy(bKeepTextures);
#if RENDER == R_R4
    for (int id = 0; id < R__NUM_CONTEXTS; ++id)
    {
        contexts_pool[id].cmd_list.OnDeviceDestroy();
    }
#else
    RCache.OnDeviceDestroy();
#endif

    // Quad
    QuadIB.Release();

    // streams
    Index.Destroy();
    Vertex.Destroy();
}

void D3DXRenderBase::Destroy()
{
    xr_delete(Resources);

    // Shutdown the graphics backend
    if (GEnv.Backend)
    {
        GEnv.Backend->Shutdown();
        auto& render = static_cast<xray::render::RENDER_NAMESPACE::CRender&>(*this);
        xr_delete(render.m_backend);
        GEnv.Backend = nullptr;
    }
}

void D3DXRenderBase::Reset(SDL_Window* hWnd, u32& dwWidth, u32& dwHeight, float& fWidth_2, float& fHeight_2)
{
    ZoneScoped;

    reset_begin();
    Memory.mem_compact();

    // Resize the swap chain via backend
    if (GEnv.Backend)
    {
        // Get new window size
        int w, h;
        SDL_GetWindowSize(hWnd, &w, &h);
        GEnv.Backend->ResizeSwapChain(static_cast<u32>(w), static_cast<u32>(h));
        std::tie(dwWidth, dwHeight) = GEnv.Backend->GetBackBufferSize();
    }

    fWidth_2 = float(dwWidth / 2);
    fHeight_2 = float(dwHeight / 2);

    Resources->reset_end();

    // create everything, renderer may use
    reset_end();

#ifndef MASTER_GOLD
    Resources->Dump(true);
#endif
}

void D3DXRenderBase::ObtainRequiredWindowFlags(u32& windowFlags)
{
    windowFlags |= SDL_WINDOW_SHOWN;
}

void D3DXRenderBase::SetupStates()
{
    GEnv.Backend->UpdateCapabilities();
#if RENDER == R_R4
    for (int id = 0; id < R__NUM_CONTEXTS; ++id)
    {
        contexts_pool[id].cmd_list.SetupStates();
    }
#else
    RCache.SetupStates();
#endif
}

void D3DXRenderBase::OnDeviceCreate(const char* shName)
{
    ZoneScoped;

    // Signal everyone - device created

    // streams
    Vertex.Create();
    Index.Create();

    CreateQuadIB();

#if RENDER == R_R4
    for (int id = 0; id < R__NUM_CONTEXTS; ++id)
    {
        contexts_pool[id].cmd_list.context_id = id;
        contexts_pool[id].cmd_list.OnDeviceCreate();
    }
#else
    RCache.OnDeviceCreate();
#endif
    m_Gamma.Update();
    Resources->OnDeviceCreate(shName);
    Resources->CompatibilityCheck();
    create();
    if (!GEnv.isDedicatedServer)
    {
        // For D3D12/FrameGraph: Skip legacy shader creation - FrameGraph has its own rendering
        const bool useFrameGraph = GEnv.Backend && GEnv.Backend->IsFrameGraph();
        if (!useFrameGraph)
        {
            m_WireShader.create("editor" DELIMITER "wire");
            m_SelectionShader.create("editor" DELIMITER "selection");
            m_PortalFadeShader.create("portal");
            m_PortalFadeGeom.create(FVF::F_L, RImplementation.Vertex.Buffer(), 0);
            DUImpl.OnDeviceCreate();  // Creates fonts/debug utils via legacy blenders
        }
        UIRenderImpl.CreateUIGeom();
    }
}

void D3DXRenderBase::Create(SDL_Window* hWnd, u32& dwWidth, u32& dwHeight, float& fWidth_2, float& fHeight_2)
{
    ZoneScoped;

#ifdef USE_RENDERDOC
    if (!g_renderdoc_api)
    {
        HMODULE hModule = GetModuleHandleA("renderdoc.dll");
        if (hModule == 0)
        {
            hModule = LoadLibraryA("renderdoc.dll");
        }

        if (hModule)
        {
            auto const RENDERDOC_GetAPI =
                reinterpret_cast<pRENDERDOC_GetAPI>(GetProcAddress(hModule, "RENDERDOC_GetAPI"));
            auto const Result =
                RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_0_0, reinterpret_cast<void**>(&g_renderdoc_api));
            if (Result == 1)
            {
                g_renderdoc_api->UnloadCrashHandler();

                string_path FolderName;
                FS.update_path(FolderName, "$app_data_root$", "captures\\openxray");
                g_renderdoc_api->SetCaptureFilePathTemplate(FolderName);

                RENDERDOC_InputButton CaptureButton[] = {eRENDERDOC_Key_PrtScrn};
                g_renderdoc_api->SetCaptureKeys(CaptureButton, ARRAYSIZE(CaptureButton));

                g_renderdoc_api->SetCaptureOptionU32(eRENDERDOC_Option_AllowVSync, 0);
                g_renderdoc_api->SetCaptureOptionU32(eRENDERDOC_Option_DebugOutputMute, 0);

                g_renderdoc_api->SetCaptureOptionU32(eRENDERDOC_Option_RefAllResources, 1);
                g_renderdoc_api->SetCaptureOptionU32(eRENDERDOC_Option_CaptureCallstacks, 1);
                g_renderdoc_api->SetCaptureOptionU32(eRENDERDOC_Option_VerifyBufferAccess, 1);
                g_renderdoc_api->SetCaptureOptionU32(eRENDERDOC_Option_APIValidation, 1);
                g_renderdoc_api->SetCaptureOptionU32(eRENDERDOC_Option_CaptureAllCmdLists, 1);
            }
        }
    }
#endif

    // Get initial window size
    int w, h;
    SDL_GetWindowSize(hWnd, &w, &h);
    dwWidth = static_cast<u32>(w);
    dwHeight = static_cast<u32>(h);

    auto& render = static_cast<xray::render::RENDER_NAMESPACE::CRender&>(*this);
    const bool enableValidation = true;

    if (ps_fg_render_mode == FG_RENDER_VULKAN)
    {
        auto* vkBackend = CreateVulkanBackend(hWnd, dwWidth, dwHeight, enableValidation);
        if (vkBackend)
        {
            render.m_backend = vkBackend;
            GEnv.Backend = vkBackend;
            Msg("* [CRender] Vulkan backend initialized successfully");
            Msg("*   Bindless textures: %s (max %u)",
                vkBackend->GetCapabilities().bindlessTextures ? "Yes" : "No",
                vkBackend->GetCapabilities().maxBindlessResources);
        }
        else
        {
            FATAL("Vulkan initialization failed");
        }
    }
    else
    {
        auto* dx12Backend = xr_new<D3D12Backend>();
        if (dx12Backend->Initialize(hWnd, dwWidth, dwHeight, enableValidation))
        {
            render.m_backend = dx12Backend;
            GEnv.Backend = dx12Backend;
            Msg("* [CRender] D3D12 backend initialized successfully");
            Msg("*   Bindless textures: %s (max %u)",
                dx12Backend->GetCapabilities().bindlessTextures ? "Yes" : "No",
                dx12Backend->GetCapabilities().maxBindlessResources);
        }
        else
        {
            Msg("! [CRender] D3D12 backend initialization failed");
            xr_delete(dx12Backend);
            FATAL("D3D12 initialization failed - no fallback available");
        }
    }

    std::tie(dwWidth, dwHeight) = GEnv.Backend->GetBackBufferSize();

    fWidth_2 = float(dwWidth / 2);
    fHeight_2 = float(dwHeight / 2);
    Resources = xr_new<CResourceManager>();
}

void D3DXRenderBase::overdrawBegin()
{
    RCache.dbg_OverdrawBegin();
}

void D3DXRenderBase::overdrawEnd()
{
    RCache.dbg_OverdrawEnd();
}

void D3DXRenderBase::DeferredLoad(bool E)
{
    Resources->DeferredLoad(E);
}
void D3DXRenderBase::ResourcesDeferredUpload()
{
    Resources->DeferredUpload();
}
void D3DXRenderBase::ResourcesDeferredUnload()
{
    Resources->DeferredUnload();
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
    return GEnv.Backend ? GEnv.Backend->GetDeviceState() : DeviceState::Lost;
}

bool D3DXRenderBase::GetForceGPU_REF()
{
    return GEnv.Backend->GetCapabilities().bForceGPU_REF;
}
u32 D3DXRenderBase::GetCacheStatPolys()
{
    return RCache.stat.render.polys;
}
void D3DXRenderBase::Begin()
{
    // Begin frame on backend
    GEnv.Backend->BeginFrame();

    if (GEnv.FrameGraphRenderer->IsEnabled())
        return;

#if RENDER == R_R4
    for (int id = 0; id < R__NUM_CONTEXTS; ++id)
    {
        contexts_pool[id].cmd_list.OnFrameBegin();
        contexts_pool[id].cmd_list.set_CullMode(CULL_CW);
        contexts_pool[id].cmd_list.set_CullMode(CULL_CCW);
    }
#else
    RCache.OnFrameBegin();
    RCache.set_CullMode(CULL_CW);
    RCache.set_CullMode(CULL_CCW);
#endif
    Vertex.Flush();
    Index.Flush();
    if (GEnv.Backend->GetCapabilities().SceneMode)
        overdrawBegin();
}

void D3DXRenderBase::Clear()
{
    RCache.ClearZB(RCache.get_ZB(), 1.0f, 0);
    if (psDeviceFlags.test(rsClearBB))
    {
        RCache.ClearRT(RCache.get_RT(), {}); // black
    }
}

void D3DXRenderBase::End()
{
    ZoneScopedN("D3DXRenderBase::BackendEnd");

    if (GEnv.FrameGraphRenderer->IsEnabled())
    {
        // End frame and present via backend
        GEnv.Backend->EndFrame();
        GEnv.Backend->Present(psDeviceFlags.test(rsVSync));
        return;
    }

    if (GEnv.Backend->GetCapabilities().SceneMode)
        overdrawEnd();

#if RENDER == R_R4
    for (int id = 0; id < R__NUM_CONTEXTS; ++id)
    {
        contexts_pool[id].cmd_list.OnFrameEnd();
    }
#else
    RCache.OnFrameEnd();
#endif

    // we're done with rendering
    cleanup_contexts();

    // End frame and present via backend
    GEnv.Backend->EndFrame();
    GEnv.Backend->Present(psDeviceFlags.test(rsVSync));
}

void D3DXRenderBase::ResourcesDestroyNecessaryTextures()
{
    Resources->DestroyNecessaryTextures();
}
void D3DXRenderBase::ClearTarget()
{
    RCache.ClearRT(RCache.get_RT(), {}); // black
}

void D3DXRenderBase::SetCacheXform(Fmatrix& mView, Fmatrix& mProject)
{
#if RENDER == R_R4
    for (int id = 0; id < R__NUM_CONTEXTS; ++id)
    {
        contexts_pool[id].cmd_list.set_xform_view(mView);
        contexts_pool[id].cmd_list.set_xform_project(mProject);
    }
#else
    RCache.set_xform_view(mView);
    RCache.set_xform_project(mProject);
#endif
}

bool D3DXRenderBase::HWSupportsShaderYUV2RGB()
{
    // DX12/FrameGraph: GPU-side YUV→RGB conversion not yet implemented
    // Force CPU-side conversion until we implement YUV shader support
    if (GEnv.Backend && GEnv.Backend->IsFrameGraph())
        return false;

    // D3D11: Use hardware capabilities check
    const auto& caps = GEnv.Backend->GetCapabilities();
    u32 v_dev = CAP_VERSION(caps.raster_major, caps.raster_minor);
    u32 v_need = CAP_VERSION(2, 0);
    return v_dev >= v_need;
}

void D3DXRenderBase::OnAssetsChanged()
{
    ZoneScoped;
    Resources->m_textures_description.UnLoad();
    Resources->m_textures_description.Load();
}

xrImTextureData D3DXRenderBase::GetImGuiTextureId(pcstr texture_name)
{
    const auto texture = Resources->_CreateTexture(texture_name);
    return
    {
        texture->GetImTextureID(),
        {
            (float)texture->get_Width(),
            (float)texture->get_Height()
        }
    };
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
    font.OutNext("Wait-L:       %2.2fms, %2.1f%%", BasicStats.Wait.result, PPP(BasicStats.Wait.result));
    font.OutNext("Wait-S:       %2.2fms, %2.1f%%", BasicStats.WaitS.result, PPP(BasicStats.WaitS.result));
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
    font.OutNext("Vertices:     %d/%d", rcstats.render.verts, rcstats.render.calls ? rcstats.render.verts / rcstats.render.calls : 0);
    font.OutNext("Polygons:     %d/%d", rcstats.render.polys, rcstats.render.calls ? rcstats.render.polys / rcstats.render.calls : 0);
    font.OutNext("DIP/DP:       %d", rcstats.render.calls);
    font.OutNext("Compute:      %d", rcstats.compute.calls);
    font.OutNext("- Groups:     %d/%d/%d", rcstats.compute.groups_x, rcstats.compute.groups_y, rcstats.compute.groups_z);
    font.OutNext("S/T/M/C:      %d/%d/%d/%d", rcstats.states, rcstats.textures, rcstats.matrices, rcstats.constants);
    font.OutNext("RT/ZB/PP:     %d/%d/%d", rcstats.target_rt, rcstats.target_zb, rcstats.pp);
    font.OutNext("PS/VS/GS:     %d/%d/%d", rcstats.ps, rcstats.vs, rcstats.gs);
    font.OutNext("HS/DS/CS:     %d/%d/%d", rcstats.hs, rcstats.ds, rcstats.cs);
    font.OutNext("DECL/VB/IB:   %d/%d/%d", rcstats.decl, rcstats.vb, rcstats.ib);
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
        if (rcstats.render.verts > 500000)
            alert->Print(font, "Verts     > 500k: %d", rcstats.render.verts);
        if (rcstats.render.calls > 1000)
            alert->Print(font, "DIP/DP    > 1k:   %d", rcstats.render.calls);
        if (BasicStats.DetailCount > 1000)
            alert->Print(font, "DT_count  > 1000: %u", BasicStats.DetailCount);
    }
    BasicStats.FrameStart();
}

// Phase 2.5.3: PBR texture conversion
using namespace xray::render::pbr;
void D3DXRenderBase::ConvertLegacyAssetsToPBR()
{
    if (ps_r4_use_pbr == 0)
        return; // PBR disabled

    Msg("~ [PBR] PBR rendering enabled, checking texture conversion...");

    // Configure texture scanning
    TextureScanConfig scanConfig;
    scanConfig.texture_roots = {"$game_textures$"};
    scanConfig.recursive = true;
    scanConfig.include_levels = true;

    // Build texture inventory
    TextureInventory inventory = BuildTextureInventory(scanConfig);
    Msg("~ [PBR] Found %d legacy texture sets", static_cast<int>(inventory.assets.size()));

    // Check if conversion is needed (missing output files)
    PBRConversionParams params;
    params.generate_mipmaps = true;
    params.default_metallic = 0.0f;  // Non-metallic by default
    params.default_roughness = 0.5f; // Mid-range roughness by default
    params.default_ao = 1.0f;        // No occlusion by default

    bool needsConversion = !VerifyPBROutputs(inventory, params);

    if (needsConversion)
    {
        Msg("~ [PBR] Starting texture conversion (outputs missing)...");

        PBRConversionStats stats;
        bool success = ConvertTexturesToPBR(inventory, params, stats, nullptr);

        if (success)
        {
            Msg("~ [PBR] Conversion complete: %d converted, %d skipped, %d failed",
                stats.textures_converted, stats.textures_skipped, stats.textures_failed);
        }
        else
        {
            Msg("! [PBR] Conversion failed!");
        }
    }
    else
    {
        Msg("~ [PBR] Textures already converted, skipping conversion.");
    }

    // Consolidate separate PBR textures into packed _pbr.dds files
    // This packs _metallic, _roughness, _ao, _parallax into single RGBA texture
    Msg("~ [PBR] Checking for PBR texture consolidation...");
    ConsolidationStats consolidationStats;
    if (ConsolidatePBRTextures("$game_textures$", consolidationStats, nullptr))
    {
        if (consolidationStats.textures_consolidated > 0)
        {
            Msg("~ [PBR] Consolidation complete: %d packed, %d files deleted",
                consolidationStats.textures_consolidated, consolidationStats.files_deleted);
        }
        else
        {
            Msg("~ [PBR] No textures needed consolidation.");
        }
    }
    else
    {
        Msg("! [PBR] Consolidation had failures: %d failed", consolidationStats.textures_failed);
    }
}

} // namespace xray::render::RENDER_NAMESPACE
