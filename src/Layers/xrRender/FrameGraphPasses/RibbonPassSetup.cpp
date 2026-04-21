// xrRender/FrameGraphPasses/RibbonPassSetup.cpp
// GPU ribbon trail rendering — VS reads StructuredBuffer of control points,
// generates Catmull-Rom subdivided quad strip with camera-facing width.
#include "stdafx.h"
#include "RibbonPassSetup.h"
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

namespace xray::render::RENDER_NAMESPACE::passes {

using namespace framegraph;
using namespace bindless;

// ═══════════════════════════════════════════════════════
//  Trail point management (unchanged from CPU version)
// ═══════════════════════════════════════════════════════

static void UpdateTrailPoints(RibbonPassState& state, const Fvector& emitterPos, float dt, float pointSize)
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
        if (diff.magnitude() < RIBBON_MIN_SEGMENT_DIST)
            shouldInsert = false;
    }

    if (shouldInsert) {
        u32 count = std::min(state.pointCount, RIBBON_MAX_POINTS - 1);
        for (u32 i = count; i > 0; i--)
            state.points[i] = state.points[i - 1];

        state.points[0].position = emitterPos;
        state.points[0].age = 0.f;
        state.points[0].size = pointSize;
        state.points[0].order = (u32(state.currentGroupID) << 16) | u32(state.nextSpawnOrder);
        state.nextSpawnOrder++;
        state.pointCount = count + 1;
    }
}

// ═══════════════════════════════════════════════════════
//  Order field group splitting (unchanged from CPU version)
// ═══════════════════════════════════════════════════════

struct RibbonGroup {
    u32 startIdx;
    u32 count;
};

static xr_vector<RibbonGroup> SplitRibbonGroups(const RibbonPoint* points, u32 pointCount)
{
    xr_vector<RibbonGroup> groups;
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

static xr_vector<GPURibbonControlPoint> PackControlPoints(
    const RibbonPoint* points, u32 count, float maxAge)
{
    xr_vector<GPURibbonControlPoint> packed(count);

    float cumDist = 0.0f;
    for (u32 i = 0; i < count; i++) {
        if (i > 0)
            cumDist += points[i].position.distance_to(points[i - 1].position);

        packed[i].posX = points[i].position.x;
        packed[i].posY = points[i].position.y;
        packed[i].posZ = points[i].position.z;
        packed[i].halfWidth = points[i].size;
        packed[i].ageNorm = (maxAge > 0.f) ? (points[i].age / maxAge) : 0.f;
        packed[i].cumDist = cumDist;
    }
    return packed;
}

// ═══════════════════════════════════════════════════════
//  Buffer management
// ═══════════════════════════════════════════════════════

static void EnsureControlPointBuffer(nvrhi::IDevice* nvDevice, u32 pointCount, RibbonPassState& state)
{
    if (state.controlPointBuffer && state.controlPointCapacity >= pointCount)
        return;

    u32 capacity = std::max(pointCount, 64u);
    nvrhi::BufferDesc desc;
    desc.byteSize = capacity * sizeof(GPURibbonControlPoint);
    desc.structStride = sizeof(GPURibbonControlPoint);
    desc.debugName = "RibbonControlPoints";
    desc.initialState = nvrhi::ResourceStates::ShaderResource;
    desc.keepInitialState = true;
    state.controlPointBuffer = nvDevice->createBuffer(desc);
    state.controlPointCapacity = state.controlPointBuffer ? capacity : 0;
}

// ═══════════════════════════════════════════════════════
//  Pipeline initialization
// ═══════════════════════════════════════════════════════

void InitializeRibbonResources(fg::RenderDevice* device, const nvrhi::FramebufferInfoEx& fbInfo, RibbonPassState& state)
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

    // Load GPU ribbon VS (SV_VertexID driven, no input layout)
    auto vsResult = shaderLoader->LoadVertexShader("ribbon", "main");
    if (!vsResult.handle)
        return;
    state.vs = vsResult.handle;

    auto psResult = shaderLoader->LoadPixelShader("ribbon", "main");
    if (!psResult.handle)
        return;
    state.ps = psResult.handle;

    nvrhi::SamplerDesc samplerDesc;
    samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Repeat);
    samplerDesc.setAllFilters(true);
    samplerDesc.setMaxAnisotropy(8.0f);
    state.sampler = nvDevice->createSampler(samplerDesc);

    state.layout = cache.GetOrCreateBindingLayoutFromReflection("RibbonPass", *vsResult.reflection, *psResult.reflection, nvDevice);

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

    state.pipeline = cache.GetOrCreatePipeline("RibbonPass_blend", pipeDesc, fbInfo, nvDevice);

    if (state.pipeline) {
        const auto& actualDesc = state.pipeline->getDesc();
        if (!actualDesc.bindingLayouts.empty())
            state.layout = actualDesc.bindingLayouts[0];
    }

    state.initialized = true;
    Msg("* [RibbonPass] GPU pipeline initialization complete (vertex-ID driven)");
}

// ═══════════════════════════════════════════════════════
//  Pass setup
// ═══════════════════════════════════════════════════════

