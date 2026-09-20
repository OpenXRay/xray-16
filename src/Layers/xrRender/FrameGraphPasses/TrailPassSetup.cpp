#include "stdafx.h"
#include "TrailPassSetup.h"
#include "PassCommon.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/Backend/D3D12Backend.h"
#include "Layers/xrRender/Bindless/MaterialBuffer.h"

namespace xray::render::fg::passes {

using namespace framegraph;
using namespace bindless;

static void UpdateTrailPoints(TrailPassState& state, const Fvector& emitterPos, float dt, float pointHalfWidth)
{
    u32 writeIdx = 0;
    for (u32 i = 0; i < state.pointCount; i++) {
        state.points[i].age += dt;
        if (state.points[i].age < state.maxAge) {
            if (writeIdx != i)
                state.points[writeIdx] = state.points[i];
            writeIdx++;
        }
    }
    state.pointCount = writeIdx;

    bool shouldInsert = true;
    if (state.pointCount > 0) {
        Fvector diff;
        diff.sub(emitterPos, state.points[0].position);
        if (diff.magnitude() < TRAIL_MIN_SEGMENT_DIST)
            shouldInsert = false;
    }

    if (shouldInsert) {
        u32 count = std::min(state.pointCount, TRAIL_MAX_POINTS - 1);
        for (u32 i = count; i > 0; i--)
            state.points[i] = state.points[i - 1];

        state.points[0].position = emitterPos;
        state.points[0].age = 0.f;
        state.points[0].order = (u32(state.currentGroupID) << 16) | u32(state.nextSpawnOrder);
        state.nextSpawnOrder++;

        Fvector dir = {0, 0, 0};
        if (count > 0) {
            Fvector tangent;
            tangent.sub(emitterPos, state.points[1].position);
            float tangentLen = tangent.magnitude();
            if (tangentLen > 0.0001f) {
                tangent.div(tangentLen);
                Fvector up = {0, 1, 0};
                dir.crossproduct(tangent, up);
                float dirLen = dir.magnitude();
                if (dirLen > 0.0001f) {
                    dir.div(dirLen);
                    dir.mul(pointHalfWidth);
                } else {
                    dir.set(pointHalfWidth, 0, 0);
                }
            } else {
                dir.set(pointHalfWidth, 0, 0);
            }
        } else {
            dir.set(pointHalfWidth, 0, 0);
        }
        state.points[0].direction = dir;

        state.pointCount = count + 1;
    }
}

struct TrailGroup {
    u32 startIdx;
    u32 count;
};

static xr_vector<TrailGroup> SplitTrailGroups(const TrailPoint* points, u32 pointCount)
{
    xr_vector<TrailGroup> groups;
    if (pointCount == 0)
        return groups;

    u16 currentGroup = u16(points[0].order >> 16);
    u32 groupStart = 0;

    for (u32 i = 1; i < pointCount; i++) {
        u16 g = u16(points[i].order >> 16);
        if (g != currentGroup) {
            groups.push_back({groupStart, i - groupStart});
            groupStart = i;
            currentGroup = g;
        }
    }
    groups.push_back({groupStart, pointCount - groupStart});
    return groups;
}

static xr_vector<GPUTrailControlPoint> PackControlPoints(
    const TrailPoint* points, u32 count, float maxAge)
{
    xr_vector<GPUTrailControlPoint> packed(count);

    float cumDist = 0.0f;
    for (u32 i = 0; i < count; i++) {
        if (i > 0)
            cumDist += points[i].position.distance_to(points[i - 1].position);

        packed[i].posX = points[i].position.x;
        packed[i].posY = points[i].position.y;
        packed[i].posZ = points[i].position.z;
        packed[i].dirX = points[i].direction.x;
        packed[i].dirY = points[i].direction.y;
        packed[i].dirZ = points[i].direction.z;
        packed[i].ageNorm = (maxAge > 0.f) ? (points[i].age / maxAge) : 0.f;
        packed[i].cumDist = cumDist;
    }
    return packed;
}

static void EnsureControlPointBuffer(nvrhi::IDevice* nvDevice, u32 pointCount, TrailPassState& state)
{
    if (state.controlPointBuffer && state.controlPointCapacity >= pointCount)
        return;

    u32 capacity = std::max(pointCount, 64u);
    nvrhi::BufferDesc desc;
    desc.byteSize = capacity * sizeof(GPUTrailControlPoint);
    desc.structStride = sizeof(GPUTrailControlPoint);
    desc.debugName = "TrailControlPoints";
    desc.initialState = nvrhi::ResourceStates::ShaderResource;
    desc.keepInitialState = true;
    state.controlPointBuffer = nvDevice->createBuffer(desc);
    state.controlPointCapacity = state.controlPointBuffer ? capacity : 0;
}

static void EnsureDummyStateBuffer(nvrhi::IDevice* nvDevice, TrailPassState& state)
{
    if (state.dummyStateBuffer)
        return;

    nvrhi::BufferDesc desc;
    desc.byteSize = 16;
    desc.initialState = nvrhi::ResourceStates::ShaderResource;
    desc.keepInitialState = true;
    desc.canHaveRawViews = true;
    desc.debugName = "TrailDummyState";
    state.dummyStateBuffer = nvDevice->createBuffer(desc);
}

void InitializeTrailResources(fg::RenderDevice* device, const nvrhi::FramebufferInfoEx& fbInfo, TrailPassState& state)
{
    if (state.initialized)
        return;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    if (!nvDevice)
        return;

    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    if (!shaderLoader)
        return;

    auto* backend = device->GetBackend();
    nvrhi::IBindingLayout* bindlessLayout = backend ? backend->GetBindlessLayout() : nullptr;
    auto& cache = GetPassResourceCache();

    auto vsResult = shaderLoader->LoadVertexShader("trail", "main");
    if (!vsResult.handle)
        return;
    state.vs = vsResult.handle;

    auto psResult = shaderLoader->LoadPixelShader("trail", "main");
    if (!psResult.handle)
        return;
    state.ps = psResult.handle;

    nvrhi::SamplerDesc samplerDesc;
    samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Repeat);
    samplerDesc.setAllFilters(true);
    samplerDesc.setMaxAnisotropy(8.0f);
    state.sampler = nvDevice->createSampler(samplerDesc);

