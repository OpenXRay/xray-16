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
    state.layout = cache.GetOrCreateBindingLayoutFromReflection("TransparentPass_CSMLadder", *vsResult.reflection, *psResult.reflection, nvDevice);
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

    auto& rt0 = pipeDesc.renderState.blendState.targets[0];
    rt0.blendEnable = true;
    rt0.srcBlend = nvrhi::BlendFactor::SrcAlpha;
    rt0.destBlend = nvrhi::BlendFactor::InvSrcAlpha;
    rt0.blendOp = nvrhi::BlendOp::Add;
    rt0.srcBlendAlpha = nvrhi::BlendFactor::One;
    rt0.destBlendAlpha = nvrhi::BlendFactor::InvSrcAlpha;
    rt0.blendOpAlpha = nvrhi::BlendOp::Add;

    state.pipeline = cache.GetOrCreatePipeline("TransparentPass_CSMLadder", pipeDesc, fbInfo, nvDevice);
    if (!state.pipeline)
        return;

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
                "TransparentWater_v3", *waterVs.reflection, *waterPs.reflection, nvDevice);
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
                state.waterPipeline = cache.GetOrCreatePipeline("TransparentWater_v3", waterDesc, fbInfo, nvDevice);
            }
            waterVs.reflection = nullptr;
            waterPs.reflection = nullptr;
        }
        if (!state.waterPipeline)
            Msg("! [TransparentPass] Water PSO unavailable — water clipped from forward");
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
    Msg("* [TransparentPass] Pipeline initialized (water=%d)", state.waterPipeline ? 1 : 0);
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

            using namespace fg::bindless;
            auto& matBuffer = MaterialBuffer::Instance();

            auto lightingCB = cache.GetOrCreateVolatileCB("TransparentPass", "LightingCB", sizeof(LightingConstants), data.device);
            auto staticGlobalsCB = cache.GetOrCreateVolatileCB("Frame", "StaticGlobals", sizeof(StaticGlobals), data.device);
            auto drawIndexBuffer = GetOrCreateDrawIndexBuffer("TransparentPass", nvDevice);

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

            framegraph::BindingSetBuilder bsb(*vsReflection, *psReflection, nvDevice, "Transparent");
            bsb.ConstantBuffer("static_globals", staticGlobalsCB);
            bsb.BufferSRV("g_Materials", matBuffer.GetBuffer());
            bsb.BufferSRV("g_InstanceData", cfg.instanceBuffer);
            bsb.BufferSRV("g_CompactBatchIndices", cfg.compactBatchIndicesBuffer);
            bsb.BufferSRV("g_CompactMaterialIDs", cfg.compactMaterialIDBuffer);
            bsb.BufferSRV("g_LightData", clm.GetLightDataBuffer());
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
            }
            if (sky0)
                bsb.Texture("s_env0", sky0);
            if (sky1)
                bsb.Texture("s_env1", sky1);

            auto bindingSet = passCache.GetOrCreateBindingSet(bsb.Build(), data.passState->layout, nvDevice);
            if (!bindingSet)
            {
                Msg("! [TransparentPass] Binding set create failed — skip draw");
                return;
            }

            nvrhi::GraphicsState gfxState;
            gfxState.pipeline = data.passState->pipeline;
            gfxState.framebuffer = framebuffer;
            gfxState.bindings = { bindingSet };

            auto* backend = data.device->GetBackend();
            nvrhi::IBindingSet* bindlessTable = nullptr;
            if (backend) {
                bindlessTable = backend->GetBindlessDescriptorTable();
                if (bindlessTable)
                    gfxState.addBindingSet(bindlessTable);
            }

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
                    vpCfg.shadowCascades[i] = cfg.shadowCascades[i];
                vpCfg.localShadowAtlas = cfg.localShadowAtlas;
                vpCfg.envSky0 = sky0;
                vpCfg.envSky1 = sky1;
                vpCfg.lightDataBuffer = clm.GetLightDataBuffer();
                vpCfg.clusterGridBuffer = clm.GetClusterGridBuffer();
                vpCfg.lightIndexListBuffer = clm.GetLightIndexListBuffer();
                vpCfg.partition = cfg.variantPartition;
                vpCfg.selectTransparent = true;

                DrawVariantPartition(cmdList, nvDevice, framebuffer, gfxState, vpCfg);
            } else {
                cmdList->setGraphicsState(gfxState);
                DrawIndexedIndirectCountOrFallback(cmdList, 0, 0, cfg.objectCount);
            }

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
                    // Unbind RT/DSV → Copy → SRV so water SSR can sample scene (Metal/MoltenVK).
                    if (ssrColor)
                    {
                        cmdList->setTextureState(colorRT, nvrhi::AllSubresources, nvrhi::ResourceStates::CopySource);
                        cmdList->setTextureState(ssrColor, nvrhi::AllSubresources, nvrhi::ResourceStates::CopyDest);
                        cmdList->copyTexture(ssrColor, nvrhi::TextureSlice(), colorRT, nvrhi::TextureSlice());
                        cmdList->setTextureState(ssrColor, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
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
                    nvrhi::ITexture* sceneDepthSrv = data.passState->waterSceneDepth;
                    if (sceneDepthSrv)
                    {
                        cmdList->setTextureState(depthRT, nvrhi::AllSubresources, nvrhi::ResourceStates::CopySource);
                        cmdList->setTextureState(sceneDepthSrv, nvrhi::AllSubresources, nvrhi::ResourceStates::CopyDest);
                        cmdList->copyTexture(sceneDepthSrv, nvrhi::TextureSlice(), depthRT, nvrhi::TextureSlice());
                        cmdList->setTextureState(sceneDepthSrv, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
                    }
                    // Restore attachments for the water draw that follows
                    cmdList->setTextureState(colorRT, nvrhi::AllSubresources, nvrhi::ResourceStates::RenderTarget);
                    if (normalRT)
                        cmdList->setTextureState(normalRT, nvrhi::AllSubresources, nvrhi::ResourceStates::RenderTarget);
                    if (baseColorRT)
                        cmdList->setTextureState(baseColorRT, nvrhi::AllSubresources, nvrhi::ResourceStates::RenderTarget);
                    if (worldPosRT)
                        cmdList->setTextureState(worldPosRT, nvrhi::AllSubresources, nvrhi::ResourceStates::RenderTarget);
                    // Transparent pass declares depth as read-only (test, no write)
                    cmdList->setTextureState(depthRT, nvrhi::AllSubresources, nvrhi::ResourceStates::DepthRead);

                    auto waterCB = cache.GetOrCreateVolatileCB(
                        "TransparentWater", "WaterParams", sizeof(Fvector4), data.device);
                    Fvector4 wi{};
                    wi.set(1.f, 1.f, 1.f, 1.f);
                    if (g_pGamePersistent)
                    {
                        const auto& env = g_pGamePersistent->Environment().CurrentEnv;
                        float intens = std::max(env.fog_color.x, std::max(env.fog_color.y, env.fog_color.z));
                        wi.set(std::max(intens, 0.35f), intens, intens, 1.f);
                    }
                    cmdList->writeBuffer(waterCB, &wi, sizeof(wi));

                    nvrhi::ITexture* foam = data.passState->foamTexture;
                    if (!foam)
                        foam = passCache.GetDummyShadowMap2D(nvDevice);

                    framegraph::BindingSetBuilder wbsb(*waterVsRefl, *waterPsRefl, nvDevice, "Transparent.Water");
                    wbsb.ConstantBuffer("static_globals", staticGlobalsCB)
                        .ConstantBuffer("WaterParams", waterCB)
                        .BufferSRV("g_Materials", matBuffer.GetBuffer())
                        .BufferSRV("g_InstanceData", cfg.instanceBuffer)
                        .BufferSRV("g_CompactBatchIndices", cfg.compactBatchIndicesBuffer)
                        .BufferSRV("g_CompactMaterialIDs", cfg.compactMaterialIDBuffer);
                    if (sky0) wbsb.Texture("s_env0", sky0);
                    if (sky1) wbsb.Texture("s_env1", sky1);
                    if (foam) wbsb.Texture("s_leaves", foam);
                    if (ssrColor) wbsb.Texture("g_SceneColor", ssrColor);
                    if (sceneDepthSrv) wbsb.Texture("g_SceneDepth", sceneDepthSrv);

                    auto waterSet = passCache.GetOrCreateBindingSet(
                        wbsb.Build(), data.passState->waterLayout, nvDevice);
                    if (waterSet)
                    {
                        nvrhi::GraphicsState waterGfx = gfxState;
                        waterGfx.pipeline = data.passState->waterPipeline;
                        waterGfx.bindings = {waterSet};
                        if (bindlessTable)
                            waterGfx.addBindingSet(bindlessTable);
                        cmdList->setGraphicsState(waterGfx);
                        DrawIndexedIndirectCountOrFallback(cmdList, 0, 0, cfg.objectCount);
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
    return outputs;
}

} // namespace xray::render::fg::passes
