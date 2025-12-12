#include "stdafx.h"
#include "ForwardColorPassSetup.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/Geometry/GeometryBatch.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/PipelineState.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/FTreeVisual.h"
#include "Layers/xrRender/ConstantSystem/FGConstantSystem.h"
#include "Layers/xrRender/GPUCullingManager.h"
#include "Layers/xrRender/Backend/D3D12Backend.h"
#include "Layers/xrRender/Bindless/MaterialBuffer.h"
#include "xrCore/FMesh.hpp"

namespace xray::render::RENDER_NAMESPACE
{
    extern float r__dtex_range;
}

namespace xray::render::RENDER_NAMESPACE::passes {

enum class RenderPhase { None, Opaque, AlphaTested, Transparent };

static nvrhi::GraphicsPipelineHandle s_bindlessPipeline;
static nvrhi::BindingLayoutHandle s_bindlessLayout;
static nvrhi::InputLayoutHandle s_bindlessInputLayout;
static nvrhi::SamplerHandle s_linearSampler;
static nvrhi::ShaderHandle s_bindlessVS;
static nvrhi::ShaderHandle s_bindlessPS;
static nvrhi::BufferHandle s_drawIndexBuffer;
static bool s_bindlessInitialized = false;

static void InitializeBindlessPipeline(ng::RenderDevice* device, nvrhi::IFramebuffer* framebuffer);

void InitializeForwardPipelines(ng::RenderDevice* device)
{
    if (!device)
        return;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    if (!nvDevice)
        return;

    Msg("* [ForwardPass] Initializing forward pipelines...");

    nvrhi::TextureDesc colorDesc;
    colorDesc.width = 64;
    colorDesc.height = 64;
    colorDesc.format = nvrhi::Format::RGBA16_FLOAT;
    colorDesc.isRenderTarget = true;
    colorDesc.initialState = nvrhi::ResourceStates::RenderTarget;
    colorDesc.keepInitialState = true;
    colorDesc.debugName = "ForwardInit_DummyColor";
    auto dummyColorRT = nvDevice->createTexture(colorDesc);

    nvrhi::TextureDesc depthDesc;
    depthDesc.width = 64;
    depthDesc.height = 64;
    depthDesc.format = nvrhi::Format::D32;
    depthDesc.isRenderTarget = true;
    depthDesc.initialState = nvrhi::ResourceStates::DepthWrite;
    depthDesc.keepInitialState = true;
    depthDesc.debugName = "ForwardInit_DummyDepth";
    auto dummyDepthRT = nvDevice->createTexture(depthDesc);

    nvrhi::FramebufferDesc fbDesc;
    fbDesc.addColorAttachment(dummyColorRT);
    fbDesc.setDepthAttachment(dummyDepthRT);
    auto dummyFramebuffer = nvDevice->createFramebuffer(fbDesc);

    if (!dummyFramebuffer) {
        Msg("! [ForwardPass] Failed to create dummy framebuffer for pipeline init");
        return;
    }

    InitializeBindlessPipeline(device, dummyFramebuffer);

    Msg("* [ForwardPass] Pipeline initialization complete");
}

void ShutdownForwardPipelines()
{
    s_bindlessPipeline = nullptr;
    s_bindlessLayout = nullptr;
    s_bindlessInputLayout = nullptr;
    s_linearSampler = nullptr;
    s_bindlessVS = nullptr;
    s_bindlessPS = nullptr;
    s_drawIndexBuffer = nullptr;
    s_bindlessInitialized = false;

    Msg("* [ForwardPass] Pipeline resources released");
}

static void InitializeBindlessPipeline(ng::RenderDevice* device, nvrhi::IFramebuffer* framebuffer)
{
    if (s_bindlessInitialized)
        return;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    if (!nvDevice)
        return;

    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    if (!shaderLoader)
        return;

    auto vsResult = shaderLoader->LoadVertexShader("bindless_forward", "main");
    auto psResult = shaderLoader->LoadPixelShader("bindless_forward", "main");

    if (!vsResult.handle || !psResult.handle) {
        Msg("! [BindlessForward] Failed to load shaders");
        return;
    }

    s_bindlessVS = vsResult.handle;
    s_bindlessPS = psResult.handle;

    nvrhi::SamplerDesc samplerDesc;
    samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Repeat);
    samplerDesc.setAllFilters(true);
    samplerDesc.setMaxAnisotropy(16.0f);
    s_linearSampler = nvDevice->createSampler(samplerDesc);

