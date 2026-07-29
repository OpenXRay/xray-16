#include "stdafx.h"
#include "TransparentPassSetup.h"
#include "IBLPrefilterPassSetup.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/Backend/D3D12Backend.h"
#include "Layers/xrRender/Bindless/MaterialBuffer.h"
#include "Layers/xrRender/Bindless/TerrainMaterialBuffer.h"
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

namespace xray::render::fg::passes {

void InitializeTransparentResources(fg::RenderDevice* device, const nvrhi::FramebufferInfoEx& fbInfo, TransparentPassState& state)
{
    if (state.initialized)
        return;

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
    state.layout = cache.GetOrCreateBindingLayoutFromReflection("TransparentPass_v2_TranspCB", *vsResult.reflection, *psResult.reflection, nvDevice);
    if (!state.layout)
        return;

    state.sampler = cache.GetLinearWrapSampler(nvDevice);

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
    pipeDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::LessOrEqual;
    pipeDesc.renderState.rasterState.frontCounterClockwise = false;
    pipeDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::Back;
    pipeDesc.renderState.rasterState.depthBias = -16;
    pipeDesc.renderState.rasterState.slopeScaledDepthBias = -2.f;

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

    state.pipeline = cache.GetOrCreatePipeline("TransparentPass_v3_AlphaBias", pipeDesc, fbInfo, nvDevice);
    if (!state.pipeline)
        return;

    {
        nvrhi::GraphicsPipelineDesc mulDesc = pipeDesc;
        auto& mulRt = mulDesc.renderState.blendState.targets[0];
        mulRt.blendEnable = true;
        mulRt.srcBlend = nvrhi::BlendFactor::DstColor;
        mulRt.destBlend = nvrhi::BlendFactor::SrcColor;
        mulRt.blendOp = nvrhi::BlendOp::Add;
        mulRt.srcBlendAlpha = nvrhi::BlendFactor::Zero;
        mulRt.destBlendAlpha = nvrhi::BlendFactor::One;
        mulRt.blendOpAlpha = nvrhi::BlendOp::Add;
        state.multiplyPipeline = cache.GetOrCreatePipeline("TransparentPass_v3_MultiplyBias", mulDesc, fbInfo, nvDevice);
    }

    QueryBindingLayoutFromPipeline(state.pipeline, state.layout);

    // Dedicated water PSO (soft + foam + SSR)
    {
        auto waterVs = shaderLoader->LoadVertexShader("water", "main");
        auto waterPs = shaderLoader->LoadPixelShader("water", "main");
        if (waterVs.handle && waterPs.handle && waterVs.reflection && waterPs.reflection)
        {
            state.waterVs = waterVs.handle;
            state.waterPs = waterPs.handle;
            state.waterLayout = cache.GetOrCreateBindingLayoutFromReflection(
                "TransparentWater_v18_Murk", *waterVs.reflection, *waterPs.reflection, nvDevice);
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
                for (u32 rt = 1; rt < 4; ++rt)
                {
                    waterDesc.renderState.blendState.targets[rt].blendEnable = false;
                    waterDesc.renderState.blendState.targets[rt].setColorWriteMask(
                        nvrhi::ColorMask::Red | nvrhi::ColorMask::Green |
                        nvrhi::ColorMask::Blue | nvrhi::ColorMask::Alpha);
                }
                state.waterPipeline = cache.GetOrCreatePipeline("TransparentWater_v18_Murk", waterDesc, fbInfo, nvDevice);
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
                "TransparentWaterDistort_v6_Amp", *waterdVs.reflection, *waterdPs.reflection, nvDevice);
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
                distortDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::LessOrEqual;
                distortDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::None;
                auto& drt = distortDesc.renderState.blendState.targets[0];
                drt.blendEnable = true;
                drt.srcBlend = nvrhi::BlendFactor::One;
                drt.destBlend = nvrhi::BlendFactor::One;
                drt.blendOp = nvrhi::BlendOp::Add;
                drt.srcBlendAlpha = nvrhi::BlendFactor::One;
                drt.destBlendAlpha = nvrhi::BlendFactor::One;
                drt.blendOpAlpha = nvrhi::BlendOp::Add;

                nvrhi::FramebufferInfoEx distortFb;
                distortFb.colorFormats.push_back(nvrhi::Format::RGBA16_FLOAT);
                distortFb.depthFormat = nvrhi::Format::D32;
                state.waterDistortPipeline = cache.GetOrCreatePipeline(
                    "TransparentWaterDistort_v6_Amp", distortDesc, distortFb, nvDevice);
            }
            waterdVs.reflection = nullptr;
            waterdPs.reflection = nullptr;
        }
        if (!state.waterDistortPipeline)
            Msg("! [TransparentPass] Water distort PSO unavailable");
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
    Msg("* [TransparentPass] Pipeline initialized (water=%d waterd=%d)",
        state.waterPipeline ? 1 : 0, state.waterDistortPipeline ? 1 : 0);
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
                data.worldPos = passBuilder.readWrite(inputs.worldPos, ResourceState::RenderTarget);
            for (u32 i = 0; i < 3; ++i) {
                if (config.shadowHZBHandles[i].is_valid())
                    passBuilder.read(config.shadowHZBHandles[i], ResourceState::ShaderResource);
            }
            if (config.shadowMaskHandle.is_valid())
                passBuilder.read(config.shadowMaskHandle, ResourceState::ShaderResource);

