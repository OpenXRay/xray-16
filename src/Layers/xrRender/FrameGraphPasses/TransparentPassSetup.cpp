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

namespace xray::render::RENDER_NAMESPACE::passes {

static nvrhi::GraphicsPipelineHandle s_transparentPipeline;
static nvrhi::BindingLayoutHandle s_transparentLayout;
static nvrhi::InputLayoutHandle s_transparentInputLayout;
static nvrhi::SamplerHandle s_transparentSampler;
static nvrhi::ShaderHandle s_transparentVS;
static nvrhi::ShaderHandle s_transparentPS;
static nvrhi::BufferHandle s_transparentDrawIndexBuffer;
static bool s_transparentInitialized = false;

static void InitializeTransparentResources(ng::RenderDevice* device, nvrhi::IFramebuffer* framebuffer)
{
    if (s_transparentInitialized)
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

    s_transparentVS = vsResult.handle;
    s_transparentPS = psResult.handle;

    nvrhi::SamplerDesc samplerDesc;
    samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Repeat);
    samplerDesc.setAllFilters(true);
    samplerDesc.setMaxAnisotropy(16.0f);
    s_transparentSampler = nvDevice->createSampler(samplerDesc);

    nvrhi::BindingLayoutDesc layoutDesc;
    layoutDesc.visibility = nvrhi::ShaderType::All;
    layoutDesc.bindings = {
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(2),  // static_globals (b2)
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(4),  // LightingConstants (b4)
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(8),    // g_Materials
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(10),   // g_VariantTextures
        nvrhi::BindingLayoutItem::Sampler(0),
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(14),   // g_InstanceData
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(15),   // g_CompactBatchIndices
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(16),   // g_CompactMaterialIDs
    };
    s_transparentLayout = nvDevice->createBindingLayout(layoutDesc);
    if (!s_transparentLayout)
        return;

    constexpr u32 vertexStride = 48;
    nvrhi::VertexAttributeDesc vertexAttribs[] = {
        nvrhi::VertexAttributeDesc()
            .setName("POSITION").setFormat(nvrhi::Format::RGB32_FLOAT)
            .setBufferIndex(0).setOffset(0).setElementStride(vertexStride),
        nvrhi::VertexAttributeDesc()
            .setName("NORMAL").setFormat(nvrhi::Format::BGRA8_UNORM)
            .setBufferIndex(0).setOffset(12).setElementStride(vertexStride),
        nvrhi::VertexAttributeDesc()
            .setName("TANGENT").setFormat(nvrhi::Format::BGRA8_UNORM)
            .setBufferIndex(0).setOffset(16).setElementStride(vertexStride),
        nvrhi::VertexAttributeDesc()
            .setName("BINORMAL").setFormat(nvrhi::Format::BGRA8_UNORM)
            .setBufferIndex(0).setOffset(20).setElementStride(vertexStride),
        nvrhi::VertexAttributeDesc()
            .setName("TEXCOORD").setFormat(nvrhi::Format::RG32_FLOAT)
            .setArraySize(2).setBufferIndex(0).setOffset(24).setElementStride(vertexStride),
        nvrhi::VertexAttributeDesc()
            .setName("COLOR").setFormat(nvrhi::Format::BGRA8_UNORM)
            .setBufferIndex(0).setOffset(40).setElementStride(vertexStride),
        nvrhi::VertexAttributeDesc()
            .setName("DRAWINDEX").setFormat(nvrhi::Format::R32_UINT)
            .setBufferIndex(1).setOffset(0).setElementStride(4).setIsInstanced(true),
    };
    s_transparentInputLayout = nvDevice->createInputLayout(vertexAttribs, 7, s_transparentVS);

    constexpr u32 MAX_DRAWS = 65536;
    xr_vector<u32> drawIndices(MAX_DRAWS);
    for (u32 i = 0; i < MAX_DRAWS; i++)
        drawIndices[i] = i;

    nvrhi::BufferDesc drawIndexDesc;
    drawIndexDesc.byteSize = MAX_DRAWS * sizeof(u32);
    drawIndexDesc.structStride = sizeof(u32);
    drawIndexDesc.isVertexBuffer = true;
    drawIndexDesc.debugName = "TransparentDrawIndexBuffer";
    drawIndexDesc.initialState = nvrhi::ResourceStates::VertexBuffer;
    drawIndexDesc.keepInitialState = true;
    s_transparentDrawIndexBuffer = nvDevice->createBuffer(drawIndexDesc);
    if (!s_transparentDrawIndexBuffer)
        return;

    if (GEnv.Backend)
        GEnv.Backend->UploadBufferData(s_transparentDrawIndexBuffer, drawIndices.data(), MAX_DRAWS * sizeof(u32));

    nvrhi::GraphicsPipelineDesc pipeDesc;
    pipeDesc.VS = s_transparentVS;
    pipeDesc.PS = s_transparentPS;
    pipeDesc.inputLayout = s_transparentInputLayout;

