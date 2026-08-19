#include "stdafx.h"
#include "PassCommon.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/Bindless/MaterialBuffer.h"
#include "Layers/xrRender/Bindless/TerrainMaterialBuffer.h"
#include "Layers/xrRender/Bindless/VariantTextureBuffer.h"
#include "Layers/xrRender/Bindless/BindlessTypes.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/ResourceManager/TextureManager.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/Environment.h"
#include "xrEngine/device.h"
#include "xrCDB/Frustum.h"

namespace xray::render::fg::passes {

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

    static shared_str s_cachedName0;
    static shared_str s_cachedName1;
    static nvrhi::ITexture* s_cachedTex0 = nullptr;
    static nvrhi::ITexture* s_cachedTex1 = nullptr;

    if (texManager && g_pGamePersistent)
    {
        auto& env = g_pGamePersistent->Environment();
        shared_str name0;
        shared_str name1;
        if (env.Current[0])
        {
            name0 = env.Current[0]->sky_texture_env_name.size()
                ? env.Current[0]->sky_texture_env_name
                : env.Current[0]->sky_texture_name;
        }
        if (env.Current[1])
        {
            name1 = env.Current[1]->sky_texture_env_name.size()
                ? env.Current[1]->sky_texture_env_name
                : env.Current[1]->sky_texture_name;
        }

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

        static shared_str s_reportedName0;
        static shared_str s_reportedName1;
        if (name0 != s_reportedName0 || name1 != s_reportedName1)
        {
            Msg("* [EnvIBL] env_s0='%s' resolved=%d env_s1='%s' resolved=%d",
                name0.c_str(), outSky0 ? 1 : 0, name1.c_str(), outSky1 ? 1 : 0);
            s_reportedName0 = name0;
            s_reportedName1 = name1;
        }
    }

    auto& cache = framegraph::GetPassResourceCache();
    if (!outSky0 && nvDevice)
        outSky0 = cache.GetDummyCubeMap(nvDevice);
    if (!outSky1 && nvDevice)
        outSky1 = cache.GetDummyCubeMap(nvDevice);
}

void BindBindlessMaterialTables(framegraph::BindingSetBuilder& bsb)
{
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
    if (mats && bsb.HasSRV("g_Materials"))
        bsb.BufferSRV("g_Materials", mats);

    nvrhi::IBuffer* terrain = bindless::TerrainMaterialBuffer::Instance().GetBuffer();
    if (!terrain)
        terrain = ensureDummy(s_dummyTerrain, "DummyTerrainMaterials", sizeof(bindless::TerrainMaterialData));
    if (terrain && bsb.HasSRV("g_TerrainMaterials"))
        bsb.BufferSRV("g_TerrainMaterials", terrain);

    nvrhi::IBuffer* variants = bindless::VariantTextureBuffer::Instance().GetBuffer();
    if (!variants)
        variants = ensureDummy(s_dummyVariant, "DummyVariantTextures", sizeof(bindless::VariantTextureData));
    if (variants && bsb.HasSRV("g_VariantTextures"))
        bsb.BufferSRV("g_VariantTextures", variants);
}

void BindEnvIblCubes(framegraph::BindingSetBuilder& bsb, fg::RenderDevice* device)
{
    if (!bsb.HasSRV("env_s0") && !bsb.HasSRV("env_s1"))
        return;

    nvrhi::ITexture* sky0 = nullptr;
    nvrhi::ITexture* sky1 = nullptr;
    ResolveEnvSkyCubes(device, sky0, sky1);
    if (bsb.HasSRV("env_s0") && sky0)
        bsb.Texture("env_s0", sky0);
    if (bsb.HasSRV("env_s1") && sky1)
        bsb.Texture("env_s1", sky1);
}

} // namespace xray::render::fg::passes
