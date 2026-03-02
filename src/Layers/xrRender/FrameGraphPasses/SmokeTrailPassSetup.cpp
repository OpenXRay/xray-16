// SmokeTrailPassSetup.cpp
// 4 framegraph passes: emit CS → simulate CS → compact CS → draw (trail.vs)
#include "stdafx.h"
#include "SmokeTrailPassSetup.h"
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

namespace xray::render::RENDER_NAMESPACE {
    class CRender;
    extern CRender RImplementation;
}

namespace xray::render::RENDER_NAMESPACE::passes {

using namespace framegraph;
using namespace bindless;

// ═══════════════════════════════════════════════════════
//  Initialize compute pipelines (once, lazy)
// ═══════════════════════════════════════════════════════

static void InitSmokeComputePipelines(ng::RenderDevice* device, SmokeTrailPassState& state)
{
    if (state.initialized)
        return;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();

    auto emitCS = RImplementation.m_shaderLoader->LoadComputeShader("smoke_trail_emit", "main").handle;
    auto simCS = RImplementation.m_shaderLoader->LoadComputeShader("smoke_trail_simulate", "main").handle;
    auto compactCS = RImplementation.m_shaderLoader->LoadComputeShader("smoke_trail_compact", "main").handle;

    if (!emitCS || !simCS || !compactCS)
    {
        Msg("! SmokeTrail: failed to load compute shaders");
        return;
    }

    state.emitCS = emitCS;
    state.simCS = simCS;
    state.compactCS = compactCS;

    // Emit layout: b5 + u0(sim) + u1(state)
    {
        nvrhi::BindingLayoutDesc d;
        d.visibility = nvrhi::ShaderType::Compute;
        d.bindings = {
            nvrhi::BindingLayoutItem::VolatileConstantBuffer(5),
            nvrhi::BindingLayoutItem::StructuredBuffer_UAV(0),
            nvrhi::BindingLayoutItem::RawBuffer_UAV(1),
        };
        state.emitLayout = nvDevice->createBindingLayout(d);

        nvrhi::ComputePipelineDesc p;
        p.CS = emitCS;
        p.bindingLayouts = { state.emitLayout };
        state.emitPipeline = nvDevice->createComputePipeline(p);
    }

    // Sim layout: b5 + u0(sim)
    {
        nvrhi::BindingLayoutDesc d;
        d.visibility = nvrhi::ShaderType::Compute;
        d.bindings = {
            nvrhi::BindingLayoutItem::VolatileConstantBuffer(5),
            nvrhi::BindingLayoutItem::StructuredBuffer_UAV(0),
        };
        state.simLayout = nvDevice->createBindingLayout(d);

        nvrhi::ComputePipelineDesc p;
        p.CS = simCS;
        p.bindingLayouts = { state.simLayout };
        state.simPipeline = nvDevice->createComputePipeline(p);
    }

    // Compact layout: b5 + u0(compact output) + u1(state) + u2(drawArgs) + u3(sim input)
    {
        nvrhi::BindingLayoutDesc d;
        d.visibility = nvrhi::ShaderType::Compute;
        d.bindings = {
            nvrhi::BindingLayoutItem::VolatileConstantBuffer(5),
            nvrhi::BindingLayoutItem::StructuredBuffer_UAV(0),
            nvrhi::BindingLayoutItem::RawBuffer_UAV(1),
            nvrhi::BindingLayoutItem::RawBuffer_UAV(2),
            nvrhi::BindingLayoutItem::StructuredBuffer_UAV(3),
        };
        state.compactLayout = nvDevice->createBindingLayout(d);

        nvrhi::ComputePipelineDesc p;
        p.CS = compactCS;
        p.bindingLayouts = { state.compactLayout };
        state.compactPipeline = nvDevice->createComputePipeline(p);
    }

    state.initialized = true;
}

// ═══════════════════════════════════════════════════════
//  Pass data structs
// ═══════════════════════════════════════════════════════

struct SmokeEmitPassData
{
    ng::RenderDevice*    device  = nullptr;
    SmokeTrailManager*   manager = nullptr;
    SmokeTrailPassState* state   = nullptr;
};

struct SmokeSimPassData
{
    ng::RenderDevice*    device  = nullptr;
    SmokeTrailManager*   manager = nullptr;
    SmokeTrailPassState* state   = nullptr;
};

struct SmokeCompactPassData
{
    ng::RenderDevice*    device  = nullptr;
    SmokeTrailManager*   manager = nullptr;
    SmokeTrailPassState* state   = nullptr;
};

struct SmokeDrawPassData
{
    VirtualResourceHandle inputColor;
    VirtualResourceHandle outputColor;
    VirtualResourceHandle depth;
    ng::RenderDevice*    device     = nullptr;
    SmokeTrailManager*   manager    = nullptr;
    SmokeTrailPassState* smokeState = nullptr;
    TrailPassState*      trailState = nullptr;
    DefaultOutputLayout  outputs;
    u32 width  = 0;
    u32 height = 0;
};

// ═══════════════════════════════════════════════════════
//  Setup function — 4 framegraph passes
// ═══════════════════════════════════════════════════════

DefaultOutputLayout setupSmokeTrailPass(
    FrameGraph&                      fg,
    ng::RenderDevice*                device,
    const DefaultOutputLayout&       inputs,
    SmokeTrailManager*               manager,
    TrailPassState*                  trailState,
    u32                              width,
    u32                              height,
    SmokeTrailPassState&             state)
{
    // ── Pass 1: Emit ──
    fg.addCallbackPass<SmokeEmitPassData>(
        "SmokeEmit",
        [&](FrameGraph& builder, PassHandle passHandle, SmokeEmitPassData& data)
        {
            RenderPassBuilder passBuilder(builder, passHandle);
            passBuilder.sideEffects();
            data.device  = device;
            data.manager = manager;
            data.state   = &state;
        },
        [](const SmokeEmitPassData& data, const FrameGraph&, ng::RenderContext* ctx)
        {
            auto* mgr = data.manager;
            auto* st  = data.state;
            if (!mgr || !mgr->IsReady())
                return;

            InitSmokeComputePipelines(data.device, *st);
            if (!st->emitPipeline)
                return;

            const auto& emitParams = mgr->GetEmitParams();
            if (emitParams.emitCount == 0)
                return;

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            nvrhi::IDevice* nvDevice = cmdList->getDevice();
            auto& cache = GetPassResourceCache();

            auto emitCB = cache.GetOrCreateVolatileCB(
                "SmokeTrail", "emit", sizeof(SmokeEmitParams), 16, nvDevice);
            cmdList->writeBuffer(emitCB, &emitParams, sizeof(emitParams));

            nvrhi::BindingSetDesc bindDesc;
            bindDesc.bindings = {
                nvrhi::BindingSetItem::ConstantBuffer(5, emitCB),
                nvrhi::BindingSetItem::StructuredBuffer_UAV(0, mgr->GetSimBuffer()),
                nvrhi::BindingSetItem::RawBuffer_UAV(1, mgr->GetStateBuffer()),
            };
            auto bindSet = nvDevice->createBindingSet(bindDesc, st->emitLayout);

            nvrhi::ComputeState cs;
            cs.pipeline = st->emitPipeline;
            cs.bindings = { bindSet };
            cmdList->setComputeState(cs);

            u32 groups = (emitParams.emitCount + SmokeTrailManager::GROUP_SIZE - 1)
                       / SmokeTrailManager::GROUP_SIZE;
            cmdList->dispatch(groups, 1, 1);
        }
    );

    // ── Pass 2: Simulate ──
    fg.addCallbackPass<SmokeSimPassData>(
        "SmokeSim",
        [&](FrameGraph& builder, PassHandle passHandle, SmokeSimPassData& data)
        {
            RenderPassBuilder passBuilder(builder, passHandle);
            passBuilder.sideEffects();
            data.device  = device;
            data.manager = manager;
            data.state   = &state;
        },
        [](const SmokeSimPassData& data, const FrameGraph&, ng::RenderContext* ctx)
        {
            auto* mgr = data.manager;
            auto* st  = data.state;
            if (!mgr || !mgr->IsReady())
                return;

            InitSmokeComputePipelines(data.device, *st);
            if (!st->simPipeline)
                return;

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            nvrhi::IDevice* nvDevice = cmdList->getDevice();
            auto& cache = GetPassResourceCache();

            const auto& simParams = mgr->GetSimParams();
            auto simCB = cache.GetOrCreateVolatileCB(
                "SmokeTrail", "sim", sizeof(SmokeSimParams), 16, nvDevice);
            cmdList->writeBuffer(simCB, &simParams, sizeof(simParams));

            nvrhi::BindingSetDesc bindDesc;
            bindDesc.bindings = {
                nvrhi::BindingSetItem::ConstantBuffer(5, simCB),
                nvrhi::BindingSetItem::StructuredBuffer_UAV(0, mgr->GetSimBuffer()),
            };
            auto bindSet = nvDevice->createBindingSet(bindDesc, st->simLayout);

            nvrhi::ComputeState cs;
            cs.pipeline = st->simPipeline;
            cs.bindings = { bindSet };
            cmdList->setComputeState(cs);

            u32 groups = (SmokeTrailManager::MAX_POINTS + SmokeTrailManager::GROUP_SIZE - 1)
                       / SmokeTrailManager::GROUP_SIZE;
            cmdList->dispatch(groups, 1, 1);
        }
    );

    // ── Pass 3: Compact ──
    fg.addCallbackPass<SmokeCompactPassData>(
        "SmokeCompact",
        [&](FrameGraph& builder, PassHandle passHandle, SmokeCompactPassData& data)
        {
            RenderPassBuilder passBuilder(builder, passHandle);
            passBuilder.sideEffects();
            data.device  = device;
            data.manager = manager;
            data.state   = &state;
        },
        [](const SmokeCompactPassData& data, const FrameGraph&, ng::RenderContext* ctx)
        {
            auto* mgr = data.manager;
            auto* st  = data.state;
            if (!mgr || !mgr->IsReady())
                return;

            InitSmokeComputePipelines(data.device, *st);
            if (!st->compactPipeline)
                return;

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            nvrhi::IDevice* nvDevice = cmdList->getDevice();
            auto& cache = GetPassResourceCache();

            const auto& compactParams = mgr->GetCompactParams();
            auto compactCB = cache.GetOrCreateVolatileCB(
                "SmokeTrail", "compact", sizeof(SmokeCompactParams), 16, nvDevice);
            cmdList->writeBuffer(compactCB, &compactParams, sizeof(compactParams));

            nvrhi::BindingSetDesc bindDesc;
            bindDesc.bindings = {
                nvrhi::BindingSetItem::ConstantBuffer(5, compactCB),
                nvrhi::BindingSetItem::StructuredBuffer_UAV(0, mgr->GetCompactBuffer()),
                nvrhi::BindingSetItem::RawBuffer_UAV(1, mgr->GetStateBuffer()),
                nvrhi::BindingSetItem::RawBuffer_UAV(2, mgr->GetDrawArgsBuffer()),
                nvrhi::BindingSetItem::StructuredBuffer_UAV(3, mgr->GetSimBuffer()),
            };
            auto bindSet = nvDevice->createBindingSet(bindDesc, st->compactLayout);

            nvrhi::ComputeState cs;
            cs.pipeline = st->compactPipeline;
            cs.bindings = { bindSet };
            cmdList->setComputeState(cs);
            cmdList->dispatch(1, 1, 1);
        }
    );

    // ── Pass 4: Draw (reuses trail.vs pipeline with drawIndirect) ──
    auto& passData = fg.addCallbackPass<SmokeDrawPassData>(
        "SmokeDraw",
        [&](FrameGraph& builder, PassHandle passHandle, SmokeDrawPassData& data)
        {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.device     = device;
            data.manager    = manager;
            data.smokeState = &state;
            data.trailState = trailState;
            data.width      = width;
            data.height     = height;
            data.outputs    = inputs;

            data.inputColor  = passBuilder.read(inputs.albedo);
            data.outputColor = passBuilder.write(inputs.albedo, ResourceState::RenderTarget);
            data.depth       = passBuilder.readWrite(inputs.depth, ResourceState::DepthStencilWrite);
        },
        [](const SmokeDrawPassData& data, const FrameGraph& fg, ng::RenderContext* ctx)
        {
            auto* mgr   = data.manager;
            auto* st    = data.smokeState;
            auto* trail = data.trailState;
            if (!mgr || !mgr->IsReady() || !st || !st->initialized)
                return;
            if (!trail || !trail->initialized)
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

            // Framebuffer: same pattern as trail pass
            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(colorRT);
            fbDesc.setDepthAttachment(depthRT);
            auto& cache = GetPassResourceCache();
            auto framebuffer = cache.GetOrCreateFramebuffer("SmokeDraw", fbDesc, nvDevice);
            if (!framebuffer)
                return;

            // Ensure trail pipeline is initialized (needs framebuffer info)
            InitializeTrailResources(data.device, framebuffer, *trail);
            if (!trail->initialized)
                return;

            // Constant buffers (writeBuffer BEFORE setGraphicsState)
            auto staticGlobalsCB = cache.GetOrCreateVolatileCB(
                "SmokeDraw", "StaticGlobals", sizeof(StaticGlobals), 16, nvDevice).Get();
            auto trailParamsCB = cache.GetOrCreateVolatileCB(
                "SmokeDraw", "TrailParams", sizeof(TrailParamsCB), 16, nvDevice).Get();

            auto staticGlobals = BuildStaticGlobals();
            cmdList->writeBuffer(staticGlobalsCB, &staticGlobals, sizeof(staticGlobals));

            // Trail params — no camera dependency, stored direction
            TrailParamsCB params = {};
            params.controlPointCount = 0;  // Ignored when useGPUState=1
            params.subdivisions      = TRAIL_SUBDIVISIONS;
            params.texCoordsFactor   = 1.0f;
            params.uvPolicy          = (u32)RibbonUVPolicy::DistanceBased;
            params.totalDist         = 0.f;  // Ignored when useGPUState=1
            params.enableTailFade    = 1;
            params.smoothingMode     = (u32)RibbonSmoothingMode::CatmullRom;
            params.edgePolicy        = (u32)TrailEdgePolicy::Center;
            params.flipX             = 0;
            params.flipY             = 0;
            params.rotate90          = 0;
            params.materialID        = 0;
            params.useGPUState       = 1;  // GPU-driven: read from t11 state buffer
            cmdList->writeBuffer(trailParamsCB, &params, sizeof(params));

            const auto& rtDesc = colorRT->getDesc();
            nvrhi::Viewport viewport(
                0.0f, static_cast<float>(rtDesc.width),
                0.0f, static_cast<float>(rtDesc.height),
                0.0f, 1.0f
            );
            nvrhi::Rect scissor(rtDesc.width, rtDesc.height);

            auto* backend = data.device->GetBackend();
            nvrhi::IDescriptorTable* bindlessTable = backend ? backend->GetBindlessDescriptorTable() : nullptr;

            // Binding set: reuse trail layout with smoke buffers
            nvrhi::BindingSetDesc bindDesc;
            bindDesc.bindings = {
                nvrhi::BindingSetItem::ConstantBuffer(2, staticGlobalsCB),
                nvrhi::BindingSetItem::ConstantBuffer(5, trailParamsCB),
                nvrhi::BindingSetItem::StructuredBuffer_SRV(8, matBuffer.GetBuffer()),
                nvrhi::BindingSetItem::StructuredBuffer_SRV(10, mgr->GetCompactBuffer()),
                nvrhi::BindingSetItem::RawBuffer_SRV(11, mgr->GetStateBuffer()),
                nvrhi::BindingSetItem::Sampler(0, trail->sampler),
            };
            auto bindingSet = cache.GetOrCreateBindingSet(bindDesc, trail->layout, nvDevice);

            // Graphics state with indirect params
            nvrhi::GraphicsState gfxState;
            gfxState.pipeline     = trail->pipeline;
            gfxState.framebuffer  = framebuffer;
            gfxState.bindings     = { bindingSet };
            if (bindlessTable)
                gfxState.addBindingSet(bindlessTable);
            gfxState.viewport.addViewport(viewport);
            gfxState.viewport.addScissorRect(scissor);
            gfxState.indirectParams = mgr->GetDrawArgsBuffer();

            cmdList->setGraphicsState(gfxState);
            cmdList->drawIndirect(0);
        }
    );

    DefaultOutputLayout output = inputs;
    output.albedo = passData.outputColor;
    return output;
}

} // namespace xray::render::RENDER_NAMESPACE::passes
