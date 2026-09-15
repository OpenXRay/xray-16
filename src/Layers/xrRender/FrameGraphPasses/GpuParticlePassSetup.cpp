#include "stdafx.h"
#include "GpuParticlePassSetup.h"
#include "ParticlePassSetup.h"
#include "PassCommon.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/GpuParticleManager.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/Backend/D3D12Backend.h"
#include "Layers/xrRender/Bindless/MaterialBuffer.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"

namespace xray::render::fg::passes {

using namespace framegraph;

struct GpuParticleDrawParams {
    Fmatrix hudWarp;
    Fvector4 cameraTop;
    Fvector4 cameraRight;
    u32 drawBucket;
    u32 padding[3];
};
static_assert(sizeof(GpuParticleDrawParams) == 112);
static_assert(sizeof(nvrhi::DrawIndirectArguments) == 16);

struct GpuParticlePassData {
    fg::RenderDevice* device;
    MaterialCache* materialCache;
    GpuParticleDrawResources resources;
    VirtualResourceHandle color, depth, normal, baseColor, distortion, sceneDepth;
    GpuParticlePassState* state;
    u32 width, height;
    bool clearDistortion;
};

struct GpuParticleBlendDesc {
    nvrhi::BlendFactor source, destination, sourceAlpha, destinationAlpha;
    const char* name;
};

static constexpr GpuParticleBlendDesc gpuParticleBlends[PARTICLE_BLEND_COUNT] = {
    { nvrhi::BlendFactor::One, nvrhi::BlendFactor::Zero, nvrhi::BlendFactor::One, nvrhi::BlendFactor::Zero, "GpuParticle.Set" },
    { nvrhi::BlendFactor::SrcAlpha, nvrhi::BlendFactor::InvSrcAlpha, nvrhi::BlendFactor::One, nvrhi::BlendFactor::InvSrcAlpha, "GpuParticle.Blend" },
    { nvrhi::BlendFactor::SrcAlpha, nvrhi::BlendFactor::One, nvrhi::BlendFactor::One, nvrhi::BlendFactor::One, "GpuParticle.Add" },
    { nvrhi::BlendFactor::DstColor, nvrhi::BlendFactor::Zero, nvrhi::BlendFactor::One, nvrhi::BlendFactor::Zero, "GpuParticle.Mul" },
    { nvrhi::BlendFactor::DstColor, nvrhi::BlendFactor::SrcColor, nvrhi::BlendFactor::One, nvrhi::BlendFactor::SrcAlpha, "GpuParticle.Mul2x" },
    { nvrhi::BlendFactor::SrcAlpha, nvrhi::BlendFactor::One, nvrhi::BlendFactor::One, nvrhi::BlendFactor::One, "GpuParticle.AlphaAdd" }
};

static constexpr u32 gpuParticleSetMask = (1u << 0) | (1u << 6);
static constexpr u32 gpuParticleColorMask = (1u << 12) - 1;
static constexpr u32 gpuParticleSoftMask = gpuParticleColorMask & ~gpuParticleSetMask;
static constexpr u32 gpuParticleDistortionMask = (1u << 12) | (1u << 13);

static void InitializeGpuParticlePipelines(const FrameGraph& graph, GpuParticlePassData& data)
{
    auto& state = *data.state;
    nvrhi::FramebufferInfoEx framebuffer;
    framebuffer.colorFormats.push_back(graph.GetResourceDesc(data.color).format);
    framebuffer.colorFormats.push_back(graph.GetResourceDesc(data.normal).format);
    framebuffer.colorFormats.push_back(graph.GetResourceDesc(data.baseColor).format);
    framebuffer.depthFormat = graph.GetResourceDesc(data.depth).format;
    framebuffer.sampleCount = graph.GetResourceDesc(data.color).sampleCount;
    const auto distortionFormat = graph.GetResourceDesc(data.distortion).format;
    if (state.distortPipeline && state.framebuffer == framebuffer && state.distortionFormat == distortionFormat)
        return;
    auto* device = data.device->GetNVRHIDevice();
    auto* loader = GEnv.Render->GetShaderLoader();
    VERIFY(device && loader);
    auto vertex = loader->LoadVertexShader("gpu_particle");
    auto set = loader->LoadPixelShader("bindless_particle_set");
    auto pixel = loader->LoadPixelShader("bindless_particle");
    auto distort = loader->LoadPixelShader("bindless_particle_distort");
    VERIFY2(vertex.handle && set.handle && pixel.handle && distort.handle, "GPU particle shaders are required");
    VERIFY(vertex.reflection && set.reflection && pixel.reflection && distort.reflection);
    state.vertexReflection = loader->GetCachedReflection("gpu_particle", ".vs");
    state.setReflection = loader->GetCachedReflection("bindless_particle_set", ".ps");
    state.pixelReflection = loader->GetCachedReflection("bindless_particle", ".ps");
    state.distortReflection = loader->GetCachedReflection("bindless_particle_distort", ".ps");
    VERIFY(state.vertexReflection && state.setReflection && state.pixelReflection && state.distortReflection);
    auto& cache = GetPassResourceCache();
    auto setLayout = cache.GetOrCreateBindingLayoutFromReflection("GpuParticle.Set", *vertex.reflection, *set.reflection, device);
    auto layout = cache.GetOrCreateBindingLayoutFromReflection("GpuParticle", *vertex.reflection, *pixel.reflection, device);
    auto distortLayout = cache.GetOrCreateBindingLayoutFromReflection("GpuParticle.Distort", *vertex.reflection, *distort.reflection, device);
    auto* backend = data.device->GetBackend();
    VERIFY(backend && backend->GetBindlessLayout());
    nvrhi::GraphicsPipelineDesc pipeline;
    pipeline.VS = vertex.handle;
    pipeline.PS = pixel.handle;
    pipeline.bindingLayouts = { layout, backend->GetBindlessLayout() };
    pipeline.primType = nvrhi::PrimitiveType::TriangleList;
    pipeline.renderState.depthStencilState.depthTestEnable = true;
    pipeline.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::GreaterOrEqual;
    pipeline.renderState.rasterState.cullMode = nvrhi::RasterCullMode::None;
    for (u32 mode = 0; mode < PARTICLE_BLEND_COUNT; ++mode) {
        const auto& blend = gpuParticleBlends[mode];
        pipeline.PS = mode == PARTICLE_BLEND_SET ? set.handle : pixel.handle;
        pipeline.bindingLayouts = { mode == PARTICLE_BLEND_SET ? setLayout : layout, backend->GetBindlessLayout() };
        auto& target = pipeline.renderState.blendState.targets[0];
        target.blendEnable = mode != PARTICLE_BLEND_SET;
        target.srcBlend = blend.source;
        target.destBlend = blend.destination;
        target.srcBlendAlpha = blend.sourceAlpha;
        target.destBlendAlpha = blend.destinationAlpha;
        pipeline.renderState.depthStencilState.depthWriteEnable = mode == PARTICLE_BLEND_SET;
        const auto auxiliaryMask = mode == PARTICLE_BLEND_SET ? nvrhi::ColorMask::All : static_cast<nvrhi::ColorMask>(0);
        pipeline.renderState.blendState.targets[1].colorWriteMask = auxiliaryMask;
        pipeline.renderState.blendState.targets[2].colorWriteMask = auxiliaryMask;
        state.pipelines[mode] = cache.GetOrCreatePipeline(blend.name, pipeline, framebuffer, device);
        VERIFY2(state.pipelines[mode], "GPU particle blend pipeline is required");
    }
    state.framebuffer = framebuffer;
    state.distortionFormat = distortionFormat;
    framebuffer.colorFormats = { distortionFormat };
    pipeline.PS = distort.handle;
    pipeline.bindingLayouts = { distortLayout, backend->GetBindlessLayout() };
    pipeline.renderState.depthStencilState.depthWriteEnable = false;
    auto& target = pipeline.renderState.blendState.targets[0];
    target.blendEnable = true;
    target.srcBlend = nvrhi::BlendFactor::One;
    target.destBlend = nvrhi::BlendFactor::One;
    target.srcBlendAlpha = nvrhi::BlendFactor::One;
    target.destBlendAlpha = nvrhi::BlendFactor::One;
    state.distortPipeline = cache.GetOrCreatePipeline("GpuParticle.Distort", pipeline, framebuffer, device);
    VERIFY2(state.distortPipeline, "GPU particle distortion pipeline is required");
}

static void DrawGpuParticles(const GpuParticlePassData& data, const FrameGraph& graph, fg::RenderContext* context)
{
    auto* device = data.device->GetNVRHIDevice();
    auto* commandList = context->GetCommandList();
    const auto& passState = *data.state;
    auto& cache = GetPassResourceCache();
    auto& materials = bindless::MaterialBuffer::Instance();
    if (data.materialCache)
        data.materialCache->FinalizePendingMaterials(context);
    materials.Upload(context);
    auto* staticGlobals = cache.GetOrCreateVolatileCB("Frame", "StaticGlobals", sizeof(StaticGlobals), data.device);
    auto* drawConstants = cache.GetOrCreateVolatileCB("GpuParticle", "DrawParams", sizeof(GpuParticleDrawParams), data.device, 128);
    GpuParticleDrawParams params = {};
    params.hudWarp = HudFovWarp();
    params.cameraTop.set(Device.vCameraTop.x, Device.vCameraTop.y, Device.vCameraTop.z, 0.0f);
    params.cameraRight.set(Device.vCameraRight.x, Device.vCameraRight.y, Device.vCameraRight.z, 0.0f);
    auto bind = [&](const ExtractedReflection& pixel, nvrhi::IBindingLayout* layout, bool softParticles) {
        BindingSetBuilder bindings(*passState.vertexReflection, pixel, device, "GpuParticle");
        bindings.ConstantBuffer("static_globals", staticGlobals)
            .ConstantBuffer("GpuParticleDrawParams", drawConstants)
            .BufferSRV("g_Particles", graph.GetPhysicalBuffer(data.resources.particleResource))
            .BufferSRV("g_Emitters", graph.GetPhysicalBuffer(data.resources.emitterResource))
            .BufferSRV("g_Programs", graph.GetPhysicalBuffer(data.resources.programResource))
            .BufferSRV("g_ParticleIndices", graph.GetPhysicalBuffer(data.resources.indexResource))
            .BufferSRV("g_BucketOffsets", graph.GetPhysicalBuffer(data.resources.bucketResource))
            .BufferSRV("g_Materials", materials.GetBuffer());
        if (softParticles)
            bindings.Texture("g_SceneDepth", graph.GetPhysicalTexture(data.sceneDepth));
        return cache.GetOrCreateBindingSet(bindings.Build(), layout, device);
    };
    const u32 mask = data.resources.drawBucketMask;
    nvrhi::BindingSetHandle setBindingSet, bindingSet, distortBindingSet;
    if (mask & gpuParticleSetMask) {
        setBindingSet = bind(*passState.setReflection, passState.pipelines[PARTICLE_BLEND_SET]->getDesc().bindingLayouts[0], false);
        VERIFY(setBindingSet);
    }
    if (mask & gpuParticleSoftMask) {
        bindingSet = bind(*passState.pixelReflection, passState.pipelines[PARTICLE_BLEND_BLEND]->getDesc().bindingLayouts[0], true);
        VERIFY(bindingSet);
    }
    if (mask & gpuParticleDistortionMask) {
        distortBindingSet = bind(*passState.distortReflection, passState.distortPipeline->getDesc().bindingLayouts[0], false);
        VERIFY(distortBindingSet);
    }
    const auto depthAttachment = nvrhi::FramebufferAttachment()
        .setTexture(graph.GetPhysicalTexture(data.depth)).setReadOnly((mask & gpuParticleSetMask) == 0);
    nvrhi::FramebufferHandle framebuffer, distortFramebuffer;
    if (mask & gpuParticleColorMask) {
        nvrhi::FramebufferDesc framebufferDesc;
        framebufferDesc.addColorAttachment(graph.GetPhysicalTexture(data.color));
        framebufferDesc.addColorAttachment(graph.GetPhysicalTexture(data.normal));
        framebufferDesc.addColorAttachment(graph.GetPhysicalTexture(data.baseColor));
        framebufferDesc.setDepthAttachment(depthAttachment);
        framebuffer = cache.GetOrCreateFramebuffer(framebufferDesc, device);
        VERIFY(framebuffer);
    }
    auto* distortion = graph.GetPhysicalTexture(data.distortion);
    if (mask & gpuParticleDistortionMask) {
        nvrhi::FramebufferDesc distortDesc;
        distortDesc.addColorAttachment(distortion);
        distortDesc.setDepthAttachment(depthAttachment);
        distortFramebuffer = cache.GetOrCreateFramebuffer(distortDesc, device);
        VERIFY(distortFramebuffer);
    }
    if (data.clearDistortion)
        commandList->clearTextureFloat(distortion, nvrhi::AllSubresources, nvrhi::Color(0.0f, 0.0f, 0.0f, 0.0f));
    auto* bindlessTable = data.device->GetBackend()->GetBindlessDescriptorTable();
    VERIFY(bindlessTable);
    nvrhi::GraphicsState state;
    state.indirectParams = graph.GetPhysicalBuffer(data.resources.argsResource);
    state.viewport.addScissorRect(nvrhi::Rect(data.width, data.height));
    state.viewport.viewports = { nvrhi::Viewport(0.0f, float(data.width), 0.0f, float(data.height), 0.0f, 1.0f) };
    static constexpr u32 buckets[] = { 0, 3, 4, 1, 2, 5, 6, 9, 10, 7, 8, 11, 12, 13 };
    for (u32 bucket : buckets) {
        if ((mask & (1u << bucket)) == 0)
            continue;
        params.drawBucket = bucket;
        commandList->writeBuffer(drawConstants, &params, sizeof(params));
        bool isDistortion = bucket >= 12;
        state.pipeline = isDistortion ? passState.distortPipeline : passState.pipelines[bucket % PARTICLE_BLEND_COUNT];
        state.framebuffer = isDistortion ? distortFramebuffer : framebuffer;
        state.bindings = { isDistortion ? distortBindingSet :
            (bucket % PARTICLE_BLEND_COUNT == PARTICLE_BLEND_SET ? setBindingSet : bindingSet), bindlessTable };
        commandList->setGraphicsState(state);
        commandList->drawIndirect(bucket * sizeof(nvrhi::DrawIndirectArguments), 1);
    }
}

GpuParticlePassOutputs setupGpuParticlePass(
    FrameGraph& graph,
    fg::RenderDevice* device,
    const GpuParticleDrawResources& resources,
    MaterialCache* materialCache,
    VirtualResourceHandle color,
    VirtualResourceHandle depth,
    VirtualResourceHandle normal,
    VirtualResourceHandle baseColor,
    VirtualResourceHandle distortion,
    u32 width,
    u32 height,
    GpuParticlePassState& state)
{
    if (resources.particleCapacity == 0 || resources.drawBucketMask == 0)
        return { color, distortion };
    VERIFY(device && color.is_valid() && depth.is_valid() && normal.is_valid() && baseColor.is_valid());
    VERIFY(resources.particleResource.is_valid() && resources.emitterResource.is_valid() && resources.programResource.is_valid());
    VERIFY(resources.indexResource.is_valid() && resources.argsResource.is_valid() && resources.bucketResource.is_valid());
    const bool writesDepth = (resources.drawBucketMask & gpuParticleSetMask) != 0;
    const bool softParticles = (resources.drawBucketMask & gpuParticleSoftMask) != 0;
    VirtualResourceHandle sceneDepth;
    if (softParticles) {
        sceneDepth = depth;
        if (writesDepth) {
            ResourceDesc depthDesc = graph.GetResourceDesc(depth);
            depthDesc.debugName = "GpuParticle.SceneDepth";
            depthDesc.isImported = false;
            depthDesc.isTransient = true;
            sceneDepth = graph.CreateTexture("GpuParticle.SceneDepth", depthDesc);
            struct DepthCopyData { VirtualResourceHandle source, destination; };
            graph.addCallbackPass<DepthCopyData>("GpuParticle.DepthCopy",
                [&](FrameGraph& builder, PassHandle handle, DepthCopyData& data) {
                    RenderPassBuilder pass(builder, handle);
                    data.source = pass.read(depth, ResourceState::CopySource);
                    data.destination = pass.write(sceneDepth, ResourceState::CopyDest);
                },
                [](const DepthCopyData& data, const FrameGraph& graph, fg::RenderContext* context) {
                    context->CopyTexture(graph.GetPhysicalTexture(data.destination), graph.GetPhysicalTexture(data.source));
                });
        }
    }
    auto& passData = graph.addCallbackPass<GpuParticlePassData>("GpuParticles",
        [&](FrameGraph& builder, PassHandle handle, GpuParticlePassData& data) {
            RenderPassBuilder pass(builder, handle);
            data.device = device;
            data.materialCache = materialCache;
            data.state = &state;
            data.width = width;
            data.height = height;
            data.resources = resources;
            pass.read(resources.particleResource);
            pass.read(resources.emitterResource);
            pass.read(resources.programResource);
            pass.read(resources.indexResource);
            pass.read(resources.bucketResource);
            pass.read(resources.argsResource, ResourceState::IndirectArgument);
            data.color = color;
            data.normal = normal;
            data.baseColor = baseColor;
            if (resources.drawBucketMask & gpuParticleColorMask) {
                pass.readWrite(color, ResourceState::RenderTarget);
                pass.readWrite(normal, ResourceState::RenderTarget);
                pass.readWrite(baseColor, ResourceState::RenderTarget);
            }
            if (writesDepth)
                data.depth = pass.readWrite(depth, ResourceState::DepthStencilWrite);
            else
                data.depth = pass.read(depth, softParticles ?
                    ResourceState::DepthStencilReadShaderResource : ResourceState::DepthStencilRead);
            data.sceneDepth = sceneDepth;
            if (softParticles && writesDepth)
                pass.read(sceneDepth);
            data.clearDistortion = !distortion.is_valid();
            if (distortion.is_valid())
                data.distortion = pass.readWrite(distortion, ResourceState::RenderTarget);
            else {
                ResourceDesc desc;
                desc.width = width;
                desc.height = height;
                desc.format = nvrhi::Format::RGBA16_FLOAT;
                desc.isRenderTarget = true;
                desc.isUAV = true;
                desc.debugName = "rt_Distortion";
                data.distortion = pass.createTexture("rt_Distortion", desc);
            }
            InitializeGpuParticlePipelines(builder, data);
        }, DrawGpuParticles);
    return { passData.color, passData.distortion };
}

}