    nvrhi::BindingLayoutDesc layoutDesc;
    layoutDesc.visibility = nvrhi::ShaderType::All;
    layoutDesc.bindings = {
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(2),
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(4),
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(8),
        nvrhi::BindingLayoutItem::Sampler(0),
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(14),
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(15),
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(16),
    };
    s_bindlessLayout = nvDevice->createBindingLayout(layoutDesc);

    constexpr u32 vertexStride = 48;
    nvrhi::VertexAttributeDesc vertexAttribs[] = {
        nvrhi::VertexAttributeDesc()
            .setName("POSITION")
            .setFormat(nvrhi::Format::RGB32_FLOAT)
            .setBufferIndex(0)
            .setOffset(0)
            .setElementStride(vertexStride),
        nvrhi::VertexAttributeDesc()
            .setName("NORMAL")
            .setFormat(nvrhi::Format::BGRA8_UNORM)
            .setBufferIndex(0)
            .setOffset(12)
            .setElementStride(vertexStride),
        nvrhi::VertexAttributeDesc()
            .setName("TANGENT")
            .setFormat(nvrhi::Format::BGRA8_UNORM)
            .setBufferIndex(0)
            .setOffset(16)
            .setElementStride(vertexStride),
        nvrhi::VertexAttributeDesc()
            .setName("BINORMAL")
            .setFormat(nvrhi::Format::BGRA8_UNORM)
            .setBufferIndex(0)
            .setOffset(20)
            .setElementStride(vertexStride),
        nvrhi::VertexAttributeDesc()
            .setName("TEXCOORD")
            .setFormat(nvrhi::Format::RG32_FLOAT)
            .setArraySize(2)
            .setBufferIndex(0)
            .setOffset(24)
            .setElementStride(vertexStride),
        nvrhi::VertexAttributeDesc()
            .setName("COLOR")
            .setFormat(nvrhi::Format::BGRA8_UNORM)
            .setBufferIndex(0)
            .setOffset(40)
            .setElementStride(vertexStride),
        nvrhi::VertexAttributeDesc()
            .setName("DRAWINDEX")
            .setFormat(nvrhi::Format::R32_UINT)
            .setBufferIndex(1)
            .setOffset(0)
            .setElementStride(4)
            .setIsInstanced(true),
    };
    s_bindlessInputLayout = nvDevice->createInputLayout(vertexAttribs, 7, s_bindlessVS);

    constexpr u32 MAX_DRAWS = 65536;
    xr_vector<u32> drawIndices(MAX_DRAWS);
    for (u32 i = 0; i < MAX_DRAWS; i++) {
        drawIndices[i] = i;
    }

    nvrhi::BufferDesc drawIndexDesc;
    drawIndexDesc.byteSize = MAX_DRAWS * sizeof(u32);
    drawIndexDesc.structStride = sizeof(u32);
    drawIndexDesc.isVertexBuffer = true;
    drawIndexDesc.debugName = "DrawIndexBuffer";
    drawIndexDesc.initialState = nvrhi::ResourceStates::VertexBuffer;
    drawIndexDesc.keepInitialState = true;
    s_drawIndexBuffer = nvDevice->createBuffer(drawIndexDesc);

    if (!s_drawIndexBuffer) {
        Msg("! [BindlessForward] Failed to create draw index buffer");
        return;
    }

