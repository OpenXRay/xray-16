#include "stdafx.h"
#include "TransparentPassSetup.h"
#include "ShaderConstants.h"
#include "VSMPassSetup.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/OutputLayout.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/Backend/D3D12Backend.h"
#include "Layers/xrRender/Bindless/MaterialBuffer.h"
#include "Layers/xrRender/Bindless/VariantBuffer.h"
#include "Layers/xrRender/Bindless/VariantTextureBuffer.h"
#include "Layers/xrRender/ShaderVariant/ShaderVariantRegistry.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "PassCommon.h"
#include "Layers/xrRender/ClusteredLightManager.h"

namespace xray::render::fg::passes {

static nvrhi::BlendFactor ToBlendFactor(u32 factor)
{
    switch (static_cast<VariantBlendFactor>(factor)) {
    case VariantBlendFactor::Zero: return nvrhi::BlendFactor::Zero;
    case VariantBlendFactor::One: return nvrhi::BlendFactor::One;
    case VariantBlendFactor::SrcColor: return nvrhi::BlendFactor::SrcColor;
    case VariantBlendFactor::InvSrcColor: return nvrhi::BlendFactor::InvSrcColor;
    case VariantBlendFactor::SrcAlpha: return nvrhi::BlendFactor::SrcAlpha;
    case VariantBlendFactor::InvSrcAlpha: return nvrhi::BlendFactor::InvSrcAlpha;
    case VariantBlendFactor::DstAlpha: return nvrhi::BlendFactor::DstAlpha;
    case VariantBlendFactor::InvDstAlpha: return nvrhi::BlendFactor::InvDstAlpha;
    case VariantBlendFactor::DstColor: return nvrhi::BlendFactor::DstColor;
    case VariantBlendFactor::InvDstColor: return nvrhi::BlendFactor::InvDstColor;
    }
    return nvrhi::BlendFactor::SrcAlpha;
}

static nvrhi::GraphicsPipelineDesc MakeBasePipelineDesc(fg::RenderDevice* device, TransparentPassState& state,
    nvrhi::IShader* ps, nvrhi::IBindingLayout* layout)
{
    nvrhi::GraphicsPipelineDesc desc;
    desc.VS = state.vs;
    desc.PS = ps;
    desc.inputLayout = state.inputLayout;
    auto* backend = device->GetBackend();
    nvrhi::IBindingLayout* bindlessLayout = backend ? backend->GetBindlessLayout() : nullptr;
    if (bindlessLayout)
        desc.bindingLayouts = { layout, bindlessLayout };
    else
        desc.bindingLayouts = { layout };
    desc.primType = nvrhi::PrimitiveType::TriangleList;
    desc.renderState.depthStencilState.depthTestEnable = true;
    desc.renderState.depthStencilState.depthWriteEnable = false;
    desc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::GreaterOrEqual;
    desc.renderState.rasterState.frontCounterClockwise = false;
    desc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::Back;
    return desc;
}

static nvrhi::IGraphicsPipeline* GetColorPipeline(fg::RenderDevice* device, TransparentPassState& state, u32 key)
{
    auto it = state.pipelines.find(key);
    if (it != state.pipelines.end())
        return it->second.Get();

    nvrhi::GraphicsPipelineDesc desc = MakeBasePipelineDesc(device, state, state.ps, state.layout);
    desc.renderState.depthStencilState.depthWriteEnable = (key & TRANSPARENT_KEY_DEPTH_WRITE) != 0;
    auto& rt0 = desc.renderState.blendState.targets[0];
    rt0.blendEnable = true;
    rt0.srcBlend = ToBlendFactor(key & 0xFFu);
    rt0.destBlend = ToBlendFactor((key >> TRANSPARENT_KEY_DST_SHIFT) & 0xFFu);
    rt0.blendOp = nvrhi::BlendOp::Add;
    rt0.srcBlendAlpha = nvrhi::BlendFactor::One;
    rt0.destBlendAlpha = nvrhi::BlendFactor::InvSrcAlpha;
    rt0.blendOpAlpha = nvrhi::BlendOp::Add;
    if (key & TRANSPARENT_KEY_UNLIT) {
        desc.renderState.blendState.targets[1].colorWriteMask = static_cast<nvrhi::ColorMask>(0);
        desc.renderState.blendState.targets[2].colorWriteMask = static_cast<nvrhi::ColorMask>(0);
    }

    string64 name;
    xr_sprintf(name, "TransparentPass_%08x", key);
    auto& cache = framegraph::GetPassResourceCache();
    nvrhi::GraphicsPipelineHandle pipeline = cache.GetOrCreatePipeline(name, desc, state.fbInfo, device->GetNVRHIDevice());
    state.pipelines[key] = pipeline;
    return pipeline.Get();
}

static nvrhi::IGraphicsPipeline* GetWallmarkPipeline(fg::RenderDevice* device, TransparentPassState& state, u32 key)
{
    auto it = state.wallmarkPipelines.find(key);
    if (it != state.wallmarkPipelines.end())
        return it->second.Get();

    nvrhi::GraphicsPipelineDesc desc = MakeBasePipelineDesc(device, state, state.wallmarkPS, state.wallmarkLayout);
    auto& rt0 = desc.renderState.blendState.targets[0];
    rt0.blendEnable = true;
    rt0.srcBlend = ToBlendFactor(key & 0xFFu);
    rt0.destBlend = ToBlendFactor((key >> TRANSPARENT_KEY_DST_SHIFT) & 0xFFu);
    rt0.blendOp = nvrhi::BlendOp::Add;
    rt0.srcBlendAlpha = nvrhi::BlendFactor::Zero;
    rt0.destBlendAlpha = nvrhi::BlendFactor::One;
    rt0.blendOpAlpha = nvrhi::BlendOp::Add;
    rt0.colorWriteMask = nvrhi::ColorMask::Red | nvrhi::ColorMask::Green | nvrhi::ColorMask::Blue;

    string64 name;
    xr_sprintf(name, "StaticWallmarkPass_%08x", key);
    auto& cache = framegraph::GetPassResourceCache();
    nvrhi::GraphicsPipelineHandle pipeline = cache.GetOrCreatePipeline(name, desc, state.wallmarkFbInfo, device->GetNVRHIDevice());
    state.wallmarkPipelines[key] = pipeline;
    return pipeline.Get();
}

static nvrhi::FramebufferInfoEx TransparentFramebufferInfo()
{
    nvrhi::FramebufferInfoEx fbInfo;
    fbInfo.colorFormats.push_back(nvrhi::Format::RGBA16_FLOAT);
    fbInfo.colorFormats.push_back(nvrhi::Format::RGBA16_FLOAT);
    fbInfo.colorFormats.push_back(nvrhi::Format::RGBA8_UNORM);
    fbInfo.depthFormat = nvrhi::Format::D32;
    return fbInfo;
}

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
    auto distortResult = shaderLoader->LoadPixelShader("bindless_forward_distort", "main");
    auto wallmarkResult = shaderLoader->LoadPixelShader("bindless_wallmark", "main");
    if (!vsResult.handle || !psResult.handle || !distortResult.handle || !wallmarkResult.handle)
        return;

