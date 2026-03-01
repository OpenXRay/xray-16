// xrRender/FrameGraphPasses/TrailPassSetup.cpp
// GPU trail rendering — Stride ShapeBuilderTrail parity.
// VS reads StructuredBuffer of control points with stored direction,
// generates Catmull-Rom subdivided quad strip. No camera dependency.
// Parallel to RibbonPassSetup.cpp (which implements ShapeBuilderRibbon).
#include "stdafx.h"
#include "TrailPassSetup.h"
#include "PassCommon.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/Backend/D3D12Backend.h"
#include "Layers/xrRender/Bindless/MaterialBuffer.h"

namespace xray::render::RENDER_NAMESPACE::passes {

using namespace framegraph;
using namespace bindless;

// ═══════════════════════════════════════════════════════
//  Trail point management
// ═══════════════════════════════════════════════════════

static void UpdateTrailPoints(TrailPassState& state, const Fvector& emitterPos, float dt, float pointHalfWidth)
{
    // Age existing points & remove expired ones
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

    // Insert new point at front if moved enough from the previous head
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

        // Compute direction: cross(tangent, up) scaled to half-width
        // Stride UpdaterSpeedToDirection: direction from velocity (pos - oldPos)
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
                    // Tangent is parallel to up — use camera right as fallback
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

// ═══════════════════════════════════════════════════════
//  Order field group splitting (same as ribbon)
// ═══════════════════════════════════════════════════════

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

// ═══════════════════════════════════════════════════════
//  Pack control points for GPU upload
// ═══════════════════════════════════════════════════════

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

// ═══════════════════════════════════════════════════════
//  Buffer management
// ═══════════════════════════════════════════════════════

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

// ═══════════════════════════════════════════════════════
//  Pipeline initialization
// ═══════════════════════════════════════════════════════

void InitializeTrailResources(ng::RenderDevice* device, nvrhi::IFramebuffer* framebuffer, TrailPassState& state)
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
    auto fbInfo = framebuffer->getFramebufferInfo();

    // Load trail VS (SV_VertexID driven, no input layout)
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

    // Binding layout: StaticGlobals (b2) + TrailParams (b5) + MaterialBuffer (t8) +
    //                 ControlPoints (t10) + TrailState (t11) + Sampler (s0)
    nvrhi::BindingLayoutDesc layoutDesc;
    layoutDesc.visibility = nvrhi::ShaderType::All;
    layoutDesc.bindings = {
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(2),   // StaticGlobals
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(5),   // TrailParams
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(8),     // MaterialBuffer
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(10),    // ControlPoints
        nvrhi::BindingLayoutItem::RawBuffer_SRV(11),           // TrailState (GPU-driven mode)
        nvrhi::BindingLayoutItem::Sampler(0),
    };
    state.layout = nvDevice->createBindingLayout(layoutDesc);

    // Pipeline: no input layout (vertex-ID driven)
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
    pipeDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::LessOrEqual;
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

    // Create dummy state buffer for CPU-driven mode (t11 must always be bound)
    EnsureDummyStateBuffer(nvDevice, state);

    state.initialized = true;
    Msg("* [TrailPass] GPU pipeline initialization complete (vertex-ID driven, stored direction)");
}

// ═══════════════════════════════════════════════════════
//  Pass setup
// ═══════════════════════════════════════════════════════

TrailPassOutput setupTrailPass(
    FrameGraph& fg,
    ng::RenderDevice* device,
    const DefaultOutputLayout& forwardInputs,
    u32 width,
    u32 height,
    TrailPassState* state)
{
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

            data.outputs.albedo = data.outputColor;
            data.outputs.normal = forwardInputs.normal;
            data.outputs.baseColor = forwardInputs.baseColor;
            data.outputs.worldPos = forwardInputs.worldPos;
            data.outputs.depth = data.depth;
        },
        [](const TrailPassData& data, const FrameGraph& fg, ng::RenderContext* ctx) {
            if (!data.passState || data.passState->pointCount < 2)
                return;

            auto* colorRT = fg.GetPhysicalTexture(data.outputColor);
            auto* depthRT = fg.GetPhysicalTexture(data.depth);
            if (!colorRT || !depthRT)
                return;

            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            if (!nvDevice || !cmdList)
                return;

            auto& matBuffer = MaterialBuffer::Instance();
            matBuffer.Upload(ctx);

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(colorRT);
            fbDesc.setDepthAttachment(depthRT);
            auto& cache = GetPassResourceCache();
            auto framebuffer = cache.GetOrCreateFramebuffer("TrailPass", fbDesc, nvDevice);
            if (!framebuffer)
                return;

            InitializeTrailResources(data.device, framebuffer, *data.passState);
            if (!data.passState->initialized)
                return;

            auto& st = *data.passState;

            // Split points by group ID
            auto groups = SplitTrailGroups(st.points, st.pointCount);

            // Constant buffers
            auto staticGlobalsCB = cache.GetOrCreateVolatileCB("TrailPass", "StaticGlobals", sizeof(StaticGlobals), 16, nvDevice).Get();
            auto trailParamsCB = cache.GetOrCreateVolatileCB("TrailPass", "TrailParams", sizeof(TrailParamsCB), 16, nvDevice).Get();

            auto staticGlobals = BuildStaticGlobals();
            cmdList->writeBuffer(staticGlobalsCB, &staticGlobals, sizeof(staticGlobals));

            const auto& rtDesc = colorRT->getDesc();
            nvrhi::Viewport viewport(
                0.0f, static_cast<float>(rtDesc.width),
                0.0f, static_cast<float>(rtDesc.height),
                0.0f, 1.0f
            );
            nvrhi::Rect scissor(rtDesc.width, rtDesc.height);

            auto* backend = data.device->GetBackend();
            nvrhi::IDescriptorTable* bindlessTable = backend ? backend->GetBindlessDescriptorTable() : nullptr;

            // Draw each group
            for (const auto& group : groups) {
                if (group.count < 2)
                    continue;

                // Pack control points with direction + cumDist
                auto packed = PackControlPoints(&st.points[group.startIdx], group.count, st.maxAge);
                if (packed.empty())
                    continue;

                // Ensure buffer capacity
                EnsureControlPointBuffer(nvDevice, group.count, st);
                if (!st.controlPointBuffer)
                    continue;

                // Upload control points
                cmdList->writeBuffer(st.controlPointBuffer, packed.data(), packed.size() * sizeof(GPUTrailControlPoint));

                // Fill TrailParams CB — no camera dependency, no invViewX/Y
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
                params.useGPUState = 0;  // CPU-driven mode
                cmdList->writeBuffer(trailParamsCB, &params, sizeof(params));

                // Create binding set for this group
                nvrhi::BindingSetDesc bindDesc;
                bindDesc.bindings = {
                    nvrhi::BindingSetItem::ConstantBuffer(2, staticGlobalsCB),
                    nvrhi::BindingSetItem::ConstantBuffer(5, trailParamsCB),
                    nvrhi::BindingSetItem::StructuredBuffer_SRV(8, matBuffer.GetBuffer()),
                    nvrhi::BindingSetItem::StructuredBuffer_SRV(10, st.controlPointBuffer),
                    nvrhi::BindingSetItem::RawBuffer_SRV(11, st.dummyStateBuffer),
                    nvrhi::BindingSetItem::Sampler(0, st.sampler),
                };
                auto bindingSet = cache.GetOrCreateBindingSet(bindDesc, st.layout, nvDevice);

                // Set graphics state
                nvrhi::GraphicsState gfxState;
                gfxState.pipeline = st.pipeline;
                gfxState.framebuffer = framebuffer;
                gfxState.bindings = { bindingSet };
                if (bindlessTable)
                    gfxState.addBindingSet(bindlessTable);
                gfxState.viewport.addViewport(viewport);
                gfxState.viewport.addScissorRect(scissor);

                cmdList->setGraphicsState(gfxState);

                // Compute vertex count: (smoothCount - 1) * 6
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
    output.layout.worldPos = passData.outputs.worldPos;
    output.layout.depth = passData.depth;
    return output;
}

} // namespace xray::render::RENDER_NAMESPACE::passes