    if (GEnv.Backend) {
        GEnv.Backend->UploadBufferData(s_drawIndexBuffer, drawIndices.data(), MAX_DRAWS * sizeof(u32));
    }

    Msg("* [BindlessForward] Created draw index buffer (65536 entries)");

    nvrhi::GraphicsPipelineDesc pipeDesc;
    pipeDesc.VS = s_bindlessVS;
    pipeDesc.PS = s_bindlessPS;
    pipeDesc.inputLayout = s_bindlessInputLayout;

    auto* backend = device->GetBackend();
    nvrhi::IBindingLayout* bindlessLayout = backend ? backend->GetBindlessLayout() : nullptr;

    if (bindlessLayout) {
        pipeDesc.bindingLayouts = { s_bindlessLayout, bindlessLayout };
    } else {
        pipeDesc.bindingLayouts = { s_bindlessLayout };
    }

    pipeDesc.primType = nvrhi::PrimitiveType::TriangleList;
    pipeDesc.renderState.depthStencilState.depthTestEnable = true;
    pipeDesc.renderState.depthStencilState.depthWriteEnable = true;
    pipeDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::LessOrEqual;
    pipeDesc.renderState.rasterState.frontCounterClockwise = false;
    pipeDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::Back;

    s_bindlessPipeline = nvDevice->createGraphicsPipeline(pipeDesc, framebuffer);
    if (!s_bindlessPipeline) {
        Msg("! [BindlessForward] Failed to create pipeline");
        return;
    }

    const nvrhi::GraphicsPipelineDesc& actualDesc = s_bindlessPipeline->getDesc();
    if (!actualDesc.bindingLayouts.empty()) {
        s_bindlessLayout = actualDesc.bindingLayouts[0];
    }

    s_bindlessInitialized = true;
    Msg("* [BindlessForward] Pipeline initialized");
}