    state.vs = vsResult.handle;
    state.ps = psResult.handle;
    state.distortPS = distortResult.handle;
    state.wallmarkPS = wallmarkResult.handle;
    state.fbInfo = fbInfo;
    nvrhi::FramebufferInfoEx wallmarkFbInfo;
    wallmarkFbInfo.colorFormats.push_back(nvrhi::Format::RGBA8_UNORM);
    wallmarkFbInfo.depthFormat = nvrhi::Format::D32;
    state.wallmarkFbInfo = wallmarkFbInfo;

    auto& cache = framegraph::GetPassResourceCache();
    state.layout = cache.GetOrCreateBindingLayoutFromReflection("TransparentPass", *vsResult.reflection, *psResult.reflection, nvDevice);
    state.distortLayout = cache.GetOrCreateBindingLayoutFromReflection("TransparentPass_Distort", *vsResult.reflection, *distortResult.reflection, nvDevice);
    state.wallmarkLayout = cache.GetOrCreateBindingLayoutFromReflection("StaticWallmarkPass", *vsResult.reflection, *wallmarkResult.reflection, nvDevice);
    if (!state.layout || !state.distortLayout || !state.wallmarkLayout)
        return;

    u32 attrCount = 0;
    auto* attrs = GetUnifiedVertexAttributes(attrCount);
    state.inputLayout = nvDevice->createInputLayout(attrs, attrCount, state.vs);