RibbonPassOutput setupRibbonPass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    const DefaultOutputLayout& forwardInputs,
    u32 width,
    u32 height,
    RibbonPassState* state)
{
    if (state) {
        nvrhi::FramebufferInfoEx fbInfo;
        fbInfo.colorFormats.push_back(nvrhi::Format::RGBA16_FLOAT);
        fbInfo.depthFormat = nvrhi::Format::D32;
        InitializeRibbonResources(device, fbInfo, *state);
    }

    auto& passData = fg.addCallbackPass<RibbonPassData>(
        "Ribbon",
        [&, width, height, state](FrameGraph& builder, PassHandle passHandle, RibbonPassData& data) {
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
        [](const RibbonPassData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
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
            auto framebuffer = cache.GetOrCreateFramebuffer("RibbonPass", fbDesc, nvDevice);
            if (!framebuffer)
                return;

            if (!data.passState->initialized)
                return;

            auto& st = *data.passState;

            // Extract camera basis from inverse view matrix (for screen-space width mode)
            Fmatrix invView;
            invView.invert(Device.mView);

            // Split points by group ID
            auto groups = SplitRibbonGroups(st.points, st.pointCount);

            // Constant buffers
            auto staticGlobalsCB = cache.GetOrCreateVolatileCB("Frame", "StaticGlobals", sizeof(StaticGlobals), data.device);
            auto ribbonParamsCB = cache.GetOrCreateVolatileCB("RibbonPass", "RibbonParams", sizeof(RibbonParamsCB), data.device);

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

                // Pack control points with cumDist
                auto packed = PackControlPoints(&st.points[group.startIdx], group.count, st.maxAge);
                if (packed.empty())
                    continue;

                // Ensure buffer capacity
                EnsureControlPointBuffer(nvDevice, group.count, st);
                if (!st.controlPointBuffer)
                    continue;

                // Upload control points
                cmdList->writeBuffer(st.controlPointBuffer, packed.data(), packed.size() * sizeof(GPURibbonControlPoint));

                // Fill RibbonParams CB
                RibbonParamsCB params = {};
                params.controlPointCount = group.count;
                params.subdivisions = RIBBON_SUBDIVISIONS;
                params.texCoordsFactor = st.texCoordsFactor;
                params.uvPolicy = (u32)st.uvPolicy;
                params.totalDist = packed.back().cumDist;
                params.enableTailFade = st.enableTailFade ? 1u : 0u;
                params.smoothingMode = (u32)st.smoothing;
                params.useScreenSpaceWidth = st.useScreenSpaceWidth ? 1u : 0u;
                params.flipX = st.uvTransform.flipX ? 1u : 0u;
                params.flipY = st.uvTransform.flipY ? 1u : 0u;
                params.rotate90 = st.uvTransform.rotate90 ? 1u : 0u;
                params.materialID = 0;
                params.invViewX.set(invView.i.x, invView.i.y, invView.i.z, 0.f);
                params.invViewY.set(invView.j.x, invView.j.y, invView.j.z, 0.f);
                cmdList->writeBuffer(ribbonParamsCB, &params, sizeof(params));

                // Create binding set for this group
                auto* shaderLoader = GEnv.Render->GetShaderLoader();
                auto* vsReflection = shaderLoader->GetCachedReflection("ribbon", ".vs");
                auto* psReflection = shaderLoader->GetCachedReflection("ribbon", ".ps");
                BindingSetBuilder bsb(*vsReflection, *psReflection, nvDevice, "Ribbon");
                bsb.ConstantBuffer("static_globals", staticGlobalsCB)
                   .ConstantBuffer("RibbonParams", ribbonParamsCB)
                   .BufferSRV("g_Materials", matBuffer.GetBuffer())
                   .BufferSRV("g_ControlPoints", st.controlPointBuffer);
                auto bindDesc = bsb.Build();
                auto bindingSet = cache.GetOrCreateBindingSet(bindDesc, st.layout, nvDevice);

                // Set graphics state
                nvrhi::GraphicsState gfxState;
                gfxState.pipeline = st.pipeline;
                gfxState.framebuffer = framebuffer;
                gfxState.bindings = { bindingSet };
                if (bindlessTable)
                    gfxState.addBindingSet(bindlessTable);
                // No vertex buffers, no index buffer (vertex-ID driven)
                gfxState.viewport.addViewport(viewport);
                gfxState.viewport.addScissorRect(scissor);

                cmdList->setGraphicsState(gfxState);

                // Compute vertex count: (smoothCount - 1) * 6
                u32 segments = group.count - 1;
                u32 smoothCount = segments * RIBBON_SUBDIVISIONS + 1;
                u32 vertexCount = (smoothCount - 1) * 6;

                cmdList->draw(nvrhi::DrawArguments().setVertexCount(vertexCount));
            }
        }
    );

    RibbonPassOutput output;
    output.layout.albedo = passData.outputColor;
    output.layout.normal = passData.outputs.normal;
    output.layout.baseColor = passData.outputs.baseColor;
    output.layout.worldPos = passData.outputs.worldPos;
    output.layout.depth = passData.depth;
    return output;
}

} // namespace xray::render::RENDER_NAMESPACE::passes