void renderBindlessForward(
    ng::RenderContext* ctx,
    ng::RenderDevice* device,
    const GeometryCollector* geometry,
    nvrhi::ITexture* colorRT,
    nvrhi::ITexture* depthRT,
    const BindlessForwardConfig& config,
    MaterialCache* materialCache)
{
    using namespace RENDER_NAMESPACE::bindless;

    if (!config.IsValid() || !geometry || geometry->GetBatches().empty())
        return;

    if (materialCache) {
        materialCache->FinalizePendingMaterials(ctx);
    }

    auto& matBuffer = MaterialBuffer::Instance();
    matBuffer.Upload(ctx);

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    nvrhi::ICommandList* cmdList = ctx->GetCommandList();
    if (!cmdList)
        return;

    if (!s_bindlessInitialized) {
        auto* shaderLoader = GEnv.Render->GetShaderLoader();
        if (!shaderLoader)
            return;

        auto vsResult = shaderLoader->LoadVertexShader("bindless_forward", "main");
        auto psResult = shaderLoader->LoadPixelShader("bindless_forward", "main");

        if (!vsResult.handle || !psResult.handle)
            return;

        s_bindlessVS = vsResult.handle;
        s_bindlessPS = psResult.handle;

        nvrhi::SamplerDesc samplerDesc;
        samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Repeat);
        samplerDesc.setAllFilters(true);
        samplerDesc.setMaxAnisotropy(16.0f);
        s_linearSampler = nvDevice->createSampler(samplerDesc);

        nvrhi::BindingLayoutDesc layoutDesc;
        layoutDesc.visibility = nvrhi::ShaderType::All;
        layoutDesc.bindings = {
            nvrhi::BindingLayoutItem::VolatileConstantBuffer(2),
            nvrhi::BindingLayoutItem::VolatileConstantBuffer(4),
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(8),
            nvrhi::BindingLayoutItem::Sampler(0),
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(14),
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(15),
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(16),
        };
        s_bindlessLayout = nvDevice->createBindingLayout(layoutDesc);

        if (!s_bindlessLayout)
            return;

        constexpr u32 vertexStride = 48;
        nvrhi::VertexAttributeDesc vertexAttribs[] = {
            nvrhi::VertexAttributeDesc()
                .setName("POSITION")
                .setFormat(nvrhi::Format::RGB32_FLOAT)
                .setBufferIndex(0)
                .setOffset(0)
                .setElementStride(vertexStride),
            nvrhi::VertexAttributeDesc()
                .setName("NORMAL")
                .setFormat(nvrhi::Format::BGRA8_UNORM)
                .setBufferIndex(0)
                .setOffset(12)
                .setElementStride(vertexStride),
            nvrhi::VertexAttributeDesc()
                .setName("TANGENT")
                .setFormat(nvrhi::Format::BGRA8_UNORM)
                .setBufferIndex(0)
                .setOffset(16)
                .setElementStride(vertexStride),
            nvrhi::VertexAttributeDesc()
                .setName("BINORMAL")
                .setFormat(nvrhi::Format::BGRA8_UNORM)
                .setBufferIndex(0)
                .setOffset(20)
                .setElementStride(vertexStride),
            nvrhi::VertexAttributeDesc()
                .setName("TEXCOORD")
                .setFormat(nvrhi::Format::RG32_FLOAT)
                .setArraySize(2)
                .setBufferIndex(0)
                .setOffset(24)
                .setElementStride(vertexStride),
            nvrhi::VertexAttributeDesc()
                .setName("COLOR")
                .setFormat(nvrhi::Format::BGRA8_UNORM)
                .setBufferIndex(0)
                .setOffset(40)
                .setElementStride(vertexStride),
            nvrhi::VertexAttributeDesc()
                .setName("DRAWINDEX")
                .setFormat(nvrhi::Format::R32_UINT)
                .setBufferIndex(1)
                .setOffset(0)
                .setElementStride(4)
                .setIsInstanced(true),
        };
        s_bindlessInputLayout = nvDevice->createInputLayout(vertexAttribs, 7, s_bindlessVS);

        constexpr u32 MAX_DRAWS = 65536;
        xr_vector<u32> drawIndices(MAX_DRAWS);
        for (u32 i = 0; i < MAX_DRAWS; i++) {
            drawIndices[i] = i;
        }

        nvrhi::BufferDesc drawIndexDesc;
        drawIndexDesc.byteSize = MAX_DRAWS * sizeof(u32);
        drawIndexDesc.structStride = sizeof(u32);
        drawIndexDesc.isVertexBuffer = true;
        drawIndexDesc.debugName = "DrawIndexBuffer";
        drawIndexDesc.initialState = nvrhi::ResourceStates::VertexBuffer;
        drawIndexDesc.keepInitialState = true;
        s_drawIndexBuffer = nvDevice->createBuffer(drawIndexDesc);

        if (!s_drawIndexBuffer) {
            Msg("! [BindlessForward] Failed to create draw index buffer");
            return;
        }

        if (GEnv.Backend) {
            GEnv.Backend->UploadBufferData(s_drawIndexBuffer, drawIndices.data(), MAX_DRAWS * sizeof(u32));
        }

        Msg("* [BindlessForward] Created draw index buffer (65536 entries)");

        nvrhi::GraphicsPipelineDesc pipeDesc;
        pipeDesc.VS = s_bindlessVS;
        pipeDesc.PS = s_bindlessPS;
        pipeDesc.inputLayout = s_bindlessInputLayout;

        auto* backend = device->GetBackend();
        nvrhi::IBindingLayout* bindlessLayout = backend ? backend->GetBindlessLayout() : nullptr;

        if (bindlessLayout) {
            pipeDesc.bindingLayouts = { s_bindlessLayout, bindlessLayout };
        } else {
            pipeDesc.bindingLayouts = { s_bindlessLayout };
        }

        pipeDesc.primType = nvrhi::PrimitiveType::TriangleList;
        pipeDesc.renderState.depthStencilState.depthTestEnable = true;
        pipeDesc.renderState.depthStencilState.depthWriteEnable = true;
        pipeDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::LessOrEqual;
        pipeDesc.renderState.rasterState.frontCounterClockwise = false;
        pipeDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::Back;

        nvrhi::FramebufferDesc fbDesc;
        fbDesc.addColorAttachment(colorRT);
        fbDesc.setDepthAttachment(depthRT);
        auto framebuffer = nvDevice->createFramebuffer(fbDesc);

        Msg("* [FGDetailManager] Creating bindless forward graphics pipeline...");
        Msg("  - VS handle: %p", s_bindlessVS.Get());
        Msg("  - PS handle: %p", s_bindlessPS.Get());
        Msg("  - Binding layout: %p", s_bindlessLayout.Get());
        Msg("* [FGDetailManager] Calling device->createGraphicsPipeline...");

        s_bindlessPipeline = nvDevice->createGraphicsPipeline(pipeDesc, framebuffer);
        if (!s_bindlessPipeline) {
            Msg("! [FGDetailManager] Failed to create bindless forward graphics pipeline");
            return;
        }

        Msg("* [FGDetailManager] Bindless forward graphics pipeline created successfully");

        const nvrhi::GraphicsPipelineDesc& actualDesc = s_bindlessPipeline->getDesc();
        if (!actualDesc.bindingLayouts.empty()) {
            s_bindlessLayout = actualDesc.bindingLayouts[0];
            Msg("* [FGDetailManager] Updated bindless forward bindingLayout from pipeline (pointer: %p)", s_bindlessLayout.Get());
        }

        s_bindlessInitialized = true;
    }

    nvrhi::FramebufferDesc fbDesc;
    fbDesc.addColorAttachment(colorRT);
    fbDesc.setDepthAttachment(depthRT);
    auto framebuffer = nvDevice->createFramebuffer(fbDesc);

    struct LightingConstants {
        Fvector4 sunDirection;
        Fvector4 sunColor;
        Fvector4 ambientColor;
        Fvector4 cameraPosition;
        Fvector4 fogParams;
        Fvector4 fogColor;
    } lightingData;
    static_assert(sizeof(LightingConstants) == 96, "LightingConstants must be 96 bytes");

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

    nvrhi::BindingSetDesc bindDesc;
    bindDesc.bindings = {
        nvrhi::BindingSetItem::ConstantBuffer(2, staticGlobalsCB),
        nvrhi::BindingSetItem::ConstantBuffer(4, lightingCB),
        nvrhi::BindingSetItem::StructuredBuffer_SRV(8, matBuffer.GetBuffer()),
        nvrhi::BindingSetItem::Sampler(0, s_linearSampler),
        nvrhi::BindingSetItem::StructuredBuffer_SRV(14, config.instanceBuffer),
        nvrhi::BindingSetItem::StructuredBuffer_SRV(15, config.compactBatchIndicesBuffer),
        nvrhi::BindingSetItem::StructuredBuffer_SRV(16, config.compactMaterialIDBuffer),
    };

    auto bindingSet = nvDevice->createBindingSet(bindDesc, s_bindlessLayout);
    if (!bindingSet)
        return;

    const auto& rtDesc = colorRT->getDesc();
    nvrhi::Viewport viewport(0.0f, static_cast<float>(rtDesc.width), 0.0f, static_cast<float>(rtDesc.height), 0.0f, 1.0f);

    static u32 s_gpuDrivenDraws = 0;

    if (!config.UseGPUCulling() || !config.UseMegaBuffers()) {
        return;
    }

    const u32 visibleCount = config.totalObjectCount;

    if (!s_bindlessPipeline || !framebuffer || !bindingSet) {
        return;
    }

    constexpr u32 INDIRECT_ARGS_SIZE = 20;

    nvrhi::GraphicsState state;
    state.pipeline = s_bindlessPipeline;
    state.framebuffer = framebuffer;
    state.bindings = { bindingSet };

    auto* backend = device->GetBackend();
    if (backend) {
        auto* bindlessTable = backend->GetBindlessDescriptorTable();
        if (bindlessTable) {
            state.addBindingSet(bindlessTable);
        }
    }

    if (!s_drawIndexBuffer) {
        Msg("! [BindlessForward] Draw index buffer not initialized!");
        return;
    }

    state.vertexBuffers = {
        {config.megaVertexBuffer, 0, 0},
        {s_drawIndexBuffer, 1, 0}
    };
    state.indexBuffer = { config.megaIndexBuffer, nvrhi::Format::R32_UINT, 0 };
    state.indirectParams = config.compactDrawArgsBuffer;
    state.indirectCountBuffer = config.compactCountBuffer;
    state.indirectCountOffset = 0;
    state.viewport.addViewport(viewport);
    state.viewport.addScissorRect(nvrhi::Rect(rtDesc.width, rtDesc.height));

    cmdList->setGraphicsState(state);
    cmdList->drawIndexedIndirectCount(0, config.totalObjectCount);

    s_gpuDrivenDraws += config.totalObjectCount;

}

