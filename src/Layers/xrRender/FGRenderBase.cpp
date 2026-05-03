#include "stdafx.h"

#include "FGRenderBase.h"
#include "FGRenderHost.h"
#include "Layers/xrRender/ResourceManager.h"
#include "Layers/xrRender/PBRConverter/PBRTextureConverter.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/Bindless/MaterialBuffer.h"
#include "Layers/xrRender/Bindless/TerrainMaterialBuffer.h"
#include "Layers/xrRender/Bindless/VariantTextureBuffer.h"

#include "xrEngine/IRenderBackend.h"
#include "xrEngine/GameFont.h"
#include "xrEngine/PerformanceAlert.hpp"

#include <SDL3/SDL.h>

extern ENGINE_API int ps_r4_use_pbr;

namespace xray::render::fg
{
void FGRenderBase::setGamma(float fGamma)     { m_Gamma.Gamma(fGamma); }
void FGRenderBase::setBrightness(float fGamma){ m_Gamma.Brightness(fGamma); }
void FGRenderBase::setContrast(float fGamma)  { m_Gamma.Contrast(fGamma); }
void FGRenderBase::updateGamma()              { m_Gamma.Update(); }

void FGRenderBase::OnDeviceDestroy(bool bKeepTextures)
{
    destroy();
    if (Resources)
        Resources->OnDeviceDestroy(bKeepTextures);
}

void FGRenderBase::Destroy()
{
    xr_delete(Resources);

    bindless::DrawMaterialIDBuffer::Instance().Shutdown();
    bindless::VariantTextureBuffer::Instance().Shutdown();
    bindless::MaterialBuffer::Instance().Shutdown();
    bindless::TerrainMaterialBuffer::Instance().Shutdown();

    framegraph::GetPassResourceCache().Clear();

    if (GEnv.Backend)
    {
        GEnv.Backend->Shutdown();
        GEnv.Backend = nullptr;
    }
}

void FGRenderBase::Reset(SDL_Window* hWnd, u32& dwWidth, u32& dwHeight, float& fWidth_2, float& fHeight_2)
{
    ZoneScoped;

    const u32 oldWidth = dwWidth;
    const u32 oldHeight = dwHeight;

    reset_begin();
    OnBackBufferResizing(oldWidth, oldHeight);
    Memory.mem_compact();

    FGRenderHost::ResizeBackend(hWnd, dwWidth, dwHeight);

    fWidth_2 = float(dwWidth / 2);
    fHeight_2 = float(dwHeight / 2);

    if (Resources)
        Resources->reset_end();

    OnBackBufferResized(dwWidth, dwHeight);
    reset_end();

#ifndef MASTER_GOLD
    if (Resources)
        Resources->Dump(true);
#endif
}

void FGRenderBase::ObtainRequiredWindowFlags(u32& windowFlags)
{
    if (ps_fg_render_mode == FG_RENDER_VULKAN)
        windowFlags |= SDL_WINDOW_VULKAN;
}

void FGRenderBase::SetupStates()
{
    if (GEnv.Backend)
        GEnv.Backend->UpdateCapabilities();
}

void FGRenderBase::OnDeviceCreate(pcstr shName)
{
    ZoneScoped;

    m_Gamma.Update();

    if (Resources)
    {
        Resources->OnDeviceCreate(shName);
        Resources->CompatibilityCheck();
    }

    create();
}

void FGRenderBase::Create(SDL_Window* hWnd, u32& dwWidth, u32& dwHeight, float& fWidth_2, float& fHeight_2)
{
    ZoneScoped;

    const bool enableValidation = !!strstr(Core.Params, "-d3ddebug");

    auto* backend = FGRenderHost::CreateBackend(hWnd, dwWidth, dwHeight, enableValidation);
    UNUSED(backend);

    fWidth_2 = float(dwWidth / 2);
    fHeight_2 = float(dwHeight / 2);

    Resources = xr_new<CResourceManager>();
}

void FGRenderBase::DeferredLoad(bool E)
{
    if (Resources) Resources->DeferredLoad(E);
}

void FGRenderBase::ResourcesDeferredUpload()
{
    if (Resources) Resources->DeferredUpload();
}

void FGRenderBase::ResourcesDeferredUnload()
{
    if (Resources) Resources->DeferredUnload();
}

void FGRenderBase::ResourcesGetMemoryUsage(u32& m_base, u32& c_base, u32& m_lmaps, u32& c_lmaps)
{
    if (Resources) Resources->_GetMemoryUsage(m_base, c_base, m_lmaps, c_lmaps);
}

void FGRenderBase::ResourcesStoreNecessaryTextures()
{
    if (Resources) Resources->StoreNecessaryTextures();
}

void FGRenderBase::ResourcesDestroyNecessaryTextures()
{
    if (Resources) Resources->DestroyNecessaryTextures();
}

void FGRenderBase::ResourcesDumpMemoryUsage()
{
    if (Resources) Resources->_DumpMemoryUsage();
}

DeviceState FGRenderBase::GetDeviceState()
{
    return GEnv.Backend ? GEnv.Backend->GetDeviceState() : DeviceState::Lost;
}

bool FGRenderBase::GetForceGPU_REF()
{
    return GEnv.Backend && GEnv.Backend->GetCapabilities().bForceGPU_REF;
}

u32 FGRenderBase::GetCacheStatPolys()
{
    return 0;
}

void FGRenderBase::Begin()
{
    if (GEnv.Backend)
        GEnv.Backend->BeginFrame();
}

void FGRenderBase::End()
{
    ZoneScopedN("FGRenderBase::BackendEnd");
    if (GEnv.Backend)
    {
        GEnv.Backend->EndFrame();
        GEnv.Backend->Present(psDeviceFlags.test(rsVSync));
    }
}

void FGRenderBase::OnAssetsChanged()
{
    ZoneScoped;
    TextureDescr.UnLoad();
    TextureDescr.Load();
}

xrImTextureData FGRenderBase::GetImGuiTextureId(pcstr texture_name)
{
    if (!Resources)
        return { ImTextureID{}, Fvector2{} };
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

void FGRenderBase::DumpStatistics(IGameFont& font, IPerformanceAlert* alert)
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
    if (alert)
    {
        if (BasicStats.DetailCount > 1000)
            alert->Print(font, "DT_count  > 1000: %u", BasicStats.DetailCount);
    }
    BasicStats.FrameStart();
}

using namespace xray::render::pbr;
void FGRenderBase::ConvertLegacyAssetsToPBR()
{
    if (ps_r4_use_pbr == 0)
        return;

    Msg("~ [PBR] PBR rendering enabled, checking texture conversion...");

    TextureScanConfig scanConfig;
    scanConfig.texture_roots = {"$game_textures$"};
    scanConfig.recursive = true;
    scanConfig.include_levels = true;

    TextureInventory inventory = BuildTextureInventory(scanConfig);
    Msg("~ [PBR] Found %d legacy texture sets", static_cast<int>(inventory.assets.size()));

    PBRConversionParams params;
    params.generate_mipmaps = true;
    params.default_metallic = 0.0f;
    params.default_roughness = 0.5f;
    params.default_ao = 1.0f;

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
}
