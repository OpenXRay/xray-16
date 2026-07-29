#include "stdafx.h"
#include "PassCommon.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/ResourceManager/TextureManager.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/Environment.h"
#include "xrEngine/device.h"
#include "xrEngine/IRenderBackend.h"
#include "xrCDB/Frustum.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/Bindless/MaterialBuffer.h"
#include "Layers/xrRender/Bindless/TerrainMaterialBuffer.h"
#include "Layers/xrRender/Bindless/VariantTextureBuffer.h"
#include "Layers/xrRender/Bindless/BindlessTypes.h"
#include <atomic>

namespace xray::render::fg::passes {

namespace
{
std::atomic<u32> g_mdiDrawCalls{0};
std::atomic<u32> g_mdiMaxDrawCountSum{0};
}

void ResetMdiDrawCounters()
{
    g_mdiDrawCalls.store(0, std::memory_order_relaxed);
    g_mdiMaxDrawCountSum.store(0, std::memory_order_relaxed);
}

u32 GetMdiDrawCallCount()
{
    return g_mdiDrawCalls.load(std::memory_order_relaxed);
}

u32 GetMdiMaxDrawCountSum()
{
    return g_mdiMaxDrawCountSum.load(std::memory_order_relaxed);
}

void DrawIndexedIndirectCountOrFallback(
    nvrhi::ICommandList* cmdList,
    uint32_t paramOffsetBytes,
    uint32_t countOffsetBytes,
    uint32_t maxDrawCount)
{
    g_mdiDrawCalls.fetch_add(1, std::memory_order_relaxed);
    g_mdiMaxDrawCountSum.fetch_add(maxDrawCount, std::memory_order_relaxed);

    // MoltenVK / older GPUs may lack drawIndirectCount. Callers that take the
    // fallback path must clear unused compact draw-arg slots (instanceCount=0).
    const bool useCount = GEnv.Backend && GEnv.Backend->GetCapabilities().drawIndirectCount;
    if (useCount)
        cmdList->drawIndexedIndirectCount(paramOffsetBytes, countOffsetBytes, maxDrawCount);
    else
        cmdList->drawIndexedIndirect(paramOffsetBytes, maxDrawCount);
}

nvrhi::BufferHandle GetOrCreateDrawIndexBuffer(const char* passName, nvrhi::IDevice* device)
{
    auto& cache = framegraph::GetPassResourceCache();
    if (cache.HasStaticBuffer(passName, "DrawIndexBuffer")) {
        nvrhi::BufferDesc desc;
        desc.byteSize = 65536 * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.isVertexBuffer = true;
        desc.debugName = "DrawIndexBuffer";
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        return cache.GetOrCreateStaticBuffer(passName, "DrawIndexBuffer", desc, device);
    }

    nvrhi::BufferDesc desc;
    desc.byteSize = 65536 * sizeof(u32);
    desc.structStride = sizeof(u32);
    desc.isVertexBuffer = true;
    desc.debugName = "DrawIndexBuffer";
    desc.initialState = nvrhi::ResourceStates::ShaderResource;
    desc.keepInitialState = true;
    auto buffer = cache.GetOrCreateStaticBuffer(passName, "DrawIndexBuffer", desc, device);

    if (buffer && GEnv.Backend) {
        xr_vector<u32> drawIndices(65536);
        for (u32 i = 0; i < 65536; i++)
            drawIndices[i] = i;
        GEnv.Backend->UploadBufferData(buffer, drawIndices.data(), 65536 * sizeof(u32));
    }
    return buffer;
}

LightingConstants FillLightingConstants()
{
    LightingConstants lc;
    if (g_pGamePersistent) {
        auto& env = g_pGamePersistent->Environment().CurrentEnv;
        lc.sunDirection.set(env.sun_dir.x, env.sun_dir.y, env.sun_dir.z, 0.0f);
        lc.sunColor.set(env.sun_color.x, env.sun_color.y, env.sun_color.z, 1.0f);
        lc.fogParams.set(env.fog_near, env.fog_far, env.fog_density, 0.0f);
        lc.fogColor.set(env.fog_color.x, env.fog_color.y, env.fog_color.z, 1.0f);
    } else {
        lc.sunDirection.set(0.5f, -0.7f, 0.5f, 0.0f);
        lc.sunColor.set(1.0f, 1.0f, 1.0f, 1.0f);
        lc.fogParams.set(50.0f, 300.0f, 0.001f, 0.0f);
        lc.fogColor.set(0.5f, 0.5f, 0.6f, 1.0f);
    }
    lc.ambientColor.set(0.1f, 0.1f, 0.15f, 1.0f);
    lc.cameraPosition.set(Device.vCameraPosition.x, Device.vCameraPosition.y, Device.vCameraPosition.z, 1.0f);
    return lc;
}

u32 ExtractFrustumPlanes(Fvector4 outPlanes[6])
{
    CFrustum frustum;
    frustum.CreateFromMatrix(Device.mFullTransform, FRUSTUM_P_LRTB | FRUSTUM_P_FAR);
    u32 count = std::min<u32>((u32)frustum.p_count, 6);
    for (u32 i = 0; i < count; i++) {
        outPlanes[i].set(frustum.planes[i].n.x, frustum.planes[i].n.y, frustum.planes[i].n.z, frustum.planes[i].d);
    }
    return count;
}

void ResolveEnvSkyCubes(fg::RenderDevice* device, nvrhi::ITexture*& outSky0, nvrhi::ITexture*& outSky1)
{
    outSky0 = nullptr;
    outSky1 = nullptr;

    nvrhi::IDevice* nvDevice = device ? device->GetNVRHIDevice() : nullptr;
    auto* texManager = (device && device->GetFGResourceManager())
        ? device->GetFGResourceManager()->GetTextureManager()
        : nullptr;

    // Cache by sky texture name — calling LoadTexture every frame was a major hitch.
    static shared_str s_cachedName0;
    static shared_str s_cachedName1;
    static nvrhi::ITexture* s_cachedTex0 = nullptr;
    static nvrhi::ITexture* s_cachedTex1 = nullptr;

    // Water .s binds $user$sky0/$user$sky1 (full sky cubemaps), not $user$env_s0 (#small).
    if (texManager && g_pGamePersistent)
    {
        auto& env = g_pGamePersistent->Environment();
        const shared_str name0 = (env.Current[0] && env.Current[0]->sky_texture_name.size())
            ? env.Current[0]->sky_texture_name
            : shared_str();
        const shared_str name1 = (env.Current[1] && env.Current[1]->sky_texture_name.size())
            ? env.Current[1]->sky_texture_name
            : shared_str();

        if (name0.size() && name0 != s_cachedName0)
        {
            s_cachedName0 = name0;
            s_cachedTex0 = texManager->GetNVRHITexture(texManager->LoadTexture(name0.c_str()));
        }
        if (name1.size() && name1 != s_cachedName1)
        {
            s_cachedName1 = name1;
            s_cachedTex1 = texManager->GetNVRHITexture(texManager->LoadTexture(name1.c_str()));
        }

        if (name0.size())
            outSky0 = s_cachedTex0;
        if (name1.size())
            outSky1 = s_cachedTex1;
    }

    auto& cache = framegraph::GetPassResourceCache();
    if (!outSky0 && nvDevice)
        outSky0 = cache.GetDummyCubeMap(nvDevice);
    if (!outSky1 && nvDevice)
        outSky1 = cache.GetDummyCubeMap(nvDevice);
}

void BindBindlessMaterialTables(framegraph::BindingSetBuilder& bsb)
{
    // bindless_common.h declares t8/t9/t10 even when a pass only samples g_Materials.
    // Skipping any of them → NVRHI null binding set → invisible geometry.
    // Always bind something: early frames / shadow-only paths may lack real tables.
    static nvrhi::BufferHandle s_dummyMat;
    static nvrhi::BufferHandle s_dummyTerrain;
    static nvrhi::BufferHandle s_dummyVariant;

    auto ensureDummy = [](nvrhi::BufferHandle& slot, const char* name, u32 stride) -> nvrhi::IBuffer* {
        if (!slot)
        {
            auto* backend = GEnv.Render ? GEnv.Render->GetRenderDevice() : nullptr;
            nvrhi::IDevice* dev = backend ? backend->GetNVRHIDevice() : nullptr;
            if (!dev)
                return nullptr;
            nvrhi::BufferDesc desc;
            desc.byteSize = stride * 4;
            desc.structStride = stride;
            desc.debugName = name;
            desc.initialState = nvrhi::ResourceStates::ShaderResource;
            desc.keepInitialState = true;
            slot = dev->createBuffer(desc);
        }
        return slot.Get();
    };

    nvrhi::IBuffer* mats = bindless::MaterialBuffer::Instance().GetBuffer();
    if (!mats)
        mats = ensureDummy(s_dummyMat, "DummyMaterials", sizeof(bindless::MaterialData));
    if (mats)
        bsb.BufferSRV("g_Materials", mats);

    nvrhi::IBuffer* terrain = bindless::TerrainMaterialBuffer::Instance().GetBuffer();
    if (!terrain)
        terrain = ensureDummy(s_dummyTerrain, "DummyTerrainMaterials", sizeof(bindless::TerrainMaterialData));
    if (terrain)
        bsb.BufferSRV("g_TerrainMaterials", terrain);

    nvrhi::IBuffer* variants = bindless::VariantTextureBuffer::Instance().GetBuffer();
    if (!variants)
        variants = ensureDummy(s_dummyVariant, "DummyVariantTextures", sizeof(bindless::VariantTextureData));
    if (variants)
        bsb.BufferSRV("g_VariantTextures", variants);
}

void BindPaintSplatBuffer(framegraph::BindingSetBuilder& bsb, nvrhi::IDevice* device,
    nvrhi::IBuffer* splatBuffer)
{
    nvrhi::IBuffer* buf = splatBuffer;
    if (!buf)
    {
        // skinned_common.h always declares t11 — shadow / HUD may not have overlays.
        static nvrhi::BufferHandle s_dummyPaint;
        if (!s_dummyPaint && device)
        {
            nvrhi::BufferDesc desc;
            desc.byteSize = 256;
            desc.structStride = 64;
            desc.debugName = "DummyPaintSplats";
            desc.initialState = nvrhi::ResourceStates::ShaderResource;
            desc.keepInitialState = true;
            s_dummyPaint = device->createBuffer(desc);
        }
        buf = s_dummyPaint.Get();
    }
    if (buf)
        bsb.BufferSRV("g_PaintSplats", buf);
}

} // namespace xray::render::fg::passes
