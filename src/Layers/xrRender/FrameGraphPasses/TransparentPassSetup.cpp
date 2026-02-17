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
#include "PassCommon.h"

namespace xray::render::RENDER_NAMESPACE::passes {

static void InitializeTransparentResources(ng::RenderDevice* device, nvrhi::IFramebuffer* framebuffer, TransparentPassState& state)
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

    nvrhi::SamplerDesc samplerDesc;
    samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Repeat);
    samplerDesc.setAllFilters(true);
    samplerDesc.setMaxAnisotropy(16.0f);
    state.sampler = nvDevice->createSampler(samplerDesc);

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
    state.layout = nvDevice->createBindingLayout(layoutDesc);
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

    state.pipeline = nvDevice->createGraphicsPipeline(pipeDesc, framebuffer);
    if (!state.pipeline)
        return;

    QueryBindingLayoutFromPipeline(state.pipeline, state.layout);

    state.initialized = true;
    Msg("* [TransparentPass] Pipeline initialized");
}

void ShutdownTransparentPipelines(TransparentPassState& state)
{
    state.pipeline = nullptr;
    state.layout = nullptr;
    state.inputLayout = nullptr;
    state.sampler = nullptr;
    state.vs = nullptr;
    state.ps = nullptr;
    state.initialized = false;
}

framegraph::DefaultOutputLayout setupTransparentPass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    const framegraph::DefaultOutputLayout& inputs,
    const TransparentPassConfig& config,
    u32 width, u32 height,
    TransparentPassState& state)
{
    using namespace framegraph;

    if (!config.IsValid()) {
        return inputs;
    }

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
        },

        [](const TransparentPassData& data,
            const FrameGraph& fg,
            ng::RenderContext* ctx) {

            auto* colorRT = fg.GetPhysicalTexture(data.color);
            auto* normalRT = fg.GetPhysicalTexture(data.normal);
            auto* depthRT = fg.GetPhysicalTexture(data.depth);
            if (!colorRT || !depthRT)
                return;

            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            if (!nvDevice || !cmdList)
                return;

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(colorRT);
            if (normalRT)
                fbDesc.addColorAttachment(normalRT);
            fbDesc.setDepthAttachment(depthRT);
            auto& cache = framegraph::GetPassResourceCache();
            auto framebuffer = cache.GetOrCreateFramebuffer("TransparentPass", fbDesc, nvDevice);
            if (!framebuffer)
                return;

            InitializeTransparentResources(data.device, framebuffer, *data.passState);
            if (!data.passState->initialized || !data.passState->pipeline)
                return;

            using namespace RENDER_NAMESPACE::bindless;
            auto& matBuffer = MaterialBuffer::Instance();

            auto lightingCB = cache.GetOrCreateVolatileCB("TransparentPass", "LightingCB", sizeof(LightingConstants), 16, nvDevice);
            auto staticGlobalsCB = cache.GetOrCreateVolatileCB("TransparentPass", "StaticGlobalsCB", sizeof(StaticGlobals), 16, nvDevice);
            auto drawIndexBuffer = GetOrCreateDrawIndexBuffer("TransparentPass", nvDevice);

            auto lightingData = FillLightingConstants();
            cmdList->writeBuffer(lightingCB, &lightingData, sizeof(lightingData));

            auto staticGlobals = BuildStaticGlobals();
            cmdList->writeBuffer(staticGlobalsCB, &staticGlobals, sizeof(staticGlobals));

            const auto& cfg = data.config;

            auto& variantTexBuffer = bindless::VariantTextureBuffer::Instance();

            nvrhi::BindingSetDesc bindDesc;
            bindDesc.bindings = {
                nvrhi::BindingSetItem::ConstantBuffer(2, staticGlobalsCB),
                nvrhi::BindingSetItem::ConstantBuffer(4, lightingCB),
                nvrhi::BindingSetItem::StructuredBuffer_SRV(8, matBuffer.GetBuffer()),
                nvrhi::BindingSetItem::StructuredBuffer_SRV(10, variantTexBuffer.GetBuffer()),
                nvrhi::BindingSetItem::Sampler(0, data.passState->sampler),
                nvrhi::BindingSetItem::StructuredBuffer_SRV(14, cfg.instanceBuffer),
                nvrhi::BindingSetItem::StructuredBuffer_SRV(15, cfg.compactBatchIndicesBuffer),
                nvrhi::BindingSetItem::StructuredBuffer_SRV(16, cfg.compactMaterialIDBuffer),
            };

            auto bindingSet = framegraph::GetPassResourceCache().GetOrCreateBindingSet(bindDesc, data.passState->layout, nvDevice);
            R_ASSERT2(bindingSet, "Transparent binding set creation failed");

            nvrhi::GraphicsState state;
            state.pipeline = data.passState->pipeline;
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
                {drawIndexBuffer, 1, 0}
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
                vpCfg.defaultPipeline = data.passState->pipeline.Get();
                vpCfg.inputLayout = data.passState->inputLayout;
                vpCfg.passLayout = data.passState->layout;
                vpCfg.bindlessLayout = backendDev ? backendDev->GetBindlessLayout() : nullptr;
                vpCfg.bindlessTable = backend ? backend->GetBindlessDescriptorTable() : nullptr;
                vpCfg.sampler = data.passState->sampler;
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
    outputs.normal = passData.normal;
    outputs.depth = passData.depth;
    return outputs;
}

} // namespace xray::render::RENDER_NAMESPACE::passes
