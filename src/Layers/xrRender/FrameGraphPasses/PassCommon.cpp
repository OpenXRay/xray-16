#include "stdafx.h"
#include "PassCommon.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/Environment.h"
#include "xrEngine/device.h"
#include "xrCDB/Frustum.h"

namespace xray::render::fg::passes {


void RecordGeometryShadowDraw(nvrhi::ICommandList* cmd, nvrhi::IDevice* device,
    nvrhi::IBuffer* dirtyList, nvrhi::IBuffer* drawArgs, nvrhi::IBuffer* geometryDirty, u32 capacity,
    ShadowPublication publication, nvrhi::IBuffer* cacheState, nvrhi::IBuffer* pageList,
    nvrhi::ComputePipelineHandle& pipeline)
{
    const bool vsm = publication == ShadowPublication::VSM;
    const char* entry = vsm ? "publishVSM" : "publishLocal";
    R_ASSERT(cacheState && (!vsm || pageList));
    auto* loader = GEnv.Render->GetShaderLoader();
    auto& cache = framegraph::GetPassResourceCache();
    if (!pipeline)
    {
        auto shader = loader->LoadComputeShader("geometry_shadow_accept", entry);
        R_ASSERT(shader.handle && shader.reflection);
        auto layout = cache.GetOrCreateBindingLayoutFromReflection(entry, *shader.reflection, device);
        R_ASSERT(layout);
        nvrhi::ComputePipelineDesc desc;
        desc.CS = shader.handle;
        desc.bindingLayouts = { layout };
        pipeline = cache.GetOrCreateComputePipeline(entry, desc, device);
        R_ASSERT(pipeline);
    }
    auto* reflection = loader->GetCachedReflection("geometry_shadow_accept",
        vsm ? ".cs:publishVSM" : ".cs:publishLocal");
    R_ASSERT(reflection);
    auto layout = pipeline->getDesc().bindingLayouts[0];
    framegraph::BindingSetBuilder builder(*reflection, device, entry);
    builder.BufferSRV("g_DirtyList", dirtyList)
        .BufferSRV("g_DrawArgs", drawArgs)
        .BufferUAV("g_GeometryDirty", geometryDirty);
    if (vsm)
        builder.BufferSRV("g_PageList", pageList).BufferUAV("g_PageTable", cacheState);
    else
        builder.BufferUAV("g_TileState", cacheState);
    auto bindings = cache.GetOrCreateBindingSet(builder.Build(), layout, device);
    R_ASSERT(bindings);
    cmd->setBufferState(dirtyList, nvrhi::ResourceStates::NonPixelShaderResource);
    cmd->setBufferState(drawArgs, nvrhi::ResourceStates::NonPixelShaderResource);
    cmd->setBufferState(geometryDirty, nvrhi::ResourceStates::UnorderedAccess);
    cmd->setBufferState(cacheState, nvrhi::ResourceStates::UnorderedAccess);
    if (vsm)
        cmd->setBufferState(pageList, nvrhi::ResourceStates::NonPixelShaderResource);
    cmd->commitBarriers();
    nvrhi::ComputeState state;
    state.pipeline = pipeline;
    state.bindings = { bindings };
    cmd->setComputeState(state);
    cmd->dispatch((capacity + 63u) / 64u, 1, 1);
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

} // namespace xray::render::fg::passes