    auto drawIndexBuffer = GetOrCreateDrawIndexBuffer("TransparentPass", nvDevice);
    if (!drawIndexBuffer)
        return;

    nvrhi::FramebufferInfoEx distortFbInfo;
    distortFbInfo.colorFormats.push_back(nvrhi::Format::RGBA16_FLOAT);
    distortFbInfo.depthFormat = nvrhi::Format::D32;
    nvrhi::GraphicsPipelineDesc distortDesc = MakeBasePipelineDesc(device, state, state.distortPS, state.distortLayout);
    distortDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::None;
    auto& drt0 = distortDesc.renderState.blendState.targets[0];
    drt0.blendEnable = true;
    drt0.srcBlend = nvrhi::BlendFactor::One;
    drt0.destBlend = nvrhi::BlendFactor::One;
    drt0.blendOp = nvrhi::BlendOp::Add;
    drt0.srcBlendAlpha = nvrhi::BlendFactor::One;
    drt0.destBlendAlpha = nvrhi::BlendFactor::One;
    drt0.blendOpAlpha = nvrhi::BlendOp::Add;
    state.distortPipeline = cache.GetOrCreatePipeline("TransparentPass_Distort", distortDesc, distortFbInfo, nvDevice);
    if (!state.distortPipeline)
        return;

    const u32 defaultKey = u32(VariantBlendFactor::SrcAlpha) | (u32(VariantBlendFactor::InvSrcAlpha) << TRANSPARENT_KEY_DST_SHIFT);
    if (!GetColorPipeline(device, state, defaultKey))
        return;

    const u32 wallmarkKey = u32(VariantBlendFactor::DstColor) | (u32(VariantBlendFactor::SrcColor) << TRANSPARENT_KEY_DST_SHIFT) | TRANSPARENT_KEY_WMARK;
    if (!GetWallmarkPipeline(device, state, wallmarkKey))
        return;

    QueryBindingLayoutFromPipeline(state.pipelines[defaultKey], state.layout);
    QueryBindingLayoutFromPipeline(state.distortPipeline, state.distortLayout);
    QueryBindingLayoutFromPipeline(state.wallmarkPipelines[wallmarkKey], state.wallmarkLayout);

    state.initialized = true;
    Msg("* [TransparentPass] Pipeline initialized");
}

static bool RangesWantDistortion(const xr_vector<TransparentDrawRange>* ranges)
{
    if (!ranges)
        return false;
    for (const auto& range : *ranges)
        if (range.key & TRANSPARENT_KEY_DISTORT)
            return true;
    return false;
}