    auto* backend = device->GetBackend();
    nvrhi::IBindingLayout* bindlessLayout = backend ? backend->GetBindlessLayout() : nullptr;
    if (bindlessLayout)
        pipeDesc.bindingLayouts = { s_transparentLayout, bindlessLayout };
    else
        pipeDesc.bindingLayouts = { s_transparentLayout };

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

    s_transparentPipeline = nvDevice->createGraphicsPipeline(pipeDesc, framebuffer);
    if (!s_transparentPipeline)
        return;

    const nvrhi::GraphicsPipelineDesc& actualDesc = s_transparentPipeline->getDesc();
    if (!actualDesc.bindingLayouts.empty())
        s_transparentLayout = actualDesc.bindingLayouts[0];

    s_transparentInitialized = true;
    Msg("* [TransparentPass] Pipeline initialized");
}

void ShutdownTransparentPipelines()
{
    s_transparentPipeline = nullptr;
    s_transparentLayout = nullptr;
    s_transparentInputLayout = nullptr;
    s_transparentSampler = nullptr;
    s_transparentVS = nullptr;
    s_transparentPS = nullptr;
    s_transparentDrawIndexBuffer = nullptr;
    s_transparentInitialized = false;
}

framegraph::DefaultOutputLayout setupTransparentPass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    const framegraph::DefaultOutputLayout& inputs,
    const TransparentPassConfig& config,
    u32 width, u32 height)
{
    using namespace framegraph;

    if (!config.IsValid()) {
        return inputs;
    }

    struct TransparentPassData {
        VirtualResourceHandle depth;
        VirtualResourceHandle color;
        ng::RenderDevice* device;
        TransparentPassConfig config;
        u32 width, height;
    };

    auto& passData = fg.addCallbackPass<TransparentPassData>(
        "Transparent Pass",

        [&, width, height, config](FrameGraph& builder, PassHandle passHandle, TransparentPassData& data) {
            data.width = width;
            data.height = height;
            data.device = device;
            data.config = config;

            RenderPassBuilder passBuilder(builder, passHandle);
            data.color = passBuilder.readWrite(inputs.albedo, ResourceState::RenderTarget);
            data.depth = passBuilder.read(inputs.depth, ResourceState::DepthStencilRead);
        },

        [](const TransparentPassData& data,
            const FrameGraph& fg,
            ng::RenderContext* ctx) {

            auto* colorRT = fg.GetPhysicalTexture(data.color);
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
            auto framebuffer = nvDevice->createFramebuffer(fbDesc);
            if (!framebuffer)
                return;

            InitializeTransparentResources(data.device, framebuffer);
            if (!s_transparentInitialized || !s_transparentPipeline)
                return;

            using namespace RENDER_NAMESPACE::bindless;
            auto& matBuffer = MaterialBuffer::Instance();

            struct LightingConstants {
                Fvector4 sunDirection;
                Fvector4 sunColor;
                Fvector4 ambientColor;
                Fvector4 cameraPosition;
                Fvector4 fogParams;
                Fvector4 fogColor;
            } lightingData;

            nvrhi::BufferDesc cbDesc;
            cbDesc.byteSize = sizeof(LightingConstants);
            cbDesc.isConstantBuffer = true;
            cbDesc.isVolatile = true;
            cbDesc.maxVersions = 16;
            auto lightingCB = nvDevice->createBuffer(cbDesc);

            if (g_pGamePersistent) {
                auto& env = g_pGamePersistent->Environment().CurrentEnv;
                lightingData.sunDirection.set(env.sun_dir.x, env.sun_dir.y, env.sun_dir.z, 0.0f);
                lightingData.sunColor.set(env.sun_color.x, env.sun_color.y, env.sun_color.z, 1.0f);
                lightingData.fogParams.set(env.fog_near, env.fog_far, env.fog_density, 0.0f);
                lightingData.fogColor.set(env.fog_color.x, env.fog_color.y, env.fog_color.z, 1.0f);
            } else {
                lightingData.sunDirection.set(0.5f, -0.7f, 0.5f, 0.0f);
                lightingData.sunColor.set(1.0f, 1.0f, 1.0f, 1.0f);
                lightingData.fogParams.set(50.0f, 300.0f, 0.001f, 0.0f);
                lightingData.fogColor.set(0.5f, 0.5f, 0.6f, 1.0f);
            }
            lightingData.ambientColor.set(0.1f, 0.1f, 0.15f, 1.0f);
            lightingData.cameraPosition.set(Device.vCameraPosition.x, Device.vCameraPosition.y, Device.vCameraPosition.z, 1.0f);
            cmdList->writeBuffer(lightingCB, &lightingData, sizeof(lightingData));

            cbDesc.byteSize = sizeof(StaticGlobals);
            auto staticGlobalsCB = nvDevice->createBuffer(cbDesc);
            StaticGlobals staticGlobals;
            FillGlobalConstants(staticGlobals);
            SunLightData sunData;
            GetSunLightData(sunData, 2.0f);
            FillSunConstants(staticGlobals, sunData);
            cmdList->writeBuffer(staticGlobalsCB, &staticGlobals, sizeof(staticGlobals));

            const auto& cfg = data.config;

            cmdList->beginTrackingBufferState(cfg.instanceBuffer, nvrhi::ResourceStates::ShaderResource);
            cmdList->beginTrackingBufferState(cfg.compactBatchIndicesBuffer, nvrhi::ResourceStates::ShaderResource);
            cmdList->beginTrackingBufferState(cfg.compactMaterialIDBuffer, nvrhi::ResourceStates::ShaderResource);
            cmdList->beginTrackingBufferState(cfg.compactDrawArgsBuffer, nvrhi::ResourceStates::IndirectArgument);
            cmdList->beginTrackingBufferState(cfg.compactCountBuffer, nvrhi::ResourceStates::IndirectArgument);

            auto& variantTexBuffer = bindless::VariantTextureBuffer::Instance();

            nvrhi::BindingSetDesc bindDesc;
            bindDesc.bindings = {
                nvrhi::BindingSetItem::ConstantBuffer(2, staticGlobalsCB),
                nvrhi::BindingSetItem::ConstantBuffer(4, lightingCB),
                nvrhi::BindingSetItem::StructuredBuffer_SRV(8, matBuffer.GetBuffer()),
                nvrhi::BindingSetItem::StructuredBuffer_SRV(10, variantTexBuffer.GetBuffer()),
                nvrhi::BindingSetItem::Sampler(0, s_transparentSampler),
                nvrhi::BindingSetItem::StructuredBuffer_SRV(14, cfg.instanceBuffer),
                nvrhi::BindingSetItem::StructuredBuffer_SRV(15, cfg.compactBatchIndicesBuffer),
                nvrhi::BindingSetItem::StructuredBuffer_SRV(16, cfg.compactMaterialIDBuffer),
            };

            auto bindingSet = nvDevice->createBindingSet(bindDesc, s_transparentLayout);
            R_ASSERT2(bindingSet, "Transparent binding set creation failed");

            nvrhi::GraphicsState state;
            state.pipeline = s_transparentPipeline;
            state.framebuffer = framebuffer;
            state.bindings = { bindingSet };

            auto* backend = data.device->GetBackend();
            if (backend) {
                auto* bindlessTable = backend->GetBindlessDescriptorTable();
                if (bindlessTable)
                    state.addBindingSet(bindlessTable);
            }

            state.vertexBuffers = {
                {cfg.megaVertexBuffer, 0, 0},
                {s_transparentDrawIndexBuffer, 1, 0}
            };
            state.indexBuffer = { cfg.megaIndexBuffer, nvrhi::Format::R32_UINT, 0 };
            state.indirectParams = cfg.compactDrawArgsBuffer;
            state.indirectCountBuffer = cfg.compactCountBuffer;

            const auto& rtDesc = colorRT->getDesc();
            nvrhi::Viewport viewport(0.0f, static_cast<float>(rtDesc.width), 0.0f, static_cast<float>(rtDesc.height), 0.0f, 1.0f);
            state.viewport.addViewport(viewport);
            state.viewport.addScissorRect(nvrhi::Rect(rtDesc.width, rtDesc.height));

            if (cfg.variantPartition.Enabled()) {
                auto* backendDev = data.device->GetBackend();

                VariantPartitionDrawConfig vpCfg;
                vpCfg.defaultPipeline = s_transparentPipeline.Get();
                vpCfg.inputLayout = s_transparentInputLayout;
                vpCfg.passLayout = s_transparentLayout;
                vpCfg.bindlessLayout = backendDev ? backendDev->GetBindlessLayout() : nullptr;
                vpCfg.bindlessTable = backend ? backend->GetBindlessDescriptorTable() : nullptr;
                vpCfg.sampler = s_transparentSampler;
                vpCfg.staticGlobalsCB = staticGlobalsCB;
                vpCfg.lightingCB = lightingCB;
                vpCfg.materialBuffer = matBuffer.GetBuffer();
                vpCfg.variantTexBuffer = variantTexBuffer.GetBuffer();
                vpCfg.instanceBuffer = cfg.instanceBuffer;
                vpCfg.megaVertexBuffer = cfg.megaVertexBuffer;
                vpCfg.partition = cfg.variantPartition;
                vpCfg.selectTransparent = true;

                DrawVariantPartition(cmdList, nvDevice, framebuffer, state, vpCfg);
            } else {
                cmdList->setGraphicsState(state);
                cmdList->drawIndexedIndirectCount(0, 0, cfg.objectCount);
            }
        }
    );

    DefaultOutputLayout outputs;
    outputs.albedo = passData.color;
    outputs.depth = passData.depth;
    return outputs;
}

} // namespace xray::render::RENDER_NAMESPACE::passes