    state.layout = cache.GetOrCreateBindingLayoutFromReflection("TrailPass", *vsResult.reflection, *psResult.reflection, nvDevice);

    nvrhi::GraphicsPipelineDesc pipeDesc;
    pipeDesc.VS = state.vs;
    pipeDesc.PS = state.ps;
    pipeDesc.inputLayout = nullptr;
    pipeDesc.bindingLayouts.push_back(state.layout);
    if (bindlessLayout)
        pipeDesc.bindingLayouts.push_back(bindlessLayout);
    pipeDesc.primType = nvrhi::PrimitiveType::TriangleList;
    pipeDesc.renderState.depthStencilState.depthTestEnable = true;
    pipeDesc.renderState.depthStencilState.depthWriteEnable = false;
    pipeDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::GreaterOrEqual;
    pipeDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::None;
    pipeDesc.renderState.blendState.targets[0].enableBlend();
    pipeDesc.renderState.blendState.targets[0].srcBlend = nvrhi::BlendFactor::SrcAlpha;
    pipeDesc.renderState.blendState.targets[0].destBlend = nvrhi::BlendFactor::InvSrcAlpha;
    pipeDesc.renderState.blendState.targets[0].srcBlendAlpha = nvrhi::BlendFactor::One;
    pipeDesc.renderState.blendState.targets[0].destBlendAlpha = nvrhi::BlendFactor::InvSrcAlpha;

    state.pipeline = cache.GetOrCreatePipeline("TrailPass_blend", pipeDesc, fbInfo, nvDevice);

    if (state.pipeline) {
        const auto& actualDesc = state.pipeline->getDesc();
        if (!actualDesc.bindingLayouts.empty())
            state.layout = actualDesc.bindingLayouts[0];
    }

    EnsureDummyStateBuffer(nvDevice, state);

    state.initialized = true;
    Msg("* [TrailPass] GPU pipeline initialization complete (vertex-ID driven, stored direction)");
}

