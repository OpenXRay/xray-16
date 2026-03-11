// SmokeTrailPassSetup.cpp
// 4 framegraph passes: emit CS → simulate CS → compact CS → draw (smoke_trail.vs)
#include "stdafx.h"
#include "SmokeTrailPassSetup.h"
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
    auto& cache = GetPassResourceCache();

    auto emitResult = RImplementation.m_shaderLoader->LoadComputeShader("smoke_trail_emit", "main");
    auto simResult = RImplementation.m_shaderLoader->LoadComputeShader("smoke_trail_simulate", "main");
    auto compactResult = RImplementation.m_shaderLoader->LoadComputeShader("smoke_trail_compact", "main");

    if (!emitResult.handle || !simResult.handle || !compactResult.handle)
    {
        Msg("! SmokeTrail: failed to load compute shaders");
        return;
    }

    state.emitCS = emitResult.handle;
    state.simCS = simResult.handle;
    state.compactCS = compactResult.handle;

    {
        state.emitLayout = cache.GetOrCreateBindingLayoutFromReflection("SmokeTrail_Emit", *emitResult.reflection, nvDevice);

        nvrhi::ComputePipelineDesc p;
        p.CS = emitResult.handle;
        p.bindingLayouts = { state.emitLayout };
        state.emitPipeline = nvDevice->createComputePipeline(p);
    }

    {
        state.simLayout = cache.GetOrCreateBindingLayoutFromReflection("SmokeTrail_Simulate", *simResult.reflection, nvDevice);

        nvrhi::ComputePipelineDesc p;
        p.CS = simResult.handle;
        p.bindingLayouts = { state.simLayout };
        state.simPipeline = nvDevice->createComputePipeline(p);
    }

    {
        state.compactLayout = cache.GetOrCreateBindingLayoutFromReflection("SmokeTrail_Compact", *compactResult.reflection, nvDevice);

        nvrhi::ComputePipelineDesc p;
        p.CS = compactResult.handle;
        p.bindingLayouts = { state.compactLayout };
        state.compactPipeline = nvDevice->createComputePipeline(p);
    }

    state.initialized = true;
}

// ═══════════════════════════════════════════════════════
//  Initialize smoke draw pipeline (once, needs framebuffer)
// ═══════════════════════════════════════════════════════

static void InitSmokeDrawPipeline(
    ng::RenderDevice* device,
    SmokeTrailPassState& state)
{
    if (state.drawPipeline)
        return;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    if (!nvDevice)
        return;

    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    if (!shaderLoader)
        return;

    // Dedicated smoke trail VS — no Catmull-Rom, direct control point quad strip
    auto vsResult = shaderLoader->LoadVertexShader("smoke_trail", "main");
    if (!vsResult.handle)
    {
        Msg("! SmokeTrail: failed to load smoke_trail.vs");
        return;
    }
    state.drawVS = vsResult.handle;

    auto psResult = shaderLoader->LoadPixelShader("smoke_trail", "main");
    if (!psResult.handle)
    {
        Msg("! SmokeTrail: failed to load smoke_trail.ps");
        return;
    }
    state.drawPS = psResult.handle;

    // Sampler
    nvrhi::SamplerDesc samplerDesc;
    samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Repeat);
    samplerDesc.setAllFilters(true);
    samplerDesc.setMaxAnisotropy(8.0f);
    state.sampler = nvDevice->createSampler(samplerDesc);

    auto* backend = device->GetBackend();
    nvrhi::IBindingLayout* bindlessLayout = backend ? backend->GetBindlessLayout() : nullptr;
    auto& cache = GetPassResourceCache();

    state.drawLayout = cache.GetOrCreateBindingLayoutFromReflection("SmokeTrail_Draw", *vsResult.reflection, *psResult.reflection, nvDevice);

    nvrhi::FramebufferInfo fbInfo;
    fbInfo.colorFormats.push_back(nvrhi::Format::RGBA16_FLOAT);
    fbInfo.depthFormat = nvrhi::Format::D32;

    nvrhi::GraphicsPipelineDesc pipeDesc;
    pipeDesc.VS = state.drawVS;  // Dedicated smoke trail VS (no subdivision)
    pipeDesc.PS = state.drawPS;
    pipeDesc.inputLayout = nullptr;
    pipeDesc.bindingLayouts.push_back(state.drawLayout);
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

    state.drawPipeline = cache.GetOrCreatePipeline("SmokeTrailDraw_blend", pipeDesc, fbInfo, nvDevice);

    if (state.drawPipeline)
    {
        const auto& actualDesc = state.drawPipeline->getDesc();
        if (!actualDesc.bindingLayouts.empty())
            state.drawLayout = actualDesc.bindingLayouts[0];

        Msg("* [SmokeTrail] Draw pipeline initialized (smoke_trail.vs + smoke_trail.ps)");
    }
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
    DefaultOutputLayout  outputs;
    u32 width  = 0;
    u32 height = 0;
    u32 noiseTextureIndex = 0;
    nvrhi::ITexture* perlin4dVolume = nullptr;
};

