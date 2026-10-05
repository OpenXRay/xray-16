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

bool TransparentPassConfig::HasRigid() const
{
    return objectCount > 0 && ranges && geometry.forwardArgs.is_valid() && geometry.forwardInstances.is_valid()
        && geometry.megaVertices.is_valid() && geometry.megaIndices.is_valid();
}

bool TransparentPassConfig::IsValid() const
{
    return HasRigid() || (skinned && gpuCulling);
}

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

static nvrhi::IGraphicsPipeline* GetColorPipeline(fg::RenderDevice* device, TransparentPassState& state, u32 key, bool skinned)
{
    const u64 pipelineKey = u64(key) | (u64(skinned) << 32);
    auto it = state.pipelines.find(pipelineKey);
    if (it != state.pipelines.end())
        return it->second.Get();

    nvrhi::GraphicsPipelineDesc desc = MakeBasePipelineDesc(device, state, state.ps, state.layout);
    if (skinned)
        desc.inputLayout = state.skinnedInputLayout;
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
    xr_sprintf(name, "TransparentPass_%08x_%u", key, u32(skinned));
    auto& cache = framegraph::GetPassResourceCache();
    nvrhi::GraphicsPipelineHandle pipeline = cache.GetOrCreatePipeline(name, desc, state.fbInfo, device->GetNVRHIDevice());
    state.pipelines[pipelineKey] = pipeline;
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

static nvrhi::FramebufferInfoEx TransparentFramebufferInfo(framegraph::FrameGraph& fg, const framegraph::DefaultOutputLayout& inputs)
{
    nvrhi::FramebufferInfoEx fbInfo;
    fbInfo.colorFormats.push_back(fg.GetResourceDesc(inputs.albedo).format);
    if (inputs.normal.is_valid())
        fbInfo.colorFormats.push_back(fg.GetResourceDesc(inputs.normal).format);
    if (inputs.baseColor.is_valid())
        fbInfo.colorFormats.push_back(fg.GetResourceDesc(inputs.baseColor).format);
    fbInfo.depthFormat = fg.GetResourceDesc(inputs.depth).format;
    return fbInfo;
}

static nvrhi::FramebufferInfoEx WallmarkFramebufferInfo(framegraph::FrameGraph& fg, const framegraph::DefaultOutputLayout& inputs)
{
    nvrhi::FramebufferInfoEx fbInfo;
    fbInfo.colorFormats.push_back(fg.GetResourceDesc(inputs.baseColor).format);
    fbInfo.depthFormat = fg.GetResourceDesc(inputs.depth).format;
    return fbInfo;
}

void InitializeTransparentResources(fg::RenderDevice* device, TransparentPassState& state)
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

    auto& cache = framegraph::GetPassResourceCache();
    state.layout = cache.GetOrCreateBindingLayoutFromReflection("TransparentPass", *vsResult.reflection, *psResult.reflection, nvDevice);
    state.distortLayout = cache.GetOrCreateBindingLayoutFromReflection("TransparentPass_Distort", *vsResult.reflection, *distortResult.reflection, nvDevice);
    state.wallmarkLayout = cache.GetOrCreateBindingLayoutFromReflection("StaticWallmarkPass", *vsResult.reflection, *wallmarkResult.reflection, nvDevice);
    if (!state.layout || !state.distortLayout || !state.wallmarkLayout)
        return;

    u32 attrCount = 0;
    auto* attrs = GetUnifiedVertexAttributes(attrCount);
    nvrhi::VertexAttributeDesc attributes[8];
    R_ASSERT(attrCount + 1u == std::size(attributes));
    std::copy_n(attrs, attrCount, attributes);
    attributes[attrCount] = nvrhi::VertexAttributeDesc()
        .setName("EXACTNORMAL").setFormat(nvrhi::Format::RGB32_FLOAT)
        .setBufferIndex(0).setOffset(0).setElementStride(sizeof(bindless::UnifiedVertex));
    state.skinnedInputLayout = nvDevice->createInputLayout(attributes, u32(std::size(attributes)), state.vs);
    for (auto& attribute : attributes)
    {
        if (attribute.bufferIndex == 0)
            attribute.elementStride = sizeof(ForwardVertex);
    }
    attributes[attrCount].offset = offsetof(ForwardVertex, normal);
    state.inputLayout = nvDevice->createInputLayout(attributes, u32(std::size(attributes)), state.vs);
    R_ASSERT(state.inputLayout && state.skinnedInputLayout);


    nvrhi::FramebufferInfoEx distortFbInfo;
    distortFbInfo.colorFormats.push_back(framegraph::kSceneDistortionFormat);
    distortFbInfo.depthFormat = framegraph::kSceneDepthFormat;
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
    distortDesc.inputLayout = state.skinnedInputLayout;
    state.skinnedDistortPipeline = cache.GetOrCreatePipeline("TransparentPass_SkinnedDistort", distortDesc, distortFbInfo, nvDevice);
    if (!state.distortPipeline || !state.skinnedDistortPipeline)
        return;
    QueryBindingLayoutFromPipeline(state.distortPipeline, state.distortLayout);

    state.initialized = true;
    Msg("* [TransparentPass] Pipeline initialized");
}

