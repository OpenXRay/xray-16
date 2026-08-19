#include "stdafx.h"
#include "TransparentPassSetup.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/Backend/D3D12Backend.h"
#include "Layers/xrRender/Bindless/MaterialBuffer.h"
#include "Layers/xrRender/Bindless/VariantTextureBuffer.h"
#include "Layers/xrRender/ShaderVariant/VariantPSOCache.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "PassCommon.h"
#include "Layers/xrRender/ClusteredLightManager.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "Layers/xrRender/ResourceManager/TextureManager.h"
#include "xrEngine/Environment.h"
#include "xrEngine/IGame_Persistent.h"
#include "Layers/xrRender/xrRender_console.h"
#include "Layers/xrRender/fgEnvironmentRender.h"

namespace xray::render::fg::passes {

void InitializeTransparentResources(fg::RenderDevice* device, const nvrhi::FramebufferInfoEx& fbInfo, TransparentPassState& state)
{
    constexpr u32 kWaterVersion = 27;
    if (state.initialized && state.waterVersion == kWaterVersion)
        return;
    state.initialized = false;
    state.waterVersion = kWaterVersion;
    state.waterDistortPipeline = nullptr;
    state.waterDistortPipeDescValid = false;
    state.glassDistortPipeline = nullptr;
    state.glassDistortPipeDescValid = false;
    state.waterPipeline = nullptr;
    state.depthCopyPipeline = nullptr;
    state.depthCopyLayout = nullptr;
    framegraph::BindingSetBuilder::InvalidateReflectionCache();

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    if (!nvDevice)
        return;

    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    if (!shaderLoader)
        return;

    auto vsResult = shaderLoader->LoadVertexShader("bindless_forward", "main");
    auto psResult = shaderLoader->LoadPixelShader("bindless_forward", "main");
    if (!vsResult.handle || !psResult.handle)
        return;

    state.vs = vsResult.handle;
    state.ps = psResult.handle;

    auto& cache = framegraph::GetPassResourceCache();
    state.layout = cache.GetOrCreateBindingLayoutFromReflection("TransparentPass_v2_Water", *vsResult.reflection, *psResult.reflection, nvDevice);
    if (!state.layout)
        return;

    u32 attrCount = 0;
    auto* attrs = GetUnifiedVertexAttributes(attrCount);
    state.inputLayout = nvDevice->createInputLayout(attrs, attrCount, state.vs);

    auto drawIndexBuffer = GetOrCreateDrawIndexBuffer("TransparentPass", nvDevice);
    if (!drawIndexBuffer)
        return;

    nvrhi::GraphicsPipelineDesc pipeDesc;
    pipeDesc.VS = state.vs;
    pipeDesc.PS = state.ps;
    pipeDesc.inputLayout = state.inputLayout;

    auto* backend = device->GetBackend();
    nvrhi::IBindingLayout* bindlessLayout = backend ? backend->GetBindlessLayout() : nullptr;
    if (bindlessLayout)
        pipeDesc.bindingLayouts = { state.layout, bindlessLayout };
    else
        pipeDesc.bindingLayouts = { state.layout };

    pipeDesc.primType = nvrhi::PrimitiveType::TriangleList;
    pipeDesc.renderState.depthStencilState.depthTestEnable = true;
    pipeDesc.renderState.depthStencilState.depthWriteEnable = false;
    pipeDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::GreaterOrEqual;
    pipeDesc.renderState.rasterState.frontCounterClockwise = false;
            pipeDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::None;

    auto& rt0 = pipeDesc.renderState.blendState.targets[0];
    rt0.blendEnable = true;
    rt0.srcBlend = nvrhi::BlendFactor::SrcAlpha;
    rt0.destBlend = nvrhi::BlendFactor::InvSrcAlpha;
    rt0.blendOp = nvrhi::BlendOp::Add;
    rt0.srcBlendAlpha = nvrhi::BlendFactor::One;
    rt0.destBlendAlpha = nvrhi::BlendFactor::InvSrcAlpha;
    rt0.blendOpAlpha = nvrhi::BlendOp::Add;
    for (u32 rt = 1; rt < 4; ++rt)
        pipeDesc.renderState.blendState.targets[rt].setColorWriteMask(nvrhi::ColorMask(0));

    state.pipeline = cache.GetOrCreatePipeline("TransparentPass_v2_Water", pipeDesc, fbInfo, nvDevice);
    if (!state.pipeline)
        return;

    QueryBindingLayoutFromPipeline(state.pipeline, state.layout);

    {
        auto waterVs = shaderLoader->LoadVertexShader("water", "main");
        auto waterPs = shaderLoader->LoadPixelShader("water", "main");
        if (waterVs.handle && waterPs.handle && waterVs.reflection && waterPs.reflection)
        {
            state.waterVs = waterVs.handle;
            state.waterPs = waterPs.handle;
            state.waterLayout = cache.GetOrCreateBindingLayoutFromReflection(
                "TransparentWater_v45_NoSSR", *waterVs.reflection, *waterPs.reflection, nvDevice);
            if (state.waterLayout)
            {
                nvrhi::GraphicsPipelineDesc waterDesc = pipeDesc;
                waterDesc.VS = state.waterVs;
                waterDesc.PS = state.waterPs;
                waterDesc.inputLayout = nvDevice->createInputLayout(attrs, attrCount, state.waterVs);
                if (bindlessLayout)
                    waterDesc.bindingLayouts = {state.waterLayout, bindlessLayout};
                else
                    waterDesc.bindingLayouts = {state.waterLayout};
                waterDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::None;
                waterDesc.renderState.depthStencilState.depthTestEnable = true;
                waterDesc.renderState.depthStencilState.depthWriteEnable = false;
                waterDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::GreaterOrEqual;
                waterDesc.renderState.rasterState.depthBias = 0;
                waterDesc.renderState.rasterState.slopeScaledDepthBias = 0.f;
                auto& wrt0 = waterDesc.renderState.blendState.targets[0];
                wrt0.blendEnable = true;
                wrt0.srcBlend = nvrhi::BlendFactor::SrcAlpha;
                wrt0.destBlend = nvrhi::BlendFactor::InvSrcAlpha;
                wrt0.blendOp = nvrhi::BlendOp::Add;
                wrt0.srcBlendAlpha = nvrhi::BlendFactor::One;
                wrt0.destBlendAlpha = nvrhi::BlendFactor::InvSrcAlpha;
                wrt0.blendOpAlpha = nvrhi::BlendOp::Add;
                for (u32 rt = 1; rt < 4; ++rt)
                {
                    waterDesc.renderState.blendState.targets[rt].blendEnable = false;
                    waterDesc.renderState.blendState.targets[rt].setColorWriteMask(
                        nvrhi::ColorMask::Red | nvrhi::ColorMask::Green |
                        nvrhi::ColorMask::Blue | nvrhi::ColorMask::Alpha);
                }
                state.waterPipeline = cache.GetOrCreatePipeline("TransparentWater_v45_NoSSR", waterDesc, fbInfo, nvDevice);
            }
            waterVs.reflection = nullptr;
            waterPs.reflection = nullptr;
        }
        if (!state.waterPipeline)
            Msg("! [TransparentPass] Water PSO unavailable — water clipped from forward");
    }

    {
        auto waterdVs = shaderLoader->LoadVertexShader("waterd", "main");
        auto waterdPs = shaderLoader->LoadPixelShader("waterd", "main");
        if (waterdVs.handle && waterdPs.handle && waterdVs.reflection && waterdPs.reflection)
        {
            state.waterDistortVs = waterdVs.handle;
            state.waterDistortPs = waterdPs.handle;
            state.waterDistortLayout = cache.GetOrCreateBindingLayoutFromReflection(
                "TransparentWaterDistort_v23_R3Fmt", *waterdVs.reflection, *waterdPs.reflection, nvDevice);
            if (state.waterDistortLayout)
            {
                state.waterDistortInputLayout =
                    nvDevice->createInputLayout(attrs, attrCount, state.waterDistortVs);
                nvrhi::GraphicsPipelineDesc distortDesc;
                distortDesc.VS = state.waterDistortVs;
                distortDesc.PS = state.waterDistortPs;
                distortDesc.inputLayout = state.waterDistortInputLayout;
                if (bindlessLayout)
                    distortDesc.bindingLayouts = {state.waterDistortLayout, bindlessLayout};
                else
                    distortDesc.bindingLayouts = {state.waterDistortLayout};
                distortDesc.primType = nvrhi::PrimitiveType::TriangleList;
                distortDesc.renderState.depthStencilState.depthTestEnable = true;
                distortDesc.renderState.depthStencilState.depthWriteEnable = false;
                distortDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::GreaterOrEqual;
                distortDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::None;
                auto& drt = distortDesc.renderState.blendState.targets[0];
                drt.blendEnable = false;
                state.waterDistortPipeDesc = distortDesc;
                state.waterDistortPipeDescValid = true;
                state.waterDistortPipeline = nullptr;
            }
            waterdVs.reflection = nullptr;
            waterdPs.reflection = nullptr;
        }
        if (!state.waterDistortPipeDescValid)
            Msg("! [TransparentPass] Water distort PSO unavailable");
    }

    {
        auto glassPs = shaderLoader->LoadPixelShader("glass_distort", "main");
        if (glassPs.handle && glassPs.reflection && vsResult.reflection)
        {
            state.glassDistortPs = glassPs.handle;
            state.glassDistortLayout = cache.GetOrCreateBindingLayoutFromReflection(
                "TransparentGlassDistort_v1", *vsResult.reflection, *glassPs.reflection, nvDevice);
            if (state.glassDistortLayout)
            {
                state.glassDistortInputLayout = nvDevice->createInputLayout(attrs, attrCount, state.vs);
                nvrhi::GraphicsPipelineDesc distortDesc;
                distortDesc.VS = state.vs;
                distortDesc.PS = state.glassDistortPs;
                distortDesc.inputLayout = state.glassDistortInputLayout;
                if (bindlessLayout)
                    distortDesc.bindingLayouts = {state.glassDistortLayout, bindlessLayout};
                else
                    distortDesc.bindingLayouts = {state.glassDistortLayout};
                distortDesc.primType = nvrhi::PrimitiveType::TriangleList;
                distortDesc.renderState.depthStencilState.depthTestEnable = true;
                distortDesc.renderState.depthStencilState.depthWriteEnable = false;
                distortDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::GreaterOrEqual;
                distortDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::None;
                auto& drt = distortDesc.renderState.blendState.targets[0];
                drt.blendEnable = true;
                drt.srcBlend = nvrhi::BlendFactor::SrcAlpha;
                drt.destBlend = nvrhi::BlendFactor::InvSrcAlpha;
                state.glassDistortPipeDesc = distortDesc;
                state.glassDistortPipeDescValid = true;
                state.glassDistortPipeline = nullptr;
            }
            glassPs.reflection = nullptr;
        }
    }

    {
        auto csResult = shaderLoader->LoadComputeShader("copy_depth_r32");
        if (csResult.handle && csResult.reflection)
        {
            state.depthCopyLayout = cache.GetOrCreateBindingLayoutFromReflection(
                "TransparentWater_DepthCopy", *csResult.reflection, nvDevice);
            if (state.depthCopyLayout)
            {
                nvrhi::ComputePipelineDesc cpd;
                cpd.CS = csResult.handle;
                cpd.bindingLayouts = {state.depthCopyLayout};
                state.depthCopyPipeline = cache.GetOrCreateComputePipeline(
                    "TransparentWater_DepthCopy", cpd, nvDevice);
            }
            state.depthCopyCB = cache.GetOrCreateVolatileCB(
                "TransparentWater", "CopyDepthParams", 16, device);
            csResult.reflection = nullptr;
        }
        if (!state.depthCopyPipeline)
            Msg("! [TransparentPass] Depth copy for soft water unavailable");
    }

    if (auto* resMgr = device->GetFGResourceManager())
    {
        if (auto* texMgr = resMgr->GetTextureManager())
        {
            auto foam = texMgr->LoadTexture("water" DELIMITER "water_foam");
            state.foamTexture = texMgr->GetNVRHITexture(foam);
        }
    }

    state.initialized = true;
    Msg("* [TransparentPass] Pipeline initialized (water=%d waterd=%d depthCopy=%d)",
        state.waterPipeline ? 1 : 0, state.waterDistortPipeDescValid ? 1 : 0,
        state.depthCopyPipeline ? 1 : 0);
}

framegraph::DefaultOutputLayout setupTransparentPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    const framegraph::DefaultOutputLayout& inputs,
    const TransparentPassConfig& config,
    u32 width, u32 height,
    TransparentPassState& state)
{
    using namespace framegraph;

    if (!config.IsValid()) {
        return inputs;
    }

    nvrhi::FramebufferInfoEx fbInfo;
    fbInfo.colorFormats.push_back(nvrhi::Format::RGBA16_FLOAT);
    fbInfo.colorFormats.push_back(nvrhi::Format::RGBA16_FLOAT);
    fbInfo.colorFormats.push_back(nvrhi::Format::RGBA8_UNORM);
    fbInfo.colorFormats.push_back(nvrhi::Format::RGBA32_FLOAT);
    fbInfo.depthFormat = nvrhi::Format::D32;
    InitializeTransparentResources(device, fbInfo, state);

    auto& passData = fg.addCallbackPass<TransparentPassData>(
        "Transparent Pass",

        [&, width, height, config](FrameGraph& builder, PassHandle passHandle, TransparentPassData& data) {
            data.width = width;
            data.height = height;
            data.device = device;
            data.config = config;
            data.passState = &state;

            RenderPassBuilder passBuilder(builder, passHandle);
            data.color = passBuilder.readWrite(inputs.albedo, ResourceState::RenderTarget);
            data.normal = passBuilder.readWrite(inputs.normal, ResourceState::RenderTarget);
            data.depth = passBuilder.read(inputs.depth, ResourceState::DepthStencilRead);
            if (inputs.baseColor.is_valid())
                data.baseColor = passBuilder.readWrite(inputs.baseColor, ResourceState::RenderTarget);

            if (inputs.worldPos.is_valid())
            {
                data.worldPos = passBuilder.readWrite(inputs.worldPos, ResourceState::RenderTarget);
                data.clearWorldPos = false;
            }
            else
            {
                ResourceDesc wpDesc;
                wpDesc.type = ResourceDesc::Type::Texture2D;
                wpDesc.width = width;
                wpDesc.height = height;
                wpDesc.format = nvrhi::Format::RGBA32_FLOAT;
                wpDesc.isRenderTarget = true;
                wpDesc.isTransient = true;
                wpDesc.debugName = "rt_TransparentWorldPos";
                data.worldPos = passBuilder.createTexture("rt_TransparentWorldPos", wpDesc);
                data.clearWorldPos = true;
            }

            if (state.waterDistortPipeDescValid || state.glassDistortPipeDescValid)
            {
                ResourceDesc distDesc;
                distDesc.type = ResourceDesc::Type::Texture2D;
                distDesc.width = width;
                distDesc.height = height;
                distDesc.format = nvrhi::Format::RGBA16_FLOAT;
                distDesc.isRenderTarget = true;
                distDesc.isTransient = true;
                distDesc.debugName = "rt_Distortion";
                data.distortion = passBuilder.createTexture("rt_Distortion", distDesc);
            }
        },

        [](const TransparentPassData& data,
            const FrameGraph& fg,
            fg::RenderContext* ctx) {

            auto* colorRT = fg.GetPhysicalTexture(data.color);
            auto* normalRT = fg.GetPhysicalTexture(data.normal);
            auto* depthRT = fg.GetPhysicalTexture(data.depth);
            if (!colorRT || !depthRT)
                return;

            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            if (!nvDevice || !cmdList)
                return;

            auto* baseColorRT = data.baseColor.is_valid() ? fg.GetPhysicalTexture(data.baseColor) : nullptr;
            auto* worldPosRT = data.worldPos.is_valid() ? fg.GetPhysicalTexture(data.worldPos) : nullptr;
            if (!normalRT || !baseColorRT || !worldPosRT)
                return;

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(colorRT);
            fbDesc.addColorAttachment(normalRT);
            fbDesc.addColorAttachment(baseColorRT);
            fbDesc.addColorAttachment(worldPosRT);
            fbDesc.setDepthAttachment(depthRT);
            auto& cache = framegraph::GetPassResourceCache();
            auto framebuffer = cache.GetOrCreateFramebuffer("TransparentPass", fbDesc, nvDevice);
            if (!framebuffer)
                return;

            if (!data.passState->initialized || !data.passState->pipeline)
                return;

            if (data.distortion.is_valid())
            {
                if (auto* distortRT = fg.GetPhysicalTexture(data.distortion))
                    cmdList->clearTextureFloat(
                        distortRT, nvrhi::AllSubresources, nvrhi::Color(0.5f, 0.5f, 0.f, 0.f));
            }

            if (data.clearWorldPos)
            {
                cmdList->clearTextureFloat(
                    worldPosRT, nvrhi::AllSubresources, nvrhi::Color(0.f, 0.f, 0.f, 0.f));
            }

            using namespace fg::bindless;
            auto& matBuffer = MaterialBuffer::Instance();

            auto lightingCB = cache.GetOrCreateVolatileCB("TransparentPass", "LightingCB", sizeof(LightingConstants), data.device);
            auto staticGlobalsCB = cache.GetOrCreateVolatileCB("Frame", "StaticGlobals", sizeof(StaticGlobals), data.device);
            auto drawIndexBuffer = GetOrCreateDrawIndexBuffer("TransparentPass", nvDevice);

            {
                StaticGlobals sg = BuildStaticGlobals();
                cmdList->writeBuffer(staticGlobalsCB, &sg, sizeof(sg));
            }

            auto lightingData = FillLightingConstants();
            cmdList->writeBuffer(lightingCB, &lightingData, sizeof(lightingData));

            const auto& cfg = data.config;
            auto& passCache = framegraph::GetPassResourceCache();
            auto& clm = ClusteredLightManager::Instance();

            nvrhi::ITexture* sky0 = cfg.envSky0
                ? cfg.envSky0
                : passCache.GetDummyCubeMap(nvDevice);
            nvrhi::ITexture* sky1 = cfg.envSky1
                ? cfg.envSky1
                : passCache.GetDummyCubeMap(nvDevice);

            auto* shaderLoader = GEnv.Render->GetShaderLoader();
            auto* vsReflection = shaderLoader->GetCachedReflection("bindless_forward", ".vs");
            auto* psReflection = shaderLoader->GetCachedReflection("bindless_forward", ".ps");
            if (!vsReflection || !psReflection)
                return;

            auto* backend = data.device->GetBackend();
            nvrhi::IBindingSet* bindlessTable = nullptr;
            if (backend)
                bindlessTable = backend->GetBindlessDescriptorTable();

            framegraph::BindingSetBuilder bsb(*vsReflection, *psReflection, nvDevice, "Transparent");
            bsb.ConstantBuffer("static_globals", staticGlobalsCB);
            BindBindlessMaterialTables(bsb);
            BindEnvIblCubes(bsb, data.device);
            bsb.BufferSRV("g_InstanceData", cfg.instanceBuffer);
            bsb.BufferSRV("g_CompactBatchIndices", cfg.compactBatchIndicesBuffer);
            bsb.BufferSRV("g_CompactMaterialIDs", cfg.compactMaterialIDBuffer);
            bsb.BufferSRV("g_LightData", clm.GetLightDataBuffer());
            bsb.BufferSRV("g_ClusterGrid", clm.GetClusterGridBuffer());
            bsb.BufferSRV("g_LightIndexList", clm.GetLightIndexListBuffer());

            auto transparentBindDesc = bsb.Build();
            auto bindingSet = passCache.GetOrCreateBindingSet(transparentBindDesc, data.passState->layout, nvDevice);
            if (!bindingSet)
                return;

            nvrhi::GraphicsState gfxState;
            gfxState.pipeline = data.passState->pipeline;
            gfxState.framebuffer = framebuffer;
            gfxState.bindings = { bindingSet };
            if (bindlessTable)
                gfxState.addBindingSet(bindlessTable);

            gfxState.vertexBuffers = {
                {cfg.megaVertexBuffer, 0, 0},
                {drawIndexBuffer, 1, 0}
            };
            gfxState.indexBuffer = { cfg.megaIndexBuffer, nvrhi::Format::R32_UINT, 0 };
            gfxState.indirectParams = cfg.compactDrawArgsBuffer;
            gfxState.indirectCountBuffer = cfg.compactCountBuffer;

            const auto& rtDesc = colorRT->getDesc();
            nvrhi::Viewport viewport(0.0f, static_cast<float>(rtDesc.width), 0.0f, static_cast<float>(rtDesc.height), 0.0f, 1.0f);
            gfxState.viewport.addViewport(viewport);
            gfxState.viewport.addScissorRect(nvrhi::Rect(rtDesc.width, rtDesc.height));

            if (cfg.variantPartition.Enabled()) {
                auto* backendDev = data.device->GetBackend();

                VariantPartitionDrawConfig vpCfg;
                vpCfg.defaultPipeline = data.passState->pipeline.Get();
                vpCfg.inputLayout = data.passState->inputLayout;
                vpCfg.passLayout = data.passState->layout;
                vpCfg.bindlessLayout = backendDev ? backendDev->GetBindlessLayout() : nullptr;
                vpCfg.bindlessTable = bindlessTable;
                vpCfg.megaVertexBuffer = cfg.megaVertexBuffer;
                vpCfg.baseBindings = transparentBindDesc;
                vpCfg.objectCount = cfg.objectCount;
                vpCfg.partition = cfg.variantPartition;
                vpCfg.selectTransparent = true;
                vpCfg.skipWmark = cfg.skipWmark;

                DrawVariantPartition(cmdList, nvDevice, framebuffer, gfxState, vpCfg);
            } else {
                cmdList->setGraphicsState(gfxState);
                DrawIndexedIndirectCountOrFallback(cmdList, 0, 0, cfg.objectCount);
            }

            if (data.passState->waterPipeline && data.passState->waterLayout)
            {
                auto* waterVsRefl = shaderLoader->GetCachedReflection("water", ".vs");
                auto* waterPsRefl = shaderLoader->GetCachedReflection("water", ".ps");
                if (waterVsRefl && waterPsRefl)
                {
                    const auto& cdesc = colorRT->getDesc();
                    const auto& ddesc = depthRT->getDesc();
                    if (!data.passState->waterSsrColor ||
                        data.passState->waterSsrColor->getDesc().width != cdesc.width ||
                        data.passState->waterSsrColor->getDesc().height != cdesc.height ||
                        data.passState->waterSsrColor->getDesc().format != cdesc.format)
                    {
                        nvrhi::TextureDesc td{};
                        td.width = cdesc.width;
                        td.height = cdesc.height;
                        td.format = cdesc.format;
                        td.mipLevels = 1;
                        td.arraySize = 1;
                        td.sampleCount = 1;
                        td.dimension = nvrhi::TextureDimension::Texture2D;
                        td.debugName = "WaterSSR_Color";
                        td.isShaderResource = true;
                        td.initialState = nvrhi::ResourceStates::ShaderResource;
                        td.keepInitialState = true;
                        data.passState->waterSsrColor = nvDevice->createTexture(td);
                    }
                    if (!data.passState->waterSceneDepth ||
                        data.passState->waterSceneDepth->getDesc().width != ddesc.width ||
                        data.passState->waterSceneDepth->getDesc().height != ddesc.height ||
                        data.passState->waterSceneDepth->getDesc().format != nvrhi::Format::R32_FLOAT)
                    {
                        nvrhi::TextureDesc td{};
                        td.width = ddesc.width;
                        td.height = ddesc.height;
                        td.format = nvrhi::Format::R32_FLOAT;
                        td.mipLevels = 1;
                        td.arraySize = 1;
                        td.sampleCount = 1;
                        td.dimension = nvrhi::TextureDimension::Texture2D;
                        td.debugName = "WaterSoft_DepthR32";
                        td.isShaderResource = true;
                        td.isUAV = true;
                        td.initialState = nvrhi::ResourceStates::UnorderedAccess;
                        td.keepInitialState = true;
                        data.passState->waterSceneDepth = nvDevice->createTexture(td);
                    }
                    nvrhi::ITexture* ssrColor = data.passState->waterSsrColor;
                    nvrhi::ITexture* softDepthR32 = data.passState->waterSceneDepth;
                    nvrhi::ITexture* underWP = passCache.GetDummyContactHistory(nvDevice);
                    cmdList->clearState();
                    if (ssrColor)
                    {
                        cmdList->setTextureState(colorRT, nvrhi::AllSubresources, nvrhi::ResourceStates::CopySource);
                        cmdList->setTextureState(ssrColor, nvrhi::AllSubresources, nvrhi::ResourceStates::CopyDest);
                        cmdList->copyTexture(ssrColor, nvrhi::TextureSlice(), colorRT, nvrhi::TextureSlice());
                    }
                    if (softDepthR32 && data.passState->depthCopyPipeline && data.passState->depthCopyLayout &&
                        data.passState->depthCopyCB)
                    {
                        cmdList->setTextureState(depthRT, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
                        cmdList->setTextureState(softDepthR32, nvrhi::AllSubresources, nvrhi::ResourceStates::UnorderedAccess);
                        struct CopyDepthCB { u32 w, h, p0, p1; } cb{};
                        cb.w = ddesc.width;
                        cb.h = ddesc.height;
                        cmdList->writeBuffer(data.passState->depthCopyCB, &cb, sizeof(cb));
                        auto* csRefl = shaderLoader->GetCachedReflection("copy_depth_r32", ".cs");
                        if (csRefl)
                        {
                            framegraph::BindingSetBuilder dbsb(*csRefl, nvDevice, "Transparent.DepthCopy");
                            dbsb.ConstantBuffer("CopyDepthParams", data.passState->depthCopyCB);
                            dbsb.Texture("t_Depth", depthRT);
                            dbsb.TextureUAV("u_DepthR32", softDepthR32);
                            auto set = passCache.GetOrCreateBindingSet(
                                dbsb.Build(), data.passState->depthCopyLayout, nvDevice);
                            if (set)
                            {
                                nvrhi::ComputeState cs;
                                cs.pipeline = data.passState->depthCopyPipeline;
                                cs.bindings = {set};
                                cmdList->setComputeState(cs);
                                cmdList->dispatch((ddesc.width + 7) / 8, (ddesc.height + 7) / 8, 1);
                            }
                        }
                        cmdList->setTextureState(softDepthR32, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
                    }
                    if (worldPosRT)
                    {
                        const auto& wpDesc = worldPosRT->getDesc();
                        if (!data.passState->waterSceneWorldPos ||
                            data.passState->waterSceneWorldPos->getDesc().width != wpDesc.width ||
                            data.passState->waterSceneWorldPos->getDesc().height != wpDesc.height)
                        {
                            nvrhi::TextureDesc td{};
                            td.width = wpDesc.width;
                            td.height = wpDesc.height;
                            td.format = wpDesc.format;
                            td.mipLevels = 1;
                            td.arraySize = 1;
                            td.sampleCount = 1;
                            td.dimension = nvrhi::TextureDimension::Texture2D;
                            td.debugName = "WaterSSR_WorldPos";
                            td.isShaderResource = true;
                            td.initialState = nvrhi::ResourceStates::ShaderResource;
                            td.keepInitialState = true;
                            data.passState->waterSceneWorldPos = nvDevice->createTexture(td);
                        }
                        if (data.passState->waterSceneWorldPos)
                        {
                            underWP = data.passState->waterSceneWorldPos;
                            cmdList->setTextureState(worldPosRT, nvrhi::AllSubresources, nvrhi::ResourceStates::CopySource);
                            cmdList->setTextureState(underWP, nvrhi::AllSubresources, nvrhi::ResourceStates::CopyDest);
                            cmdList->copyTexture(underWP, nvrhi::TextureSlice(), worldPosRT, nvrhi::TextureSlice());
                            cmdList->setTextureState(underWP, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
                        }
                    }
                    if (ssrColor)
                        cmdList->setTextureState(ssrColor, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);

                    cmdList->setTextureState(colorRT, nvrhi::AllSubresources, nvrhi::ResourceStates::RenderTarget);
                    cmdList->setTextureState(depthRT, nvrhi::AllSubresources, nvrhi::ResourceStates::DepthRead);
                    if (normalRT)
                        cmdList->setTextureState(normalRT, nvrhi::AllSubresources, nvrhi::ResourceStates::RenderTarget);
                    if (baseColorRT)
                        cmdList->setTextureState(baseColorRT, nvrhi::AllSubresources, nvrhi::ResourceStates::RenderTarget);
                    if (worldPosRT)
                        cmdList->setTextureState(
                            worldPosRT, nvrhi::AllSubresources, nvrhi::ResourceStates::RenderTarget);

                    auto waterCB = cache.GetOrCreateVolatileCB(
                        "TransparentWater", "WaterParams", sizeof(Fvector4), data.device);
                    Fvector4 wi{};
                    float intens = 1.f;
                    if (g_pGamePersistent)
                        intens = g_pGamePersistent->Environment().CurrentEnv.m_fWaterIntensity;
                    intens = std::clamp(intens, 0.f, 1.f);
                    const float softOn = 1.f;
                    wi.set(intens, intens, intens, softOn);
                    cmdList->writeBuffer(waterCB, &wi, sizeof(wi));

                    nvrhi::ITexture* foam = data.passState->foamTexture;
                    if (!foam)
                        foam = passCache.GetDummyShadowMap2D(nvDevice);

                    nvrhi::ITexture* ssrColorBind = ssrColor
                        ? ssrColor
                        : passCache.GetDummyContactHistory(nvDevice);
                    nvrhi::ITexture* sceneDepthBind = softDepthR32
                        ? softDepthR32
                        : passCache.GetDummyContactDepth(nvDevice);
                    if (!underWP)
                        underWP = passCache.GetDummyContactHistory(nvDevice);

                    framegraph::BindingSetBuilder wbsb(*waterVsRefl, *waterPsRefl, nvDevice, "Transparent.Water");
                    wbsb.ConstantBuffer("static_globals", staticGlobalsCB)
                        .ConstantBuffer("WaterParams", waterCB);
                    BindBindlessMaterialTables(wbsb);
                    wbsb.BufferSRV("g_InstanceData", cfg.instanceBuffer)
                        .BufferSRV("g_CompactBatchIndices", cfg.compactBatchIndicesBuffer)
                        .BufferSRV("g_CompactMaterialIDs", cfg.compactMaterialIDBuffer);
                    wbsb.Texture("s_env0", sky0);
                    wbsb.Texture("s_env1", sky1);
                    wbsb.Texture("s_leaves", foam);
                    wbsb.TextureSlot(20, ssrColorBind);
                    wbsb.TextureSlot(21, sceneDepthBind);
                    wbsb.TextureSlot(23, underWP);

                    auto waterSet = passCache.GetOrCreateBindingSet(
                        wbsb.Build(), data.passState->waterLayout, nvDevice);
                    if (waterSet)
                    {
                        nvrhi::GraphicsState waterGfx;
                        waterGfx.pipeline = data.passState->waterPipeline;
                        waterGfx.framebuffer = framebuffer;
                        waterGfx.bindings = {waterSet};
                        if (bindlessTable)
                            waterGfx.addBindingSet(bindlessTable);
                        waterGfx.vertexBuffers = {
                            {cfg.megaVertexBuffer, 0, 0},
                            {drawIndexBuffer, 1, 0}
                        };
                        waterGfx.indexBuffer = { cfg.megaIndexBuffer, nvrhi::Format::R32_UINT, 0 };
                        waterGfx.indirectParams = cfg.compactDrawArgsBuffer;
                        waterGfx.indirectCountBuffer = cfg.compactCountBuffer;
                        waterGfx.viewport.addViewport(viewport);
                        waterGfx.viewport.addScissorRect(nvrhi::Rect(rtDesc.width, rtDesc.height));
                        cmdList->setGraphicsState(waterGfx);
                        DrawIndexedIndirectCountOrFallback(cmdList, 0, 0, cfg.objectCount);
                    }

                    if (data.passState->waterDistortPipeDescValid && data.passState->waterDistortLayout &&
                        data.distortion.is_valid())
                    {
                        auto* distortRT = fg.GetPhysicalTexture(data.distortion);
                        auto* waterdVsRefl = shaderLoader->GetCachedReflection("waterd", ".vs");
                        auto* waterdPsRefl = shaderLoader->GetCachedReflection("waterd", ".ps");
                        if (distortRT && waterdVsRefl && waterdPsRefl)
                        {
                            nvrhi::FramebufferDesc distortFbDesc;
                            distortFbDesc.addColorAttachment(distortRT);
                            distortFbDesc.setDepthAttachment(depthRT);
                            auto distortFB = passCache.GetOrCreateFramebuffer(
                                "TransparentWaterDistort_Depth", distortFbDesc, nvDevice);
                            if (distortFB)
                            {
                                const auto& distortFbi = distortFB->getFramebufferInfo();
                                if (!data.passState->waterDistortPipeline ||
                                    data.passState->waterDistortPipeline->getFramebufferInfo() != distortFbi)
                                {
                                    data.passState->waterDistortPipeline = passCache.GetOrCreatePipeline(
                                        "TransparentWaterDistort_v23_R3Fmt",
                                        data.passState->waterDistortPipeDesc,
                                        distortFbi,
                                        nvDevice);
                                }
                                if (!data.passState->waterDistortPipeline)
                                    Msg("! [TransparentPass] Water distort PSO create failed");

                                framegraph::BindingSetBuilder dbsb(
                                    *waterdVsRefl, *waterdPsRefl, nvDevice, "Transparent.WaterDistort");
                                dbsb.ConstantBuffer("static_globals", staticGlobalsCB)
                                    .ConstantBuffer("WaterParams", waterCB);
                                BindBindlessMaterialTables(dbsb);
                                dbsb.BufferSRV("g_InstanceData", cfg.instanceBuffer)
                                    .BufferSRV("g_CompactBatchIndices", cfg.compactBatchIndicesBuffer)
                                    .BufferSRV("g_CompactMaterialIDs", cfg.compactMaterialIDBuffer);
                                dbsb.TextureSlot(21, sceneDepthBind);
                                dbsb.TextureSlot(23, underWP);

                                auto distortSet = passCache.GetOrCreateBindingSet(
                                    dbsb.Build(), data.passState->waterDistortLayout, nvDevice);
                                if (distortSet && data.passState->waterDistortPipeline)
                                {
                                    nvrhi::GraphicsState dgfx;
                                    dgfx.pipeline = data.passState->waterDistortPipeline;
                                    dgfx.framebuffer = distortFB;
                                    dgfx.bindings = {distortSet};
                                    if (bindlessTable)
                                        dgfx.addBindingSet(bindlessTable);
                                    dgfx.vertexBuffers = {
                                        {cfg.megaVertexBuffer, 0, 0},
                                        {drawIndexBuffer, 1, 0}
                                    };
                                    dgfx.indexBuffer = {
                                        cfg.megaIndexBuffer, nvrhi::Format::R32_UINT, 0};
                                    dgfx.indirectParams = cfg.compactDrawArgsBuffer;
                                    dgfx.indirectCountBuffer = cfg.compactCountBuffer;
                                    dgfx.viewport.addViewport(viewport);
                                    dgfx.viewport.addScissorRect(
                                        nvrhi::Rect(rtDesc.width, rtDesc.height));
                                    cmdList->setGraphicsState(dgfx);
                                    DrawIndexedIndirectCountOrFallback(
                                        cmdList, 0, 0, cfg.objectCount);
                                }
                            }
                        }
                    }
                }

                if (data.passState->glassDistortPipeDescValid && data.passState->glassDistortLayout &&
                    data.distortion.is_valid() && cfg.IsValid())
                {
                    auto* distortRT = fg.GetPhysicalTexture(data.distortion);
                    auto* glassPsRefl = shaderLoader->GetCachedReflection("glass_distort", ".ps");
                    if (distortRT && vsReflection && glassPsRefl)
                    {
                        nvrhi::FramebufferDesc distortFbDesc;
                        distortFbDesc.addColorAttachment(distortRT);
                        distortFbDesc.setDepthAttachment(depthRT);
                        auto distortFB = passCache.GetOrCreateFramebuffer(
                            "TransparentGlassDistort", distortFbDesc, nvDevice);
                        if (distortFB)
                        {
                            const auto& distortFbi = distortFB->getFramebufferInfo();
                            if (!data.passState->glassDistortPipeline ||
                                data.passState->glassDistortPipeline->getFramebufferInfo() != distortFbi)
                            {
                                data.passState->glassDistortPipeline = passCache.GetOrCreatePipeline(
                                    "TransparentGlassDistort_v1",
                                    data.passState->glassDistortPipeDesc,
                                    distortFbi,
                                    nvDevice);
                            }
                            framegraph::BindingSetBuilder gbsb(
                                *vsReflection, *glassPsRefl, nvDevice, "Transparent.GlassDistort");
                            gbsb.ConstantBuffer("static_globals", staticGlobalsCB);
                            BindBindlessMaterialTables(gbsb);
                            gbsb.BufferSRV("g_InstanceData", cfg.instanceBuffer)
                                .BufferSRV("g_CompactBatchIndices", cfg.compactBatchIndicesBuffer)
                                .BufferSRV("g_CompactMaterialIDs", cfg.compactMaterialIDBuffer);
                            auto glassSet = passCache.GetOrCreateBindingSet(
                                gbsb.Build(), data.passState->glassDistortLayout, nvDevice);
                            if (glassSet && data.passState->glassDistortPipeline)
                            {
                                nvrhi::GraphicsState dgfx;
                                dgfx.pipeline = data.passState->glassDistortPipeline;
                                dgfx.framebuffer = distortFB;
                                dgfx.bindings = {glassSet};
                                if (bindlessTable)
                                    dgfx.addBindingSet(bindlessTable);
                                dgfx.vertexBuffers = {
                                    {cfg.megaVertexBuffer, 0, 0},
                                    {drawIndexBuffer, 1, 0}
                                };
                                dgfx.indexBuffer = {cfg.megaIndexBuffer, nvrhi::Format::R32_UINT, 0};
                                dgfx.indirectParams = cfg.compactDrawArgsBuffer;
                                dgfx.indirectCountBuffer = cfg.compactCountBuffer;
                                dgfx.viewport.addViewport(viewport);
                                dgfx.viewport.addScissorRect(nvrhi::Rect(rtDesc.width, rtDesc.height));
                                cmdList->setGraphicsState(dgfx);
                                DrawIndexedIndirectCountOrFallback(cmdList, 0, 0, cfg.objectCount);
                            }
                        }
                    }
                }

            }
        }
    );

    DefaultOutputLayout outputs;
    outputs.albedo = passData.color;
    outputs.normal = passData.normal;
    outputs.baseColor = passData.baseColor;
    outputs.worldPos = passData.worldPos;
    outputs.depth = passData.depth;
    outputs.distortion = passData.distortion;
    return outputs;
}

framegraph::DefaultOutputLayout setupWallmarkPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    const framegraph::DefaultOutputLayout& inputs,
    const TransparentPassConfig& config,
    u32 width, u32 height,
    TransparentPassState& state)
{
    using namespace framegraph;

    if (!config.IsValid() || !config.variantPartition.Enabled())
        return inputs;

    nvrhi::FramebufferInfoEx fbInfo;
    fbInfo.colorFormats.push_back(nvrhi::Format::RGBA16_FLOAT);
    fbInfo.depthFormat = nvrhi::Format::D32;
    InitializeTransparentResources(device, fbInfo, state);

    struct WallmarkPassData {
        VirtualResourceHandle color;
        VirtualResourceHandle depth;
        fg::RenderDevice* device = nullptr;
        TransparentPassConfig config;
        TransparentPassState* passState = nullptr;
    };

    auto& passData = fg.addCallbackPass<WallmarkPassData>(
        "Wallmarks",
        [&, config](FrameGraph& builder, PassHandle passHandle, WallmarkPassData& data) {
            data.device = device;
            data.config = config;
            data.passState = &state;
            RenderPassBuilder pb(builder, passHandle);
            data.color = pb.readWrite(inputs.albedo, ResourceState::RenderTarget);
            data.depth = pb.read(inputs.depth, ResourceState::DepthStencilRead);
        },
        [](const WallmarkPassData& data, const FrameGraph& fgGraph, fg::RenderContext* ctx) {
            auto* colorRT = fgGraph.GetPhysicalTexture(data.color);
            auto* depthRT = fgGraph.GetPhysicalTexture(data.depth);
            if (!colorRT || !depthRT || !data.passState || !data.passState->initialized)
                return;

            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            if (!nvDevice || !cmdList)
                return;

            auto& cache = framegraph::GetPassResourceCache();
            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(colorRT);
            fbDesc.setDepthAttachment(depthRT);
            auto framebuffer = cache.GetOrCreateFramebuffer("WallmarkPass", fbDesc, nvDevice);
            if (!framebuffer)
                return;

            using namespace fg::bindless;
            auto staticGlobalsCB = cache.GetOrCreateVolatileCB("Frame", "StaticGlobals", sizeof(StaticGlobals), data.device);
            {
                StaticGlobals sg = BuildStaticGlobals();
                cmdList->writeBuffer(staticGlobalsCB, &sg, sizeof(sg));
            }

            auto* shaderLoader = GEnv.Render->GetShaderLoader();
            auto* vsReflection = shaderLoader->GetCachedReflection("bindless_forward", ".vs");
            auto* psReflection = shaderLoader->GetCachedReflection("bindless_forward", ".ps");
            if (!vsReflection || !psReflection)
                return;

            auto* backend = data.device->GetBackend();
            nvrhi::IDescriptorTable* bindlessTable = backend ? backend->GetBindlessDescriptorTable() : nullptr;
            auto& clm = ClusteredLightManager::Instance();
            const auto& cfg = data.config;

            framegraph::BindingSetBuilder bsb(*vsReflection, *psReflection, nvDevice, "Wallmark");
            bsb.ConstantBuffer("static_globals", staticGlobalsCB);
            BindBindlessMaterialTables(bsb);
            BindEnvIblCubes(bsb, data.device);
            bsb.BufferSRV("g_InstanceData", cfg.instanceBuffer);
            bsb.BufferSRV("g_CompactBatchIndices", cfg.compactBatchIndicesBuffer);
            bsb.BufferSRV("g_CompactMaterialIDs", cfg.compactMaterialIDBuffer);
            bsb.BufferSRV("g_LightData", clm.GetLightDataBuffer());
            bsb.BufferSRV("g_ClusterGrid", clm.GetClusterGridBuffer());
            bsb.BufferSRV("g_LightIndexList", clm.GetLightIndexListBuffer());
            auto bindDesc = bsb.Build();
            auto bindingSet = cache.GetOrCreateBindingSet(bindDesc, data.passState->layout, nvDevice);
            if (!bindingSet)
                return;

            auto drawIndexBuffer = GetOrCreateDrawIndexBuffer("TransparentPass", nvDevice);
            nvrhi::GraphicsState gfxState;
            gfxState.framebuffer = framebuffer;
            gfxState.bindings = { bindingSet };
            if (bindlessTable)
                gfxState.addBindingSet(bindlessTable);
            gfxState.vertexBuffers = {
                {cfg.megaVertexBuffer, 0, 0},
                {drawIndexBuffer, 1, 0}
            };
            gfxState.indexBuffer = { cfg.megaIndexBuffer, nvrhi::Format::R32_UINT, 0 };
            const auto& rtDesc = colorRT->getDesc();
            nvrhi::Viewport viewport(0.0f, static_cast<float>(rtDesc.width), 0.0f, static_cast<float>(rtDesc.height), 0.0f, 1.0f);
            gfxState.viewport.addViewport(viewport);
            gfxState.viewport.addScissorRect(nvrhi::Rect(rtDesc.width, rtDesc.height));

            VariantPartitionDrawConfig vpCfg;
            vpCfg.inputLayout = data.passState->inputLayout;
            vpCfg.passLayout = data.passState->layout;
            vpCfg.bindlessLayout = backend ? backend->GetBindlessLayout() : nullptr;
            vpCfg.bindlessTable = bindlessTable;
            vpCfg.megaVertexBuffer = cfg.megaVertexBuffer;
            vpCfg.baseBindings = bindDesc;
            vpCfg.objectCount = cfg.objectCount;
            vpCfg.partition = cfg.variantPartition;
            vpCfg.selectTransparent = true;
            vpCfg.onlyWmark = true;
            DrawVariantPartition(cmdList, nvDevice, framebuffer, gfxState, vpCfg);
        }
    );

    DefaultOutputLayout outputs = inputs;
    outputs.albedo = passData.color;
    outputs.depth = passData.depth;
    return outputs;
}

} // namespace xray::render::fg::passes