TrailPassOutput setupTrailPass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    const DefaultOutputLayout& forwardInputs,
    VirtualResourceHandle materials,
    u32 width,
    u32 height,
    TrailPassState* state)
{
    if (state) {
        nvrhi::FramebufferInfoEx fbInfo;
        fbInfo.colorFormats.push_back(nvrhi::Format::RGBA16_FLOAT);
        fbInfo.depthFormat = nvrhi::Format::D32;
        InitializeTrailResources(device, fbInfo, *state);
    }

    auto& passData = fg.addCallbackPass<TrailPassData>(
        "Trail",
        [&, width, height, state](FrameGraph& builder, PassHandle passHandle, TrailPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.width = width;
            data.height = height;
            data.device = device;
            data.passState = state;

            data.inputColor = passBuilder.read(forwardInputs.albedo);
            data.outputColor = passBuilder.write(forwardInputs.albedo, ResourceState::RenderTarget);
            data.depth = passBuilder.readWrite(forwardInputs.depth, ResourceState::DepthStencilWrite);
            data.materials = passBuilder.read(materials, ResourceState::ShaderResource);

            data.outputs.albedo = data.outputColor;
            data.outputs.normal = forwardInputs.normal;
            data.outputs.baseColor = forwardInputs.baseColor;
            data.outputs.depth = data.depth;
        },
        [](const TrailPassData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
            if (!data.passState)
                return;

            auto* colorRT = fg.GetPhysicalTexture(data.outputColor);
            auto* depthRT = fg.GetPhysicalTexture(data.depth);
            if (!colorRT || !depthRT)
                return;

            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            if (!nvDevice || !cmdList)
                return;

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(colorRT);
            fbDesc.setDepthAttachment(depthRT);
            auto& cache = GetPassResourceCache();
            auto framebuffer = cache.GetOrCreateFramebuffer(fbDesc, nvDevice);
            if (!framebuffer)
                return;

            if (!data.passState->initialized)
                return;

            if (data.passState->pointCount < 2)
                return;

            auto& st = *data.passState;

            auto groups = SplitTrailGroups(st.points, st.pointCount);

            auto staticGlobalsCB = cache.GetOrCreateVolatileCB("Frame", "StaticGlobals", sizeof(StaticGlobals), data.device);
            auto trailParamsCB = cache.GetOrCreateVolatileCB("TrailPass", "TrailParams", sizeof(TrailParamsCB), data.device);

            const auto& rtDesc = colorRT->getDesc();
            nvrhi::Viewport viewport(
                0.0f, static_cast<float>(rtDesc.width),
                0.0f, static_cast<float>(rtDesc.height),
                0.0f, 1.0f
            );
            nvrhi::Rect scissor(rtDesc.width, rtDesc.height);

            auto* backend = data.device->GetBackend();
            nvrhi::IDescriptorTable* bindlessTable = backend ? backend->GetBindlessDescriptorTable() : nullptr;

            for (const auto& group : groups) {
                if (group.count < 2)
                    continue;

                auto packed = PackControlPoints(&st.points[group.startIdx], group.count, st.maxAge);
                if (packed.empty())
                    continue;

                EnsureControlPointBuffer(nvDevice, group.count, st);
                if (!st.controlPointBuffer)
                    continue;

                cmdList->writeBuffer(st.controlPointBuffer, packed.data(), packed.size() * sizeof(GPUTrailControlPoint));

                TrailParamsCB params = {};
                params.controlPointCount = group.count;
                params.subdivisions = TRAIL_SUBDIVISIONS;
                params.texCoordsFactor = st.texCoordsFactor;
                params.uvPolicy = (u32)st.uvPolicy;
                params.totalDist = packed.back().cumDist;
                params.enableTailFade = st.enableTailFade ? 1u : 0u;
                params.smoothingMode = (u32)st.smoothing;
                params.edgePolicy = (u32)st.edgePolicy;
                params.flipX = st.uvTransform.flipX ? 1u : 0u;
                params.flipY = st.uvTransform.flipY ? 1u : 0u;
                params.rotate90 = st.uvTransform.rotate90 ? 1u : 0u;
                params.materialID = 0;
                params.useGPUState = 0;
                cmdList->writeBuffer(trailParamsCB, &params, sizeof(params));

                auto* shaderLoader = GEnv.Render->GetShaderLoader();
                auto* vsReflection = shaderLoader->GetCachedReflection("trail", ".vs");
                auto* psReflection = shaderLoader->GetCachedReflection("trail", ".ps");
                if (!vsReflection || !psReflection)
                    return;
                BindingSetBuilder bsb(*vsReflection, *psReflection, nvDevice, "Trail");
                bsb.ConstantBuffer("static_globals", staticGlobalsCB)
                   .ConstantBuffer("TrailParams", trailParamsCB)
                   .BufferSRV("g_Materials", fg.GetPhysicalBuffer(data.materials))
                   .BufferSRV("g_ControlPoints", st.controlPointBuffer)
                   .BufferSRV("g_TrailState", st.dummyStateBuffer);
                const auto& bindDesc = bsb.Build();
                auto bindingSet = cache.GetOrCreateBindingSet(bindDesc, st.layout, nvDevice);

                nvrhi::GraphicsState gfxState;
                gfxState.pipeline = st.pipeline;
                gfxState.framebuffer = framebuffer;
                gfxState.bindings = { bindingSet };
                if (bindlessTable)
                    gfxState.addBindingSet(bindlessTable);
                gfxState.viewport.addViewport(viewport);
                gfxState.viewport.addScissorRect(scissor);

                cmdList->setGraphicsState(gfxState);

                u32 segs = group.count - 1;
                u32 smoothCount = segs * TRAIL_SUBDIVISIONS + 1;
                u32 vertexCount = (smoothCount - 1) * 6;

                cmdList->draw(nvrhi::DrawArguments().setVertexCount(vertexCount));
            }
        }
    );

    TrailPassOutput output;
    output.layout.albedo = passData.outputColor;
    output.layout.normal = passData.outputs.normal;
    output.layout.baseColor = passData.outputs.baseColor;
    output.layout.depth = passData.depth;
    return output;
}

}