static bool BindTransparentTargets(fg::RenderDevice* device, TransparentPassState& state, const nvrhi::FramebufferInfoEx& fbInfo)
{
    if (!state.initialized)
        return false;
    if (state.fbInfo == fbInfo && !state.pipelines.empty())
        return true;

    state.fbInfo = fbInfo;
    state.pipelines.clear();
    const u32 defaultKey = u32(VariantBlendFactor::SrcAlpha) | (u32(VariantBlendFactor::InvSrcAlpha) << TRANSPARENT_KEY_DST_SHIFT);
    if (!GetColorPipeline(device, state, defaultKey, false) || !GetColorPipeline(device, state, defaultKey, true))
        return false;
    QueryBindingLayoutFromPipeline(state.pipelines[defaultKey], state.layout);
    return true;
}

static bool BindWallmarkTargets(fg::RenderDevice* device, TransparentPassState& state, const nvrhi::FramebufferInfoEx& fbInfo)
{
    if (!state.initialized)
        return false;
    if (state.wallmarkFbInfo == fbInfo && !state.wallmarkPipelines.empty())
        return true;

    state.wallmarkFbInfo = fbInfo;
    state.wallmarkPipelines.clear();
    const u32 wallmarkKey = u32(VariantBlendFactor::DstColor) | (u32(VariantBlendFactor::SrcColor) << TRANSPARENT_KEY_DST_SHIFT) | TRANSPARENT_KEY_WMARK;
    if (!GetWallmarkPipeline(device, state, wallmarkKey))
        return false;
    QueryBindingLayoutFromPipeline(state.wallmarkPipelines[wallmarkKey], state.wallmarkLayout);
    return true;
}