            if (state.waterDistortPipeline)
            {
                framegraph::ResourceDesc distDesc;
                distDesc.type = framegraph::ResourceDesc::Type::Texture2D;
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

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(colorRT);
            if (normalRT)
                fbDesc.addColorAttachment(normalRT);
            if (baseColorRT)
                fbDesc.addColorAttachment(baseColorRT);
            if (worldPosRT)
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
                        distortRT, nvrhi::AllSubresources, nvrhi::Color(0.f, 0.f, 0.f, 0.f));
            }

            using namespace fg::bindless;
            auto& matBuffer = MaterialBuffer::Instance();

            auto lightingCB = cache.GetOrCreateVolatileCB("TransparentPass", "LightingCB", sizeof(LightingConstants), data.device);
            auto staticGlobalsCB = cache.GetOrCreateVolatileCB("Frame", "StaticGlobals", sizeof(StaticGlobals), data.device);
            auto transparentDrawCB = cache.GetOrCreateVolatileCB("Frame", "TransparentDrawCB", 16, data.device);
            auto drawIndexBuffer = GetOrCreateDrawIndexBuffer("TransparentPass", nvDevice);

            {
                StaticGlobals sg = BuildStaticGlobals();
                cmdList->writeBuffer(staticGlobalsCB, &sg, sizeof(sg));
            }

            auto lightingData = FillLightingConstants();
            cmdList->writeBuffer(lightingCB, &lightingData, sizeof(lightingData));

            const auto& cfg = data.config;

            auto& variantTexBuffer = bindless::VariantTextureBuffer::Instance();
            auto& clm = ClusteredLightManager::Instance();
            auto& passCache = framegraph::GetPassResourceCache();

            nvrhi::ITexture* dummy2D = passCache.GetDummyShadowMap2D(nvDevice);
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

            const auto& rtDesc = colorRT->getDesc();
            nvrhi::Viewport viewport(0.0f, static_cast<float>(rtDesc.width), 0.0f, static_cast<float>(rtDesc.height), 0.0f, 1.0f);

            auto drawTransparentBatch = [&](nvrhi::IGraphicsPipeline* pipeline, u32 blendPass) {
                if (!pipeline)
                    return;

                u32 passMode[4] = { blendPass, 0, 0, 0 };
                cmdList->writeBuffer(transparentDrawCB, passMode, sizeof(passMode));

                framegraph::BindingSetBuilder bsb(*vsReflection, *psReflection, nvDevice, "Transparent");
                bsb.ConstantBuffer("static_globals", staticGlobalsCB);
                bsb.ConstantBuffer("TransparentDrawCB", transparentDrawCB);
                BindBindlessMaterialTables(bsb);
                bsb.BufferSRV("g_InstanceData", cfg.instanceBuffer);
                bsb.BufferSRV("g_CompactBatchIndices", cfg.compactBatchIndicesBuffer);
                bsb.BufferSRV("g_CompactMaterialIDs", cfg.compactMaterialIDBuffer);
                bsb.BufferSRV("g_LightData", clm.GetLightDataBuffer());
                if (clm.GetShadowDataBuffer())
                    bsb.BufferSRV("g_ShadowData", clm.GetShadowDataBuffer());
                bsb.BufferSRV("g_ClusterGrid", clm.GetClusterGridBuffer());
                bsb.BufferSRV("g_LightIndexList", clm.GetLightIndexListBuffer());
                {
                    static const char* kNames[3] = {"g_ShadowMap0", "g_ShadowMap1", "g_ShadowMap2"};
                    for (u32 i = 0; i < 3; ++i)
                    {
                        nvrhi::ITexture* t = cfg.shadowCascades[i] ? cfg.shadowCascades[i]
                            : (i == 0 && cfg.shadowMapArray ? cfg.shadowMapArray : dummy2D);
                        if (t)
                            bsb.Texture(kNames[i], t);
                    }
                }
                {
                    nvrhi::ITexture* contactHist = cfg.contactHistory
                        ? cfg.contactHistory
                        : passCache.GetDummyContactHistory(nvDevice);
                    if (contactHist)
                        bsb.Texture("g_ContactHistory", contactHist);
                }
                {
                    nvrhi::ITexture* localAtlas = cfg.localShadowAtlas
                        ? cfg.localShadowAtlas
                        : passCache.GetDummyShadowMap(nvDevice);
                    if (localAtlas)
                        bsb.Texture("g_LocalShadowAtlas", localAtlas);
                    nvrhi::ITexture* localEsm = cfg.localShadowESM
                        ? cfg.localShadowESM
                        : passCache.GetDummyLocalShadowESM(nvDevice);
                    if (localEsm)
                        bsb.Texture("g_LocalShadowESM", localEsm);
                }
                {
                    static const char* kHzb[3] = {"g_ShadowHZB0", "g_ShadowHZB1", "g_ShadowHZB2"};
                    nvrhi::ITexture* dummyHzb = passCache.GetDummyContactDepth(nvDevice);
                    for (u32 i = 0; i < 3; ++i)
                    {
                        nvrhi::ITexture* hz = cfg.shadowHZB[i] ? cfg.shadowHZB[i] : dummyHzb;
                        if (hz)
                            bsb.Texture(kHzb[i], hz);
                    }
                }
                {
                    nvrhi::ITexture* shadowMask = cfg.shadowMask
                        ? cfg.shadowMask
                        : passCache.GetDummyContactHistory(nvDevice);
                    if (shadowMask)
                        bsb.Texture("g_ShadowMask", shadowMask);
                }
                if (sky0)
                    bsb.Texture("s_env0", sky0);
                if (sky1)
                    bsb.Texture("s_env1", sky1);
                BindIBLResources(bsb, GetCurrentIBLBindResources(), nvDevice);

                auto bindingSet = passCache.GetOrCreateBindingSet(bsb.Build(), data.passState->layout, nvDevice);
                if (!bindingSet)
                    return;

                nvrhi::GraphicsState gfxState;
                gfxState.pipeline = pipeline;
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
                gfxState.viewport.addViewport(viewport);
                gfxState.viewport.addScissorRect(nvrhi::Rect(rtDesc.width, rtDesc.height));

                if (cfg.variantPartition.Enabled()) {
                    auto* backendDev = data.device->GetBackend();

                    VariantPartitionDrawConfig vpCfg;
                    vpCfg.defaultPipeline = pipeline;
                    vpCfg.inputLayout = data.passState->inputLayout;
                    vpCfg.passLayout = data.passState->layout;
                    vpCfg.bindlessLayout = backendDev ? backendDev->GetBindlessLayout() : nullptr;
                    vpCfg.bindlessTable = bindlessTable;
                    vpCfg.sampler = data.passState->sampler
                        ? data.passState->sampler.Get()
                        : passCache.GetLinearWrapSampler(nvDevice);
                    vpCfg.staticGlobalsCB = staticGlobalsCB;
                    vpCfg.lightingCB = lightingCB;
                    vpCfg.materialBuffer = matBuffer.GetBuffer();
                    vpCfg.variantTexBuffer = variantTexBuffer.GetBuffer();
                    vpCfg.instanceBuffer = cfg.instanceBuffer;
                    vpCfg.megaVertexBuffer = cfg.megaVertexBuffer;
                    vpCfg.shadowMapArray = cfg.shadowCascades[0] ? cfg.shadowCascades[0] : cfg.shadowMapArray;
                    for (u32 i = 0; i < 3; ++i)
                    {
                        vpCfg.shadowCascades[i] = cfg.shadowCascades[i];
                        vpCfg.shadowHZB[i] = cfg.shadowHZB[i];
                    }
                    vpCfg.localShadowAtlas = cfg.localShadowAtlas;
                    vpCfg.localShadowESM = cfg.localShadowESM;
                    vpCfg.contactHistory = cfg.contactHistory;
                    vpCfg.shadowMask = cfg.shadowMask;
                    vpCfg.envSky0 = sky0;
                    vpCfg.envSky1 = sky1;
                    vpCfg.lightDataBuffer = clm.GetLightDataBuffer();
                    vpCfg.shadowDataBuffer = clm.GetShadowDataBuffer();
                    vpCfg.clusterGridBuffer = clm.GetClusterGridBuffer();
                    vpCfg.lightIndexListBuffer = clm.GetLightIndexListBuffer();
                    vpCfg.partition = cfg.variantPartition;
                    vpCfg.selectTransparent = true;

                    DrawVariantPartition(cmdList, nvDevice, framebuffer, gfxState, vpCfg);
                } else {
                    cmdList->setGraphicsState(gfxState);
                    DrawIndexedIndirectCountOrFallback(cmdList, 0, 0, cfg.objectCount);
                }
            };

            drawTransparentBatch(data.passState->pipeline.Get(), 1);
            drawTransparentBatch(data.passState->multiplyPipeline.Get(), 2);

            // Second draw: dedicated water PSO (soft depth + foam + SSR)
            if (data.passState->waterPipeline && data.passState->waterLayout)
            {
                auto* waterVsRefl = shaderLoader->GetCachedReflection("water", ".vs");
                auto* waterPsRefl = shaderLoader->GetCachedReflection("water", ".ps");
                if (waterVsRefl && waterPsRefl)
                {
                    // Snapshot HDR + depth for SSR/soft water (cannot sample RTs we write / DSV)
                    const auto& cdesc = colorRT->getDesc();
                    if (!data.passState->waterSsrColor ||
                        data.passState->waterSsrColor->getDesc().width != cdesc.width ||
                        data.passState->waterSsrColor->getDesc().height != cdesc.height)
                    {
                        nvrhi::TextureDesc td = cdesc;
                        td.debugName = "WaterSSR_Color";
                        td.isRenderTarget = true;
                        td.isShaderResource = true;
                        td.initialState = nvrhi::ResourceStates::ShaderResource;
                        td.keepInitialState = true;
                        data.passState->waterSsrColor = nvDevice->createTexture(td);
                    }
                    nvrhi::ITexture* ssrColor = data.passState->waterSsrColor;
                    nvrhi::ITexture* sceneDepthSrv = nullptr;
                    // Single ping-pong: unbind → copy color+depth → restore for water draw
                    if (ssrColor)
                    {
                        cmdList->setTextureState(colorRT, nvrhi::AllSubresources, nvrhi::ResourceStates::CopySource);
                        cmdList->setTextureState(ssrColor, nvrhi::AllSubresources, nvrhi::ResourceStates::CopyDest);
                        cmdList->copyTexture(ssrColor, nvrhi::TextureSlice(), colorRT, nvrhi::TextureSlice());
                    }

                    const auto& ddesc = depthRT->getDesc();
                    if (!data.passState->waterSceneDepth ||
                        data.passState->waterSceneDepth->getDesc().width != ddesc.width ||
                        data.passState->waterSceneDepth->getDesc().height != ddesc.height)
                    {
                        nvrhi::TextureDesc td{};
                        td.width = ddesc.width;
                        td.height = ddesc.height;
                        td.format = nvrhi::Format::D32;
                        td.debugName = "WaterSSR_Depth";
                        td.isShaderResource = true;
                        td.isTypeless = true;
                        td.initialState = nvrhi::ResourceStates::ShaderResource;
                        td.keepInitialState = true;
                        data.passState->waterSceneDepth = nvDevice->createTexture(td);
                    }
                    sceneDepthSrv = data.passState->waterSceneDepth;
                    if (sceneDepthSrv)
                    {
                        cmdList->setTextureState(depthRT, nvrhi::AllSubresources, nvrhi::ResourceStates::CopySource);
                        cmdList->setTextureState(sceneDepthSrv, nvrhi::AllSubresources, nvrhi::ResourceStates::CopyDest);
                        cmdList->copyTexture(sceneDepthSrv, nvrhi::TextureSlice(), depthRT, nvrhi::TextureSlice());
                        cmdList->setTextureState(sceneDepthSrv, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
                    }

                    auto* worldPosRT = data.worldPos.is_valid() ? fg.GetPhysicalTexture(data.worldPos) : nullptr;
                    if (worldPosRT)
                    {
                        const auto& wpDesc = worldPosRT->getDesc();
                        if (!data.passState->waterSceneWorldPos ||
                            data.passState->waterSceneWorldPos->getDesc().width != wpDesc.width ||
                            data.passState->waterSceneWorldPos->getDesc().height != wpDesc.height)
                        {
                            nvrhi::TextureDesc td = wpDesc;
                            td.debugName = "WaterSoft_WorldPos";
                            td.isRenderTarget = true;
                            td.isShaderResource = true;
                            td.initialState = nvrhi::ResourceStates::ShaderResource;
                            td.keepInitialState = true;
                            data.passState->waterSceneWorldPos = nvDevice->createTexture(td);
                        }
                        if (data.passState->waterSceneWorldPos)
                        {
                            nvrhi::ITexture* wpCopy = data.passState->waterSceneWorldPos;
                            cmdList->setTextureState(
                                worldPosRT, nvrhi::AllSubresources, nvrhi::ResourceStates::CopySource);
                            cmdList->setTextureState(
                                wpCopy, nvrhi::AllSubresources, nvrhi::ResourceStates::CopyDest);
                            cmdList->copyTexture(
                                wpCopy, nvrhi::TextureSlice(), worldPosRT, nvrhi::TextureSlice());
                            cmdList->setTextureState(
                                wpCopy, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
                        }
                    }

                    if (ssrColor)
                        cmdList->setTextureState(ssrColor, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
                    cmdList->setTextureState(colorRT, nvrhi::AllSubresources, nvrhi::ResourceStates::RenderTarget);
                    cmdList->setTextureState(depthRT, nvrhi::AllSubresources, nvrhi::ResourceStates::DepthRead);
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
                    const float softOn = ps_r2_ls_flags.test(R2FLAG_SOFT_WATER) ? 1.f : 0.f;
                    wi.set(intens, intens, intens, softOn);
                    cmdList->writeBuffer(waterCB, &wi, sizeof(wi));

                    nvrhi::ITexture* foam = data.passState->foamTexture;
                    if (!foam)
                        foam = passCache.GetDummyShadowMap2D(nvDevice);

                    framegraph::BindingSetBuilder wbsb(*waterVsRefl, *waterPsRefl, nvDevice, "Transparent.Water");
                    wbsb.ConstantBuffer("static_globals", staticGlobalsCB)
                        .ConstantBuffer("WaterParams", waterCB);
                    BindBindlessMaterialTables(wbsb);
                    wbsb.BufferSRV("g_InstanceData", cfg.instanceBuffer)
                        .BufferSRV("g_CompactBatchIndices", cfg.compactBatchIndicesBuffer)
                        .BufferSRV("g_CompactMaterialIDs", cfg.compactMaterialIDBuffer);
                    if (sky0) wbsb.Texture("s_env0", sky0);
                    if (sky1) wbsb.Texture("s_env1", sky1);
                    if (foam) wbsb.Texture("s_leaves", foam);
                    if (ssrColor) wbsb.Texture("g_SceneColor", ssrColor);
                    if (sceneDepthSrv) wbsb.Texture("g_SceneDepth", sceneDepthSrv);
                    else wbsb.Texture("g_SceneDepth", passCache.GetDummyContactDepth(nvDevice));
                    if (data.passState->waterSceneWorldPos)
                        wbsb.Texture("g_UnderWorldPos", data.passState->waterSceneWorldPos);
                    else
                        wbsb.Texture("g_UnderWorldPos", passCache.GetDummyContactHistory(nvDevice));

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

                    if (data.passState->waterDistortPipeline && data.passState->waterDistortLayout &&
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
                                "TransparentWaterDistort", distortFbDesc, nvDevice);
                            if (distortFB)
                            {
                                framegraph::BindingSetBuilder dbsb(
                                    *waterdVsRefl, *waterdPsRefl, nvDevice, "Transparent.WaterDistort");
                                dbsb.ConstantBuffer("static_globals", staticGlobalsCB)
                                    .ConstantBuffer("WaterParams", waterCB);
                                BindBindlessMaterialTables(dbsb);
                                dbsb.BufferSRV("g_InstanceData", cfg.instanceBuffer)
                                    .BufferSRV("g_CompactBatchIndices", cfg.compactBatchIndicesBuffer)
                                    .BufferSRV("g_CompactMaterialIDs", cfg.compactMaterialIDBuffer);
                                if (sceneDepthSrv)
                                    dbsb.Texture("g_SceneDepth", sceneDepthSrv);
                                else
                                    dbsb.Texture("g_SceneDepth", passCache.GetDummyContactDepth(nvDevice));
                                dbsb.Texture("g_SceneColor", ssrColor
                                    ? ssrColor
                                    : passCache.GetDummyContactHistory(nvDevice));
                                if (data.passState->waterSceneWorldPos)
                                    dbsb.Texture("g_UnderWorldPos", data.passState->waterSceneWorldPos);
                                else
                                    dbsb.Texture("g_UnderWorldPos", passCache.GetDummyContactHistory(nvDevice));

                                auto distortSet = passCache.GetOrCreateBindingSet(
                                    dbsb.Build(), data.passState->waterDistortLayout, nvDevice);
                                if (distortSet)
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

} // namespace xray::render::fg::passes