// ═══════════════════════════════════════════════════════
//  Setup function — 4 framegraph passes
// ═══════════════════════════════════════════════════════

DefaultOutputLayout setupSmokeTrailPass(
    FrameGraph&                      fg,
    ng::RenderDevice*                device,
    const DefaultOutputLayout&       inputs,
    SmokeTrailManager*               manager,
    u32                              width,
    u32                              height,
    SmokeTrailPassState&             state,
    nvrhi::ITexture*                 perlin4dVolume)
{
    InitSmokeComputePipelines(device, state);

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

            if (!st->emitPipeline)
                return;

            const auto& emitParams = mgr->GetEmitParams();
            if (emitParams.emitCount == 0)
                return;

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            nvrhi::IDevice* nvDevice = cmdList->getDevice();
            auto& cache = GetPassResourceCache();

            auto emitCB = cache.GetOrCreateVolatileCB(
                "SmokeTrail", "emit", sizeof(SmokeEmitParams), data.device);
            cmdList->writeBuffer(emitCB, &emitParams, sizeof(emitParams));

            auto* emitReflection = RImplementation.m_shaderLoader->GetCachedReflection("smoke_trail_emit", ".cs");
            BindingSetBuilder bsb(*emitReflection, nvDevice, "SmokeTrail.Emit");
            bsb.ConstantBuffer("SmokeEmitCB", emitCB)
               .BufferUAV("g_SimBuffer", mgr->GetSimBuffer())
               .BufferUAV("g_StateBuffer", mgr->GetStateBuffer());
            auto bindDesc = bsb.Build();
            auto bindSet = cache.GetOrCreateBindingSet(bindDesc, st->emitLayout, nvDevice);

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

            if (!st->simPipeline)
                return;

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            nvrhi::IDevice* nvDevice = cmdList->getDevice();
            auto& cache = GetPassResourceCache();

            const auto& simParams = mgr->GetSimParams();
            auto simCB = cache.GetOrCreateVolatileCB(
                "SmokeTrail", "sim", sizeof(SmokeSimParams), data.device);
            cmdList->writeBuffer(simCB, &simParams, sizeof(simParams));

            auto* simReflection = RImplementation.m_shaderLoader->GetCachedReflection("smoke_trail_simulate", ".cs");
            BindingSetBuilder bsb(*simReflection, nvDevice, "SmokeTrail.Sim");
            bsb.ConstantBuffer("SmokeSimCB", simCB)
               .BufferUAV("g_SimBuffer", mgr->GetSimBuffer());
            auto bindDesc = bsb.Build();
            auto bindSet = cache.GetOrCreateBindingSet(bindDesc, st->simLayout, nvDevice);

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

            if (!st->compactPipeline)
                return;

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            nvrhi::IDevice* nvDevice = cmdList->getDevice();
            auto& cache = GetPassResourceCache();

            const auto& compactParams = mgr->GetCompactParams();
            auto compactCB = cache.GetOrCreateVolatileCB(
                "SmokeTrail", "compact", sizeof(SmokeCompactParams), data.device);
            cmdList->writeBuffer(compactCB, &compactParams, sizeof(compactParams));

            auto* compactReflection = RImplementation.m_shaderLoader->GetCachedReflection("smoke_trail_compact", ".cs");
            BindingSetBuilder bsb(*compactReflection, nvDevice, "SmokeTrail.Compact");
            bsb.ConstantBuffer("SmokeCompactCB", compactCB)
               .BufferUAV("g_CompactBuffer", mgr->GetCompactBuffer())
               .BufferUAV("g_StateBuffer", mgr->GetStateBuffer())
               .BufferUAV("g_DrawArgs", mgr->GetDrawArgsBuffer())
               .BufferUAV("g_SimBuffer", mgr->GetSimBuffer());
            auto bindDesc = bsb.Build();
            auto bindSet = cache.GetOrCreateBindingSet(bindDesc, st->compactLayout, nvDevice);

            nvrhi::ComputeState cs;
            cs.pipeline = st->compactPipeline;
            cs.bindings = { bindSet };
            cmdList->setComputeState(cs);
            cmdList->dispatch(1, 1, 1);
        }
    );

    // ── Pass 4: Draw (smoke_trail.vs pipeline with drawIndirect) ──
    InitSmokeDrawPipeline(device, state);

    auto& passData = fg.addCallbackPass<SmokeDrawPassData>(
        "SmokeDraw",
        [&](FrameGraph& builder, PassHandle passHandle, SmokeDrawPassData& data)
        {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.device            = device;
            data.manager           = manager;
            data.smokeState        = &state;
            data.width             = width;
            data.height            = height;
            data.outputs           = inputs;
            data.perlin4dVolume    = perlin4dVolume;

            data.inputColor  = passBuilder.read(inputs.albedo);
            data.outputColor = passBuilder.write(inputs.albedo, ResourceState::RenderTarget);
            data.depth       = passBuilder.readWrite(inputs.depth, ResourceState::DepthStencilWrite);
        },
        [](const SmokeDrawPassData& data, const FrameGraph& fg, ng::RenderContext* ctx)
        {
            auto* mgr   = data.manager;
            auto* st    = data.smokeState;
            if (!mgr || !mgr->IsReady() || !st || !st->initialized)
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

            // Framebuffer
            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(colorRT);
            fbDesc.setDepthAttachment(depthRT);
            auto& cache = GetPassResourceCache();
            auto framebuffer = cache.GetOrCreateFramebuffer("SmokeDraw", fbDesc, nvDevice);
            if (!framebuffer)
                return;

            if (!st->drawPipeline)
                return;

            // Constant buffers (writeBuffer BEFORE setGraphicsState)
            auto staticGlobalsCB = cache.GetOrCreateVolatileCB(
                "Frame", "StaticGlobals", sizeof(StaticGlobals), data.device);
            auto trailParamsCB = cache.GetOrCreateVolatileCB(
                "SmokeDraw", "TrailParams", sizeof(TrailParamsCB), data.device);

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

            // Per-instance turbulence (same params as compact, applied per-instance in VS)
            const auto& compactParams = mgr->GetCompactParams();
            params.turbAmount     = compactParams.turbAmount;
            params.turbFrequency  = compactParams.turbFrequency;
            params.turbEvolution  = compactParams.turbEvolution;
            params.sphereCenterX  = compactParams.sphereCenterX;
            params.sphereCenterY  = compactParams.sphereCenterY;
            params.sphereCenterZ  = compactParams.sphereCenterZ;
            params.sphereRadius      = compactParams.sphereRadius;

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

            if (!data.perlin4dVolume)
                return;

            auto* shaderLoader = GEnv.Render->GetShaderLoader();
            auto* vsReflection = shaderLoader->GetCachedReflection("smoke_trail", ".vs");
            auto* psReflection = shaderLoader->GetCachedReflection("smoke_trail", ".ps");
            BindingSetBuilder bsb(*vsReflection, *psReflection, nvDevice, "SmokeTrail.Draw");
            bsb.ConstantBuffer("static_globals", staticGlobalsCB)
               .ConstantBuffer("TrailParams", trailParamsCB)
               .BufferSRV("g_ControlPoints", mgr->GetCompactBuffer())
               .BufferSRV("g_TrailState", mgr->GetStateBuffer())
               .Texture("g_Perlin4D", data.perlin4dVolume);
            auto bindDesc = bsb.Build();
            auto bindingSet = cache.GetOrCreateBindingSet(bindDesc, st->drawLayout, nvDevice);

            // Graphics state with indirect params
            nvrhi::GraphicsState gfxState;
            gfxState.pipeline     = st->drawPipeline;
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