void WarmTransparentPipelines(fg::RenderDevice* device, TransparentPassState& state)
{
    InitializeTransparentResources(device, state);

    nvrhi::FramebufferInfoEx fbInfo;
    fbInfo.colorFormats.push_back(framegraph::kSceneColorFormat);
    fbInfo.colorFormats.push_back(framegraph::kSceneNormalFormat);
    fbInfo.colorFormats.push_back(framegraph::kSceneBaseColorFormat);
    fbInfo.depthFormat = framegraph::kSceneDepthFormat;

    nvrhi::FramebufferInfoEx wallmarkFbInfo;
    wallmarkFbInfo.colorFormats.push_back(framegraph::kSceneBaseColorFormat);
    wallmarkFbInfo.depthFormat = framegraph::kSceneDepthFormat;

    if (!BindTransparentTargets(device, state, fbInfo) || !BindWallmarkTargets(device, state, wallmarkFbInfo))
        return;

    const ShaderVariantRegistry& variants = ShaderVariantRegistry::Instance();
    for (u32 index = 0; index < variants.GetVariantCount(); ++index)
    {
        const u32 key = TransparentKeyForVariant(variants.GetVariantByIndex(index));
        if (key & TRANSPARENT_KEY_WMARK)
            GetWallmarkPipeline(device, state, key);
        else if ((key & TRANSPARENT_KEY_NO_COLOR) == 0)
        {
            GetColorPipeline(device, state, key, false);
            GetColorPipeline(device, state, key, true);
        }
    }
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
    const ClusterLightOutput& clusterLights,
    const SkyEnvironmentFrame& sky,
    framegraph::VirtualResourceHandle sunMask,
    framegraph::VirtualResourceHandle skinnedOrder,
    u32 width, u32 height,
    TransparentPassState& state)
{
    using namespace framegraph;

    if (!config.IsValid()) {
        return inputs;
    }

    InitializeTransparentResources(device, state);
    if (!BindTransparentTargets(device, state, TransparentFramebufferInfo(fg, inputs)))
        return inputs;

    const bool wantDistortion = RangesWantDistortion(config.ranges)
        || (config.skinned && config.gpuCulling && RangesWantDistortion(&config.gpuCulling->GetSkinnedForwardRanges()));

    auto& passData = fg.addCallbackPass<TransparentPassData>(
        "Transparent Pass",

        [&, width, height, config, localShadow, clusterLights, sky, sunMask, skinnedOrder, wantDistortion](FrameGraph& builder, PassHandle passHandle, TransparentPassData& data) {
            data.width = width;
            data.height = height;
            data.device = device;
            data.config = config;
            data.passState = &state;

            RenderPassBuilder passBuilder(builder, passHandle);
            passBuilder.read(config.geometry.forwardDrawIndices, ResourceState::VertexBuffer);
            if (config.HasRigid())
            {
                passBuilder.read(config.geometry.megaVertices, ResourceState::VertexBuffer);
                passBuilder.read(config.geometry.megaIndices, ResourceState::IndexBuffer);
                passBuilder.read(config.geometry.forwardArgs, ResourceState::IndirectArgument);
                passBuilder.read(config.geometry.forwardInstances, ResourceState::ShaderResource);
            }
            if (config.skinned)
            {
                passBuilder.read(config.geometry.deformedVertices, ResourceState::VertexBuffer);
                passBuilder.read(config.geometry.skinnedIndices, ResourceState::IndexBuffer);
                passBuilder.read(config.geometry.skinnedForwardArgs, ResourceState::IndirectArgument);
                passBuilder.read(config.geometry.skinnedForwardInstances, ResourceState::ShaderResource);
            }
            for (const auto handle : { config.geometry.materials, config.geometry.variants, config.geometry.variantTextures })
            {
                if (handle.is_valid())
                    passBuilder.read(handle, ResourceState::ShaderResource);
            }
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
                distDesc.format = framegraph::kSceneDistortionFormat;
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
            }
            if (clusterLights.active) {
                data.clusterLightData = passBuilder.read(clusterLights.lightData, ResourceState::ShaderResource);
                data.clusterGrid = passBuilder.read(clusterLights.clusterGrid, ResourceState::ShaderResource);
                data.clusterLightIndexList = passBuilder.read(clusterLights.lightIndexList, ResourceState::ShaderResource);
            }
            if (sky.irradiance.is_valid())
                data.skyIrradiance = passBuilder.read(sky.irradiance, ResourceState::ShaderResource);
            if (sky.specular.is_valid())
                data.skySpecular = passBuilder.read(sky.specular, ResourceState::ShaderResource);
            if (sky.dfg.is_valid())
                data.skyDFG = passBuilder.read(sky.dfg, ResourceState::ShaderResource);
            if (sky.probes.is_valid())
                data.skyProbes = passBuilder.read(sky.probes, ResourceState::ShaderResource);
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

            if (data.config.lighting && data.config.lighting->effective == LightingMode::ReferencePT && data.config.lighting->recorded)
            {
                if (data.distortion.is_valid())
                {
                    if (auto* distortion = fg.GetPhysicalTexture(data.distortion))
                        cmdList->clearTextureFloat(distortion, nvrhi::AllSubresources, nvrhi::Color(0.f));
                }
                return;
            }

            auto* baseColorRT = data.baseColor.is_valid() ? fg.GetPhysicalTexture(data.baseColor) : nullptr;

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(colorRT);
            if (normalRT)
                fbDesc.addColorAttachment(normalRT);
            if (baseColorRT)
                fbDesc.addColorAttachment(baseColorRT);
            fbDesc.setDepthAttachment(depthRT);
            auto& cache = framegraph::GetPassResourceCache();
            auto framebuffer = cache.GetOrCreateFramebuffer(fbDesc, nvDevice);
            if (!framebuffer)
                return;

            if (!data.passState->initialized)
                return;

            using namespace fg::bindless;
            auto staticGlobalsCB = cache.GetOrCreateVolatileCB("Frame", "StaticGlobals", sizeof(StaticGlobals), data.device);
            auto* drawIndexBuffer = fg.GetPhysicalBuffer(data.config.geometry.forwardDrawIndices);

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
            ResolveLocalShadowBindings(fg, data.localShadow, nvDevice, localTiles, localStatic, localDyn);
            nvrhi::ITexture* sunMaskTex = ResolveSunMask(fg, data.sunMask, nvDevice);
            nvrhi::IBuffer* skyIrradiance = data.skyIrradiance.is_valid() ? fg.GetPhysicalBuffer(data.skyIrradiance) : nullptr;
            nvrhi::ITexture* skySpecular = data.skySpecular.is_valid() ? fg.GetPhysicalTexture(data.skySpecular) : nullptr;
            nvrhi::ITexture* skyDFG = data.skyDFG.is_valid() ? fg.GetPhysicalTexture(data.skyDFG) : nullptr;
            nvrhi::IBuffer* skyProbes = data.skyProbes.is_valid() ? fg.GetPhysicalBuffer(data.skyProbes) : nullptr;

            auto makeColorBindings = [&](nvrhi::IBuffer* instanceBuffer, const char* name) -> nvrhi::IBindingSet* {
                framegraph::BindingSetBuilder bsb(*vsReflection, *psReflection, nvDevice, name);
                bsb.ConstantBuffer("static_globals", staticGlobalsCB);
                bsb.BufferSRV("g_Materials", fg.GetPhysicalBuffer(cfg.geometry.materials));
                bsb.BufferSRV("g_Variants", fg.GetPhysicalBuffer(cfg.geometry.variants));
                bsb.BufferSRV("g_InstanceData", instanceBuffer);
                bsb.BufferSRV("g_LightData", ClusteredLightManager::Instance().GetLightDataBuffer());
                bsb.BufferSRV("g_ClusterGrid", ClusteredLightManager::Instance().GetClusterGridBuffer());
                bsb.BufferSRV("g_LightIndexList", ClusteredLightManager::Instance().GetLightIndexListBuffer());
                bsb.BufferSRV("g_LocalShadowTiles", localTiles);
                bsb.Texture("g_LocalShadowStatic", localStatic);
                bsb.Texture("g_LocalShadowDyn", localDyn);
                bsb.Texture("g_SunShadowMask", sunMaskTex);
                bsb.BufferSRV("g_SkyIrradiance", skyIrradiance);
                bsb.Texture("g_SkySpecular", skySpecular);
                bsb.Texture("g_SkyDFG", skyDFG);
                bsb.BufferSRV("g_SkyProbes", skyProbes);
                auto set = cache.GetOrCreateBindingSet(bsb.Build(), data.passState->layout, nvDevice);
                R_ASSERT2(set, "Transparent binding set creation failed");
                return set;
            };
            auto makeDistortBindings = [&](nvrhi::IBuffer* instanceBuffer, const char* name) -> nvrhi::IBindingSet* {
                framegraph::BindingSetBuilder bsb(*vsReflection, *distortReflection, nvDevice, name);
                bsb.ConstantBuffer("static_globals", staticGlobalsCB);
                bsb.BufferSRV("g_Materials", fg.GetPhysicalBuffer(cfg.geometry.materials));
                bsb.BufferSRV("g_Variants", fg.GetPhysicalBuffer(cfg.geometry.variants));
                bsb.BufferSRV("g_VariantTextures", fg.GetPhysicalBuffer(cfg.geometry.variantTextures));
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
                bool skinned;
            };
            DrawSource sources[2] = {};
            u32 sourceCount = 0;
            if (cfg.HasRigid())
                sources[sourceCount++] = { fg.GetPhysicalBuffer(cfg.geometry.megaVertices),
                    fg.GetPhysicalBuffer(cfg.geometry.megaIndices), fg.GetPhysicalBuffer(cfg.geometry.forwardInstances),
                    fg.GetPhysicalBuffer(cfg.geometry.forwardArgs), cfg.ranges, "Transparent", false };
            if (cfg.skinned && cfg.gpuCulling && cfg.gpuCulling->GetSkinnedForwardCount() > 0) {
                GPUCullingManager& gc = *cfg.gpuCulling;
                nvrhi::IBuffer* preVB = fg.GetPhysicalBuffer(cfg.geometry.deformedVertices);
                nvrhi::IBuffer* skinnedIB = fg.GetPhysicalBuffer(cfg.geometry.skinnedIndices);
                nvrhi::IBuffer* args = fg.GetPhysicalBuffer(cfg.geometry.skinnedForwardArgs);
                nvrhi::IBuffer* instances = fg.GetPhysicalBuffer(cfg.geometry.skinnedForwardInstances);
                if (!preVB || !skinnedIB || !args || !instances)
                    FATAL("[FrameGraph] prepared skinned forward geometry is unavailable");
                sources[sourceCount++] = { preVB, skinnedIB, instances, args,
                    &gc.GetSkinnedForwardRanges(), "Transparent.Skinned", true };
            }

            auto drawRanges = [&](const DrawSource& src, nvrhi::IFramebuffer* fb, nvrhi::IBindingSet* bindings, bool distortPass) {
                for (const auto& range : *src.ranges) {
                    if (range.count == 0 || (range.key & TRANSPARENT_KEY_WMARK))
                        continue;
                    if (distortPass ? (range.key & TRANSPARENT_KEY_DISTORT) == 0 : (range.key & TRANSPARENT_KEY_NO_COLOR) != 0)
                        continue;
                    nvrhi::IGraphicsPipeline* pipeline = distortPass
                        ? (src.skinned ? data.passState->skinnedDistortPipeline.Get() : data.passState->distortPipeline.Get())
                        : GetColorPipeline(data.device, *data.passState, range.key, src.skinned);
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
            auto distortFB = cache.GetOrCreateFramebuffer(distortFbDesc, nvDevice);
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

    InitializeTransparentResources(device, state);
    if (!BindWallmarkTargets(device, state, WallmarkFramebufferInfo(fg, inputs)))
        return inputs;

    auto& passData = fg.addCallbackPass<StaticWallmarkPassData>(
        "Static Wallmarks",

        [&, width, height, config](FrameGraph& builder, PassHandle passHandle, StaticWallmarkPassData& data) {
            data.width = width;
            data.height = height;
            data.device = device;
            data.config = config;
            data.passState = &state;

            RenderPassBuilder passBuilder(builder, passHandle);
            passBuilder.read(config.geometry.megaVertices, ResourceState::VertexBuffer);
            passBuilder.read(config.geometry.megaIndices, ResourceState::IndexBuffer);
            passBuilder.read(config.geometry.forwardArgs, ResourceState::IndirectArgument);
            passBuilder.read(config.geometry.forwardInstances, ResourceState::ShaderResource);
            passBuilder.read(config.geometry.forwardDrawIndices, ResourceState::VertexBuffer);
            passBuilder.read(config.geometry.materials, ResourceState::ShaderResource);
            passBuilder.read(config.geometry.variants, ResourceState::ShaderResource);
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
            auto framebuffer = cache.GetOrCreateFramebuffer(fbDesc, nvDevice);
            if (!framebuffer)
                return;

            auto* shaderLoader = GEnv.Render->GetShaderLoader();
            auto* vsReflection = shaderLoader->GetCachedReflection("bindless_forward", ".vs");
            auto* psReflection = shaderLoader->GetCachedReflection("bindless_wallmark", ".ps");
            if (!vsReflection || !psReflection)
                return;

            using namespace fg::bindless;
            auto staticGlobalsCB = cache.GetOrCreateVolatileCB("Frame", "StaticGlobals", sizeof(StaticGlobals), data.device);
            auto* drawIndexBuffer = fg.GetPhysicalBuffer(data.config.geometry.forwardDrawIndices);
            const auto& cfg = data.config;

            framegraph::BindingSetBuilder bsb(*vsReflection, *psReflection, nvDevice, "StaticWallmark");
            bsb.ConstantBuffer("static_globals", staticGlobalsCB);
            bsb.BufferSRV("g_Materials", fg.GetPhysicalBuffer(cfg.geometry.materials));
            bsb.BufferSRV("g_Variants", fg.GetPhysicalBuffer(cfg.geometry.variants));
            bsb.BufferSRV("g_InstanceData", fg.GetPhysicalBuffer(cfg.geometry.forwardInstances));
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
                    {fg.GetPhysicalBuffer(cfg.geometry.megaVertices), 0, 0},
                    {drawIndexBuffer, 1, 0}
                };
                gs.indexBuffer = { fg.GetPhysicalBuffer(cfg.geometry.megaIndices), nvrhi::Format::R32_UINT, 0 };
                gs.indirectParams = fg.GetPhysicalBuffer(cfg.geometry.forwardArgs);
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