framegraph::DefaultOutputLayout setupForwardColorPass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    framegraph::VirtualResourceHandle depthInput,
    framegraph::VirtualResourceHandle colorInput,
    const GeometryCollector* geometry,
    MaterialCache* materialCache,
    u32 width,
    u32 height,
    framegraph::VirtualResourceHandle drawArgsInput,
    const BindlessForwardConfig& bindlessConfig)
{
    using namespace framegraph;

    struct ForwardColorPassData {
        VirtualResourceHandle depth;
        VirtualResourceHandle color;
        VirtualResourceHandle drawArgsBuffer;
        ng::RenderDevice* device;
        const GeometryCollector* geometry;
        MaterialCache* materialCache;
        DefaultOutputLayout outputs;
        u32 width;
        u32 height;
        BindlessForwardConfig bindlessConfig;
    };

    auto& passData = fg.addCallbackPass<ForwardColorPassData>(
        "Forward+ Color Pass",

        [&, width, height, colorInput, drawArgsInput, bindlessConfig](FrameGraph& builder, PassHandle passHandle, ForwardColorPassData& data) {
            data.width = width;
            data.height = height;
            data.device = device;
            data.geometry = geometry;
            data.materialCache = materialCache;
            data.bindlessConfig = bindlessConfig;

            RenderPassBuilder passBuilder(builder, passHandle);

            data.depth = passBuilder.readWrite(depthInput, ResourceState::DepthStencilWrite);

            data.color = passBuilder.readWrite(colorInput, ResourceState::RenderTarget);

            if (drawArgsInput.is_valid()) {
                data.drawArgsBuffer = passBuilder.read(drawArgsInput, ResourceState::IndirectArgument);
            }

            data.outputs.albedo = data.color;
            data.outputs.depth = data.depth;
        },

        [](const ForwardColorPassData& data,
            const FrameGraph& fg,
            ng::RenderContext* ctx) {

            auto* depthRT = fg.GetPhysicalTexture(data.depth);
            auto* colorRT = fg.GetPhysicalTexture(data.color);

            if (!depthRT || !colorRT)
                return;

            if (!data.geometry)
                return;

            nvrhi::IBuffer* drawArgsBuffer = nullptr;
            if (data.drawArgsBuffer.is_valid()) {
                drawArgsBuffer = fg.GetPhysicalBuffer(data.drawArgsBuffer);
            }

            renderBindlessForward(
                ctx,
                data.device,
                data.geometry,
                colorRT,
                depthRT,
                data.bindlessConfig,
                data.materialCache
            );
        }
    );

    DefaultOutputLayout outputs;
    outputs.albedo = passData.color;
    outputs.normal = passData.color;
    outputs.material = passData.color;
    outputs.depth = passData.depth;
    return outputs;
}
}
