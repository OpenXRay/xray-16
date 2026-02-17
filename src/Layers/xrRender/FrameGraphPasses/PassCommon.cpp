#include "stdafx.h"
#include "PassCommon.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/Environment.h"
#include "xrEngine/device.h"
#include "xrCDB/Frustum.h"

namespace xray::render::RENDER_NAMESPACE::passes {

nvrhi::BufferHandle GetOrCreateDrawIndexBuffer(const char* passName, nvrhi::IDevice* device)
{
    auto& cache = framegraph::GetPassResourceCache();
    if (cache.HasStaticBuffer(passName, "DrawIndexBuffer")) {
        nvrhi::BufferDesc desc;
        desc.byteSize = 65536 * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.isVertexBuffer = true;
        desc.debugName = "DrawIndexBuffer";
        desc.initialState = nvrhi::ResourceStates::VertexBuffer;
        desc.keepInitialState = true;
        return cache.GetOrCreateStaticBuffer(passName, "DrawIndexBuffer", desc, device);
    }

    nvrhi::BufferDesc desc;
    desc.byteSize = 65536 * sizeof(u32);
    desc.structStride = sizeof(u32);
    desc.isVertexBuffer = true;
    desc.debugName = "DrawIndexBuffer";
    desc.initialState = nvrhi::ResourceStates::VertexBuffer;
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

nvrhi::FramebufferHandle CreateDummyPipelineFramebuffer(nvrhi::IDevice* device)
{
    nvrhi::TextureDesc colorDesc;
    colorDesc.width = 64;
    colorDesc.height = 64;
    colorDesc.format = nvrhi::Format::RGBA16_FLOAT;
    colorDesc.isRenderTarget = true;
    colorDesc.initialState = nvrhi::ResourceStates::RenderTarget;
    colorDesc.keepInitialState = true;
    colorDesc.debugName = "DummyPipelineColor";
    auto dummyColorRT = device->createTexture(colorDesc);

    nvrhi::TextureDesc normalDesc = colorDesc;
    normalDesc.debugName = "DummyPipelineNormal";
    auto dummyNormalRT = device->createTexture(normalDesc);

    nvrhi::TextureDesc depthDesc;
    depthDesc.width = 64;
    depthDesc.height = 64;
    depthDesc.format = nvrhi::Format::D32;
    depthDesc.isRenderTarget = true;
    depthDesc.initialState = nvrhi::ResourceStates::DepthWrite;
    depthDesc.keepInitialState = true;
    depthDesc.debugName = "DummyPipelineDepth";
    auto dummyDepthRT = device->createTexture(depthDesc);

    nvrhi::FramebufferDesc fbDesc;
    fbDesc.addColorAttachment(dummyColorRT);
    fbDesc.addColorAttachment(dummyNormalRT);
    fbDesc.setDepthAttachment(dummyDepthRT);
    return device->createFramebuffer(fbDesc);
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

} // namespace xray::render::RENDER_NAMESPACE::passes