framegraph::DefaultOutputLayout setupTransparentPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    const framegraph::DefaultOutputLayout& inputs,
    const TransparentPassConfig& config,
    const LocalShadowOutput& localShadow,
    framegraph::VirtualResourceHandle sunMask,
    framegraph::VirtualResourceHandle skinnedOrder,
    u32 width, u32 height,
    TransparentPassState& state)
{
    using namespace framegraph;

    if (!config.IsValid()) {
        return inputs;
    }

    InitializeTransparentResources(device, TransparentFramebufferInfo(), state);

    const bool wantDistortion = RangesWantDistortion(config.ranges)
        || (config.skinned && config.gpuCulling && RangesWantDistortion(&config.gpuCulling->GetSkinnedForwardRanges()));

    auto& passData = fg.addCallbackPass<TransparentPassData>(
        "Transparent Pass",

        [&, width, height, config, localShadow, sunMask, skinnedOrder, wantDistortion](FrameGraph& builder, PassHandle passHandle, TransparentPassData& data) {
            data.width = width;
            data.height = height;
            data.device = device;
            data.config = config;
            data.passState = &state;

            RenderPassBuilder passBuilder(builder, passHandle);
            data.color = passBuilder.readWrite(inputs.albedo, ResourceState::RenderTarget);
            data.normal = passBuilder.readWrite(inputs.normal, ResourceState::RenderTarget);
            data.depth = passBuilder.readWrite(inputs.depth, ResourceState::DepthStencilWrite);
            if (inputs.baseColor.is_valid())
                data.baseColor = passBuilder.readWrite(inputs.baseColor, ResourceState::RenderTarget);
            if (skinnedOrder.is_valid())
                data.skinnedOrder = passBuilder.read(skinnedOrder, ResourceState::ShaderResource);
            if (sunMask.is_valid())
                data.sunMask = passBuilder.read(sunMask, ResourceState::ShaderResource);
            if (wantDistortion) {
                ResourceDesc distDesc;
                distDesc.type = ResourceDesc::Type::Texture2D;
                distDesc.width = width;
                distDesc.height = height;
                distDesc.format = nvrhi::Format::RGBA16_FLOAT;
                distDesc.isRenderTarget = true;
                distDesc.isTransient = true;
                distDesc.isUAV = true;
                distDesc.debugName = "rt_Distortion";
                data.distortion = passBuilder.createTexture("rt_Distortion", distDesc);
            }
            data.localShadow = localShadow;
            if (localShadow.active) {
                data.localTiles = passBuilder.read(localShadow.tiles, ResourceState::ShaderResource);
                data.localStatic = passBuilder.read(localShadow.staticAtlas, ResourceState::ShaderResource);
                data.localDyn = passBuilder.read(localShadow.dynAtlas, ResourceState::ShaderResource);
                data.localHud = passBuilder.read(localShadow.hudAtlas, ResourceState::ShaderResource);
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

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(colorRT);
            if (normalRT)
                fbDesc.addColorAttachment(normalRT);
            if (baseColorRT)
                fbDesc.addColorAttachment(baseColorRT);
            fbDesc.setDepthAttachment(depthRT);
            auto& cache = framegraph::GetPassResourceCache();
            auto framebuffer = cache.GetOrCreateFramebuffer("TransparentPass", fbDesc, nvDevice);
            if (!framebuffer)
                return;

            if (!data.passState->initialized)
                return;

            using namespace fg::bindless;
            auto& matBuffer = MaterialBuffer::Instance();
            auto staticGlobalsCB = cache.GetOrCreateVolatileCB("Frame", "StaticGlobals", sizeof(StaticGlobals), data.device);
            auto drawIndexBuffer = GetOrCreateDrawIndexBuffer("TransparentPass", nvDevice);

            const auto& cfg = data.config;

            auto* shaderLoader = GEnv.Render->GetShaderLoader();
            auto* vsReflection = shaderLoader->GetCachedReflection("bindless_forward", ".vs");
            auto* psReflection = shaderLoader->GetCachedReflection("bindless_forward", ".ps");
            auto* distortReflection = shaderLoader->GetCachedReflection("bindless_forward_distort", ".ps");
            if (!vsReflection || !psReflection || !distortReflection)
                return;

            nvrhi::IBuffer* localTiles = nullptr;
            nvrhi::ITexture* localStatic = nullptr;
            nvrhi::ITexture* localDyn = nullptr;
            nvrhi::ITexture* localHud = nullptr;
            ResolveLocalShadowBindings(fg, data.localShadow, nvDevice, localTiles, localStatic, localDyn, localHud);
            nvrhi::ITexture* sunMaskTex = ResolveSunMask(fg, data.sunMask, nvDevice);

            auto makeColorBindings = [&](nvrhi::IBuffer* instanceBuffer, const char* name) -> nvrhi::IBindingSet* {
                framegraph::BindingSetBuilder bsb(*vsReflection, *psReflection, nvDevice, name);
                bsb.ConstantBuffer("static_globals", staticGlobalsCB);
                bsb.BufferSRV("g_Materials", matBuffer.GetBuffer());
                bsb.BufferSRV("g_Variants", VariantBuffer::Instance().GetBuffer());
                bsb.BufferSRV("g_InstanceData", instanceBuffer);
                bsb.BufferSRV("g_LightData", ClusteredLightManager::Instance().GetLightDataBuffer());
                bsb.BufferSRV("g_ClusterGrid", ClusteredLightManager::Instance().GetClusterGridBuffer());
                bsb.BufferSRV("g_LightIndexList", ClusteredLightManager::Instance().GetLightIndexListBuffer());
                bsb.BufferSRV("g_LocalShadowTiles", localTiles);
                bsb.Texture("g_LocalShadowStatic", localStatic);
                bsb.Texture("g_LocalShadowDyn", localDyn);
                bsb.Texture("g_LocalShadowHud", localHud);
                bsb.Texture("g_SunShadowMask", sunMaskTex);
                auto set = cache.GetOrCreateBindingSet(bsb.Build(), data.passState->layout, nvDevice);
                R_ASSERT2(set, "Transparent binding set creation failed");
                return set;
            };
            auto makeDistortBindings = [&](nvrhi::IBuffer* instanceBuffer, const char* name) -> nvrhi::IBindingSet* {
                framegraph::BindingSetBuilder bsb(*vsReflection, *distortReflection, nvDevice, name);
                bsb.ConstantBuffer("static_globals", staticGlobalsCB);
                bsb.BufferSRV("g_Materials", matBuffer.GetBuffer());
                bsb.BufferSRV("g_Variants", VariantBuffer::Instance().GetBuffer());
                bsb.BufferSRV("g_VariantTextures", VariantTextureBuffer::Instance().GetBuffer());
                bsb.BufferSRV("g_InstanceData", instanceBuffer);
                auto set = cache.GetOrCreateBindingSet(bsb.Build(), data.passState->distortLayout, nvDevice);
                R_ASSERT2(set, "Transparent distortion binding set creation failed");
                return set;
            };

            auto* backend = data.device->GetBackend();
            nvrhi::IBindingSet* bindlessTable = backend ? backend->GetBindlessDescriptorTable() : nullptr;
            const auto& rtDesc = colorRT->getDesc();
            nvrhi::Viewport viewport(0.0f, static_cast<float>(rtDesc.width), 0.0f, static_cast<float>(rtDesc.height), 0.0f, 1.0f);
            nvrhi::Rect scissor(rtDesc.width, rtDesc.height);

            struct DrawSource {
                nvrhi::IBuffer* vertexBuffer;
                nvrhi::IBuffer* indexBuffer;
                nvrhi::IBuffer* instanceBuffer;
                nvrhi::IBuffer* drawArgs;
                const xr_vector<TransparentDrawRange>* ranges;
                const char* name;
            };
            DrawSource sources[2] = {};
            u32 sourceCount = 0;
            if (cfg.HasRigid())
                sources[sourceCount++] = { cfg.megaVertexBuffer, cfg.megaIndexBuffer, cfg.instanceBuffer, cfg.drawArgsBuffer, cfg.ranges, "Transparent" };
            if (cfg.skinned && cfg.gpuCulling && cfg.gpuCulling->GetSkinnedForwardCount() > 0) {
                GPUCullingManager& gc = *cfg.gpuCulling;
                nvrhi::IBuffer* preVB = gc.GetSkinnedPreVertexBuffer();
                nvrhi::IBuffer* skinnedIB = gc.GetSkinnedPools().GetCombinedIndexBuffer();
                if (preVB && skinnedIB && gc.GetSkinnedForwardArgsBuffer() && gc.GetSkinnedForwardInstanceBuffer())
                    sources[sourceCount++] = { preVB, skinnedIB, gc.GetSkinnedForwardInstanceBuffer(), gc.GetSkinnedForwardArgsBuffer(), &gc.GetSkinnedForwardRanges(), "Transparent.Skinned" };
            }

            auto drawRanges = [&](const DrawSource& src, nvrhi::IFramebuffer* fb, nvrhi::IBindingSet* bindings, bool distortPass) {
                for (const auto& range : *src.ranges) {
                    if (range.count == 0 || (range.key & TRANSPARENT_KEY_WMARK))
                        continue;
                    if (distortPass ? (range.key & TRANSPARENT_KEY_DISTORT) == 0 : (range.key & TRANSPARENT_KEY_NO_COLOR) != 0)
                        continue;
                    nvrhi::IGraphicsPipeline* pipeline = distortPass
                        ? data.passState->distortPipeline.Get()
                        : GetColorPipeline(data.device, *data.passState, range.key);
                    if (!pipeline)
                        continue;
                    nvrhi::GraphicsState gs;
                    gs.pipeline = pipeline;
                    gs.framebuffer = fb;
                    gs.bindings = { bindings };
                    if (bindlessTable)
                        gs.addBindingSet(bindlessTable);
                    gs.vertexBuffers = {
                        {src.vertexBuffer, 0, 0},
                        {drawIndexBuffer, 1, 0}
                    };
                    gs.indexBuffer = { src.indexBuffer, nvrhi::Format::R32_UINT, 0 };
                    gs.indirectParams = src.drawArgs;
                    gs.viewport.addViewport(viewport);
                    gs.viewport.addScissorRect(scissor);
                    cmdList->setGraphicsState(gs);
                    cmdList->drawIndexedIndirect(range.first * sizeof(IndirectDrawArgs), range.count);
                }
            };

            for (u32 i = 0; i < sourceCount; ++i)
                drawRanges(sources[i], framebuffer, makeColorBindings(sources[i].instanceBuffer, sources[i].name), false);

            if (!data.distortion.is_valid())
                return;
            auto* distortRT = fg.GetPhysicalTexture(data.distortion);
            if (!distortRT)
                return;
            nvrhi::FramebufferDesc distortFbDesc;
            distortFbDesc.addColorAttachment(distortRT);
            distortFbDesc.setDepthAttachment(depthRT);
            auto distortFB = cache.GetOrCreateFramebuffer("TransparentPass_Distort", distortFbDesc, nvDevice);
            if (!distortFB)
                return;
            cmdList->clearTextureFloat(distortRT, nvrhi::AllSubresources, nvrhi::Color(0.f, 0.f, 0.f, 0.f));
            for (u32 i = 0; i < sourceCount; ++i)
                drawRanges(sources[i], distortFB, makeDistortBindings(sources[i].instanceBuffer, sources[i].name), true);
        }
    );

    DefaultOutputLayout outputs;
    outputs.albedo = passData.color;
    outputs.normal = passData.normal;
    outputs.baseColor = passData.baseColor;
    outputs.depth = passData.depth;
    outputs.distortion = passData.distortion.is_valid() ? passData.distortion : inputs.distortion;
    return outputs;
}

static bool RangesWantWallmarks(const xr_vector<TransparentDrawRange>* ranges)
{
    if (!ranges)
        return false;
    for (const auto& range : *ranges)
        if (range.key & TRANSPARENT_KEY_WMARK)
            return true;
    return false;
}

struct StaticWallmarkPassData {
    framegraph::VirtualResourceHandle depth;
    framegraph::VirtualResourceHandle baseColor;
    fg::RenderDevice* device;
    TransparentPassConfig config;
    TransparentPassState* passState;
    u32 width, height;
};

framegraph::DefaultOutputLayout setupStaticWallmarkPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    const framegraph::DefaultOutputLayout& inputs,
    const TransparentPassConfig& config,
    u32 width, u32 height,
    TransparentPassState& state)
{
    using namespace framegraph;

    if (!config.HasRigid() || !RangesWantWallmarks(config.ranges) || !inputs.baseColor.is_valid())
        return inputs;

    InitializeTransparentResources(device, TransparentFramebufferInfo(), state);

    auto& passData = fg.addCallbackPass<StaticWallmarkPassData>(
        "Static Wallmarks",

        [&, width, height, config](FrameGraph& builder, PassHandle passHandle, StaticWallmarkPassData& data) {
            data.width = width;
            data.height = height;
            data.device = device;
            data.config = config;
            data.passState = &state;

            RenderPassBuilder passBuilder(builder, passHandle);
            data.depth = passBuilder.read(inputs.depth, ResourceState::DepthStencilRead);
            data.baseColor = passBuilder.readWrite(inputs.baseColor, ResourceState::RenderTarget);
        },

        [](const StaticWallmarkPassData& data,
            const FrameGraph& fg,
            fg::RenderContext* ctx) {

            auto* baseColorRT = fg.GetPhysicalTexture(data.baseColor);
            auto* depthRT = fg.GetPhysicalTexture(data.depth);
            if (!baseColorRT || !depthRT || !data.passState->initialized)
                return;

            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            if (!nvDevice || !cmdList)
                return;

            auto& cache = framegraph::GetPassResourceCache();
            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(baseColorRT);
            fbDesc.setDepthAttachment(depthRT);
            auto framebuffer = cache.GetOrCreateFramebuffer("StaticWallmarkPass", fbDesc, nvDevice);
            if (!framebuffer)
                return;

            auto* shaderLoader = GEnv.Render->GetShaderLoader();
            auto* vsReflection = shaderLoader->GetCachedReflection("bindless_forward", ".vs");
            auto* psReflection = shaderLoader->GetCachedReflection("bindless_wallmark", ".ps");
            if (!vsReflection || !psReflection)
                return;

            using namespace fg::bindless;
            auto staticGlobalsCB = cache.GetOrCreateVolatileCB("Frame", "StaticGlobals", sizeof(StaticGlobals), data.device);
            auto drawIndexBuffer = GetOrCreateDrawIndexBuffer("TransparentPass", nvDevice);
            const auto& cfg = data.config;

            framegraph::BindingSetBuilder bsb(*vsReflection, *psReflection, nvDevice, "StaticWallmark");
            bsb.ConstantBuffer("static_globals", staticGlobalsCB);
            bsb.BufferSRV("g_Materials", MaterialBuffer::Instance().GetBuffer());
            bsb.BufferSRV("g_Variants", VariantBuffer::Instance().GetBuffer());
            bsb.BufferSRV("g_InstanceData", cfg.instanceBuffer);
            nvrhi::IBindingSet* bindings = cache.GetOrCreateBindingSet(bsb.Build(), data.passState->wallmarkLayout, nvDevice);
            R_ASSERT2(bindings, "Static wallmark binding set creation failed");

            auto* backend = data.device->GetBackend();
            nvrhi::IBindingSet* bindlessTable = backend ? backend->GetBindlessDescriptorTable() : nullptr;
            const auto& rtDesc = baseColorRT->getDesc();
            nvrhi::Viewport viewport(0.0f, static_cast<float>(rtDesc.width), 0.0f, static_cast<float>(rtDesc.height), 0.0f, 1.0f);
            nvrhi::Rect scissor(rtDesc.width, rtDesc.height);

            for (const auto& range : *cfg.ranges) {
                if (range.count == 0 || (range.key & TRANSPARENT_KEY_WMARK) == 0)
                    continue;
                nvrhi::IGraphicsPipeline* pipeline = GetWallmarkPipeline(data.device, *data.passState, range.key);
                if (!pipeline)
                    continue;
                nvrhi::GraphicsState gs;
                gs.pipeline = pipeline;
                gs.framebuffer = framebuffer;
                gs.bindings = { bindings };
                if (bindlessTable)
                    gs.addBindingSet(bindlessTable);
                gs.vertexBuffers = {
                    {cfg.megaVertexBuffer, 0, 0},
                    {drawIndexBuffer, 1, 0}
                };
                gs.indexBuffer = { cfg.megaIndexBuffer, nvrhi::Format::R32_UINT, 0 };
                gs.indirectParams = cfg.drawArgsBuffer;
                gs.viewport.addViewport(viewport);
                gs.viewport.addScissorRect(scissor);
                cmdList->setGraphicsState(gs);
                cmdList->drawIndexedIndirect(range.first * sizeof(IndirectDrawArgs), range.count);
            }
        }
    );

    DefaultOutputLayout outputs = inputs;
    outputs.baseColor = passData.baseColor;
    outputs.depth = passData.depth;
    return outputs;
}

} // namespace xray::render::fg::passes
