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
#include "Layers/xrRender/SkeletonCustom.h"
#include "Layers/xrRender/FSkinned.h"
#include "Layers/xrRender/FTreeVisual.h"
#include "Layers/xrRender/SkeletonX.h"
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

static nvrhi::GraphicsPipelineHandle s_skinnedPipeline;
static nvrhi::BindingLayoutHandle s_skinnedLayout;
static nvrhi::InputLayoutHandle s_skinnedInputLayout;
static nvrhi::ShaderHandle s_skinnedVS;
static nvrhi::ShaderHandle s_skinnedPS;
static bool s_skinnedInitialized = false;

static nvrhi::GraphicsPipelineHandle s_skinnedHQ1WPipeline;
static nvrhi::InputLayoutHandle s_skinnedHQ1WInputLayout;
static nvrhi::ShaderHandle s_skinnedHQ1WVS;
static bool s_skinnedHQ1WInitialized = false;

static nvrhi::GraphicsPipelineHandle s_skinnedHQ4WPipeline;
static nvrhi::InputLayoutHandle s_skinnedHQ4WInputLayout;
static nvrhi::ShaderHandle s_skinnedHQ4WVS;
static bool s_skinnedHQ4WInitialized = false;

static nvrhi::GraphicsPipelineHandle s_skinnedHQ2WPipeline;
static nvrhi::InputLayoutHandle s_skinnedHQ2WInputLayout;
static nvrhi::ShaderHandle s_skinnedHQ2WVS;
static bool s_skinnedHQ2WInitialized = false;

static void InitializeBindlessPipeline(ng::RenderDevice* device, nvrhi::IFramebuffer* framebuffer);
static void InitializeSkinnedPipelines(ng::RenderDevice* device, nvrhi::IFramebuffer* framebuffer);

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
    depthDesc.format = nvrhi::Format::D24S8;
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
    InitializeSkinnedPipelines(device, dummyFramebuffer);

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

    s_skinnedPipeline = nullptr;
    s_skinnedLayout = nullptr;
    s_skinnedInputLayout = nullptr;
    s_skinnedVS = nullptr;
    s_skinnedPS = nullptr;
    s_skinnedInitialized = false;

    s_skinnedHQ1WPipeline = nullptr;
    s_skinnedHQ1WInputLayout = nullptr;
    s_skinnedHQ1WVS = nullptr;
    s_skinnedHQ1WInitialized = false;

    s_skinnedHQ4WPipeline = nullptr;
    s_skinnedHQ4WInputLayout = nullptr;
    s_skinnedHQ4WVS = nullptr;
    s_skinnedHQ4WInitialized = false;

    s_skinnedHQ2WPipeline = nullptr;
    s_skinnedHQ2WInputLayout = nullptr;
    s_skinnedHQ2WVS = nullptr;
    s_skinnedHQ2WInitialized = false;

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

    s_bindlessInitialized = true;
    Msg("* [BindlessForward] Pipeline initialized");
}

static void InitializeSkinnedPipelines(ng::RenderDevice* device, nvrhi::IFramebuffer* framebuffer)
{
    if (s_skinnedInitialized && s_skinnedHQ1WInitialized && s_skinnedHQ4WInitialized && s_skinnedHQ2WInitialized)
        return;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    if (!nvDevice)
        return;

    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    if (!shaderLoader)
        return;

    auto* backend = device->GetBackend();
    nvrhi::IBindingLayout* bindlessLayout = backend ? backend->GetBindlessLayout() : nullptr;

    if (!s_skinnedLayout) {
        Msg("* [SkinnedPipeline] Creating binding layout...");
        nvrhi::BindingLayoutDesc skinnedLayoutDesc;
        skinnedLayoutDesc.visibility = nvrhi::ShaderType::All;
        skinnedLayoutDesc.bindings = {
            nvrhi::BindingLayoutItem::VolatileConstantBuffer(0),
            nvrhi::BindingLayoutItem::VolatileConstantBuffer(2),
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(3),
            nvrhi::BindingLayoutItem::VolatileConstantBuffer(4),
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(8),
            nvrhi::BindingLayoutItem::Sampler(0),
        };
        s_skinnedLayout = nvDevice->createBindingLayout(skinnedLayoutDesc);
        Msg("* [SkinnedPipeline] Binding layout: %s", s_skinnedLayout ? "OK" : "FAILED");
    }

    if (!s_skinnedPS) {
        auto skinnedPsResult = shaderLoader->LoadPixelShader("bindless_skinned", "main");
        if (skinnedPsResult.handle) {
            s_skinnedPS = skinnedPsResult.handle;
        } else {
            Msg("! [SkinnedPipeline] Failed to load pixel shader");
            return;
        }
    }

    if (!s_linearSampler) {
        nvrhi::SamplerDesc samplerDesc;
        samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Repeat);
        samplerDesc.setAllFilters(true);
        samplerDesc.setMaxAnisotropy(16.0f);
        s_linearSampler = nvDevice->createSampler(samplerDesc);
    }

    if (!s_skinnedInitialized) {
        auto skinnedVsResult = shaderLoader->LoadVertexShader("bindless_skinned", "main");
        if (skinnedVsResult.handle) {
            s_skinnedVS = skinnedVsResult.handle;

            constexpr u32 stride = 24;
            nvrhi::VertexAttributeDesc attribs[] = {
                nvrhi::VertexAttributeDesc().setName("POSITION").setFormat(nvrhi::Format::RGBA16_SNORM).setOffset(0).setElementStride(stride),
                nvrhi::VertexAttributeDesc().setName("NORMAL").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(8).setElementStride(stride),
                nvrhi::VertexAttributeDesc().setName("TANGENT").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(12).setElementStride(stride),
                nvrhi::VertexAttributeDesc().setName("BINORMAL").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(16).setElementStride(stride),
                nvrhi::VertexAttributeDesc().setName("TEXCOORD").setFormat(nvrhi::Format::RG16_SNORM).setOffset(20).setElementStride(stride),
            };
            s_skinnedInputLayout = nvDevice->createInputLayout(attribs, 5, s_skinnedVS);

            nvrhi::GraphicsPipelineDesc pipeDesc;
            pipeDesc.VS = s_skinnedVS;
            pipeDesc.PS = s_skinnedPS;
            pipeDesc.inputLayout = s_skinnedInputLayout;
            if (bindlessLayout) {
                pipeDesc.bindingLayouts = { s_skinnedLayout, bindlessLayout };
            } else {
                pipeDesc.bindingLayouts = { s_skinnedLayout };
            }
            pipeDesc.primType = nvrhi::PrimitiveType::TriangleList;
            pipeDesc.renderState.depthStencilState.depthTestEnable = true;
            pipeDesc.renderState.depthStencilState.depthWriteEnable = true;
            pipeDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::LessOrEqual;
            pipeDesc.renderState.rasterState.frontCounterClockwise = false;
            pipeDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::Back;

            Msg("* [SkinnedPipeline] About to call createGraphicsPipeline (24-byte)...");
            s_skinnedPipeline = nvDevice->createGraphicsPipeline(pipeDesc, framebuffer);
            Msg("* [SkinnedPipeline] Result: %s", s_skinnedPipeline ? "OK" : "FAILED");
        }
        s_skinnedInitialized = true;
    }

    if (!s_skinnedHQ1WInitialized) {
        auto vsResult = shaderLoader->LoadVertexShader("bindless_skinned_hq", "main");
        if (vsResult.handle) {
            s_skinnedHQ1WVS = vsResult.handle;

            constexpr u32 stride = 36;
            nvrhi::VertexAttributeDesc attribs[] = {
                nvrhi::VertexAttributeDesc().setName("POSITION").setFormat(nvrhi::Format::RGBA32_FLOAT).setOffset(0).setElementStride(stride),
                nvrhi::VertexAttributeDesc().setName("NORMAL").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(16).setElementStride(stride),
                nvrhi::VertexAttributeDesc().setName("TANGENT").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(20).setElementStride(stride),
                nvrhi::VertexAttributeDesc().setName("BINORMAL").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(24).setElementStride(stride),
                nvrhi::VertexAttributeDesc().setName("TEXCOORD").setFormat(nvrhi::Format::RG32_FLOAT).setOffset(28).setElementStride(stride),
            };
            s_skinnedHQ1WInputLayout = nvDevice->createInputLayout(attribs, 5, s_skinnedHQ1WVS);

            nvrhi::GraphicsPipelineDesc pipeDesc;
            pipeDesc.VS = s_skinnedHQ1WVS;
            pipeDesc.PS = s_skinnedPS;
            pipeDesc.inputLayout = s_skinnedHQ1WInputLayout;
            if (bindlessLayout) {
                pipeDesc.bindingLayouts = { s_skinnedLayout, bindlessLayout };
            } else {
                pipeDesc.bindingLayouts = { s_skinnedLayout };
            }
            pipeDesc.primType = nvrhi::PrimitiveType::TriangleList;
            pipeDesc.renderState.depthStencilState.depthTestEnable = true;
            pipeDesc.renderState.depthStencilState.depthWriteEnable = true;
            pipeDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::LessOrEqual;
            pipeDesc.renderState.rasterState.frontCounterClockwise = false;
            pipeDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::Back;

            Msg("* [SkinnedHQ1W] About to call createGraphicsPipeline (36-byte)...");
            s_skinnedHQ1WPipeline = nvDevice->createGraphicsPipeline(pipeDesc, framebuffer);
            Msg("* [SkinnedHQ1W] Result: %s", s_skinnedHQ1WPipeline ? "OK" : "FAILED");
        }
        s_skinnedHQ1WInitialized = true;
    }

    if (!s_skinnedHQ4WInitialized) {
        auto vsResult = shaderLoader->LoadVertexShader("bindless_skinned_4w", "main");
        if (vsResult.handle) {
            s_skinnedHQ4WVS = vsResult.handle;

            constexpr u32 stride = 40;
            nvrhi::VertexAttributeDesc attribs[] = {
                nvrhi::VertexAttributeDesc().setName("POSITION").setFormat(nvrhi::Format::RGBA32_FLOAT).setOffset(0).setElementStride(stride),
                nvrhi::VertexAttributeDesc().setName("NORMAL").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(16).setElementStride(stride),
                nvrhi::VertexAttributeDesc().setName("TANGENT").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(20).setElementStride(stride),
                nvrhi::VertexAttributeDesc().setName("BINORMAL").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(24).setElementStride(stride),
                nvrhi::VertexAttributeDesc().setName("TEXCOORD").setFormat(nvrhi::Format::RG32_FLOAT).setOffset(28).setElementStride(stride),
                nvrhi::VertexAttributeDesc().setName("BLENDINDICES").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(36).setElementStride(stride),
            };
            s_skinnedHQ4WInputLayout = nvDevice->createInputLayout(attribs, 6, s_skinnedHQ4WVS);

            nvrhi::GraphicsPipelineDesc pipeDesc;
            pipeDesc.VS = s_skinnedHQ4WVS;
            pipeDesc.PS = s_skinnedPS;
            pipeDesc.inputLayout = s_skinnedHQ4WInputLayout;
            if (bindlessLayout) {
                pipeDesc.bindingLayouts = { s_skinnedLayout, bindlessLayout };
            } else {
                pipeDesc.bindingLayouts = { s_skinnedLayout };
            }
            pipeDesc.primType = nvrhi::PrimitiveType::TriangleList;
            pipeDesc.renderState.depthStencilState.depthTestEnable = true;
            pipeDesc.renderState.depthStencilState.depthWriteEnable = true;
            pipeDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::LessOrEqual;
            pipeDesc.renderState.rasterState.frontCounterClockwise = false;
            pipeDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::Back;

            Msg("* [SkinnedHQ4W] About to call createGraphicsPipeline (40-byte)...");
            s_skinnedHQ4WPipeline = nvDevice->createGraphicsPipeline(pipeDesc, framebuffer);
            Msg("* [SkinnedHQ4W] Result: %s", s_skinnedHQ4WPipeline ? "OK" : "FAILED");
        }
        s_skinnedHQ4WInitialized = true;
    }

    if (!s_skinnedHQ2WInitialized) {
        auto vsResult = shaderLoader->LoadVertexShader("bindless_skinned_2w", "main");
        if (vsResult.handle) {
            s_skinnedHQ2WVS = vsResult.handle;

            constexpr u32 stride = 44;
            nvrhi::VertexAttributeDesc attribs[] = {
                nvrhi::VertexAttributeDesc().setName("POSITION").setFormat(nvrhi::Format::RGBA32_FLOAT).setOffset(0).setElementStride(stride),
                nvrhi::VertexAttributeDesc().setName("NORMAL").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(16).setElementStride(stride),
                nvrhi::VertexAttributeDesc().setName("TANGENT").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(20).setElementStride(stride),
                nvrhi::VertexAttributeDesc().setName("BINORMAL").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(24).setElementStride(stride),
                nvrhi::VertexAttributeDesc().setName("TEXCOORD").setFormat(nvrhi::Format::RGBA32_FLOAT).setOffset(28).setElementStride(stride),
            };
            s_skinnedHQ2WInputLayout = nvDevice->createInputLayout(attribs, 5, s_skinnedHQ2WVS);

            nvrhi::GraphicsPipelineDesc pipeDesc;
            pipeDesc.VS = s_skinnedHQ2WVS;
            pipeDesc.PS = s_skinnedPS;
            pipeDesc.inputLayout = s_skinnedHQ2WInputLayout;
            if (bindlessLayout) {
                pipeDesc.bindingLayouts = { s_skinnedLayout, bindlessLayout };
            } else {
                pipeDesc.bindingLayouts = { s_skinnedLayout };
            }
            pipeDesc.primType = nvrhi::PrimitiveType::TriangleList;
            pipeDesc.renderState.depthStencilState.depthTestEnable = true;
            pipeDesc.renderState.depthStencilState.depthWriteEnable = true;
            pipeDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::LessOrEqual;
            pipeDesc.renderState.rasterState.frontCounterClockwise = false;
            pipeDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::Back;

            Msg("* [SkinnedHQ2W] About to call createGraphicsPipeline (44-byte)...");
            s_skinnedHQ2WPipeline = nvDevice->createGraphicsPipeline(pipeDesc, framebuffer);
            Msg("* [SkinnedHQ2W] Result: %s", s_skinnedHQ2WPipeline ? "OK" : "FAILED");
        }
        s_skinnedHQ2WInitialized = true;
    }

    Msg("* [SkinnedPipeline] All pipelines initialized");
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

        s_bindlessPipeline = nvDevice->createGraphicsPipeline(pipeDesc, framebuffer);
        if (!s_bindlessPipeline)
            return;

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
    state.viewport.addViewport(viewport);
    state.viewport.addScissorRect(nvrhi::Rect(rtDesc.width, rtDesc.height));

    cmdList->setGraphicsState(state);

    cmdList->drawIndexedIndirect(0, visibleCount);

    s_gpuDrivenDraws += visibleCount;

}

void renderBindlessSkinned(
    ng::RenderContext* ctx,
    ng::RenderDevice* device,
    const GeometryCollector* geometry,
    nvrhi::ITexture* colorRT,
    nvrhi::ITexture* depthRT,
    const BindlessForwardConfig& config)
{
    using namespace RENDER_NAMESPACE::bindless;

    if (!geometry || geometry->GetBatches().empty())
        return;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    nvrhi::ICommandList* cmdList = ctx->GetCommandList();
    if (!cmdList || !nvDevice)
        return;

    nvrhi::FramebufferDesc fbDesc;
    fbDesc.addColorAttachment(colorRT);
    fbDesc.setDepthAttachment(depthRT);
    auto framebuffer = nvDevice->createFramebuffer(fbDesc);
    if (!framebuffer)
        return;

    const auto& rtDesc = colorRT->getDesc();
    nvrhi::Viewport viewport(0.0f, static_cast<float>(rtDesc.width), 0.0f, static_cast<float>(rtDesc.height), 0.0f, 1.0f);

    auto& matBuffer = MaterialBuffer::Instance();

    nvrhi::BufferDesc staticGlobalsCbDesc;
    staticGlobalsCbDesc.byteSize = sizeof(StaticGlobals);
    staticGlobalsCbDesc.isConstantBuffer = true;
    staticGlobalsCbDesc.isVolatile = true;
    staticGlobalsCbDesc.maxVersions = 16;
    auto staticGlobalsCB = nvDevice->createBuffer(staticGlobalsCbDesc);

    StaticGlobals staticGlobals;
    FillGlobalConstants(staticGlobals);
    SunLightData sunData;
    GetSunLightData(sunData, 2.0f);
    FillSunConstants(staticGlobals, sunData);
    cmdList->writeBuffer(staticGlobalsCB, &staticGlobals, sizeof(staticGlobals));

    auto* backend = device->GetBackend();

    const auto& batches = geometry->GetBatches();

    u32 skinnedCount = 0;
    for (const auto& batch : batches) {
        if (batch.isSkinned)
            skinnedCount++;
    }

    if (skinnedCount == 0)
        return;

    if (!s_skinnedPipeline && !s_skinnedHQ1WPipeline && !s_skinnedHQ4WPipeline && !s_skinnedHQ2WPipeline)
        return;

    nvrhi::BufferDesc dynTransCbDesc;
    dynTransCbDesc.byteSize = sizeof(DynamicTransforms);
    dynTransCbDesc.isConstantBuffer = true;
    dynTransCbDesc.isVolatile = true;
    dynTransCbDesc.maxVersions = 128;
    auto dynTransformsCB = nvDevice->createBuffer(dynTransCbDesc);

    constexpr u32 MAX_BONES = 78;
    constexpr u32 BONE_STRIDE = sizeof(float) * 12;

    nvrhi::BufferDesc boneSbDesc;
    boneSbDesc.byteSize = MAX_BONES * BONE_STRIDE;
    boneSbDesc.structStride = BONE_STRIDE;
    boneSbDesc.initialState = nvrhi::ResourceStates::ShaderResource;
    boneSbDesc.keepInitialState = true;
    boneSbDesc.debugName = "BoneMatrices_SB";
    auto bonesSB = nvDevice->createBuffer(boneSbDesc);

    if (!bonesSB) {
        Msg("! [SkinnedRender] Failed to create bones structured buffer");
        return;
    }

    nvrhi::BufferDesc matIdCbDesc;
    matIdCbDesc.byteSize = 16;
    matIdCbDesc.isConstantBuffer = true;
    matIdCbDesc.isVolatile = true;
    matIdCbDesc.maxVersions = 128;
    auto matIdCB = nvDevice->createBuffer(matIdCbDesc);

    struct Float3x4 {
        float m[3][4];
    };

    xr_vector<Float3x4> boneData(MAX_BONES);
    for (u32 i = 0; i < MAX_BONES; i++) {
        boneData[i].m[0][0] = 1.0f; boneData[i].m[0][1] = 0.0f; boneData[i].m[0][2] = 0.0f; boneData[i].m[0][3] = 0.0f;
        boneData[i].m[1][0] = 0.0f; boneData[i].m[1][1] = 1.0f; boneData[i].m[1][2] = 0.0f; boneData[i].m[1][3] = 0.0f;
        boneData[i].m[2][0] = 0.0f; boneData[i].m[2][1] = 0.0f; boneData[i].m[2][2] = 1.0f; boneData[i].m[2][3] = 0.0f;
    }

    nvrhi::IDescriptorTable* bindlessTable = nullptr;
    if (backend) {
        bindlessTable = backend->GetBindlessDescriptorTable();
    }

    for (const auto& batch : batches) {
        if (!batch.isSkinned)
            continue;

        if (!batch.vertexBuffer || !batch.indexBuffer)
            continue;

        nvrhi::IGraphicsPipeline* selectedPipeline = nullptr;
        const char* pipelineName = "unknown";

        if (batch.vertexStride == 36) {
            selectedPipeline = s_skinnedHQ1WPipeline.Get();
            pipelineName = "HQ1W (36-byte)";
        } else if (batch.vertexStride == 40) {
            selectedPipeline = s_skinnedHQ4WPipeline.Get();
            pipelineName = "HQ4W (40-byte)";
        } else if (batch.vertexStride == 44) {
            selectedPipeline = s_skinnedHQ2WPipeline.Get();
            pipelineName = "HQ2W (44-byte)";
        } else if (batch.vertexStride == 24) {
            selectedPipeline = s_skinnedPipeline.Get();
            pipelineName = "non-HQ (24-byte)";
        } else if (batch.vertexStride >= 36) {
            selectedPipeline = s_skinnedHQ1WPipeline.Get();
            pipelineName = "HQ1W (fallback)";
        } else {
            selectedPipeline = s_skinnedPipeline.Get();
            pipelineName = "non-HQ (fallback)";
        }

        if (!selectedPipeline) {
            static bool s_pipelineWarningLogged = false;
            if (!s_pipelineWarningLogged) {
                Msg("! [SkinnedRender] No pipeline available for stride %u", batch.vertexStride);
                s_pipelineWarningLogged = true;
            }
            continue;
        }

        static u32 s_lastLoggedStride = 0;
        if (batch.vertexStride != s_lastLoggedStride) {
            Msg("* [SkinnedRender] Using %s pipeline for stride %u, VB=%p IB=%p",
                pipelineName, batch.vertexStride,
                batch.vertexBuffer.Get(), batch.indexBuffer.Get());
            s_lastLoggedStride = batch.vertexStride;
        }

        nvrhi::GraphicsState skinnedState;
        skinnedState.pipeline = selectedPipeline;
        skinnedState.framebuffer = framebuffer;
        skinnedState.viewport.addViewport(viewport);
        skinnedState.viewport.addScissorRect(nvrhi::Rect(rtDesc.width, rtDesc.height));

        RENDER_NAMESPACE::CKinematics* Parent = nullptr;
        u32 visualType = batch.visual ? batch.visual->getType() : 0;

        if (visualType == MT_SKELETON_GEOMDEF_ST) {
            auto* skeletonMesh = static_cast<RENDER_NAMESPACE::CSkeletonX_ST*>(batch.visual);
            Parent = skeletonMesh->GetParent();
        } else if (visualType == MT_SKELETON_GEOMDEF_PM) {
            auto* skeletonMesh = static_cast<RENDER_NAMESPACE::CSkeletonX_PM*>(batch.visual);
            Parent = skeletonMesh->GetParent();
        }

        if (!Parent)
            continue;

        Parent->CalculateBones(TRUE);

        DynamicTransforms dynTransData = {};
        FillDynamicTransforms(dynTransData, batch.worldMatrix);

        u32 boneCount = Parent->LL_BoneCount();
        u32 bonesToFill = std::min(boneCount, MAX_BONES);

        static bool s_loggedBoneCount = false;
        if (!s_loggedBoneCount) {
            Msg("* [SkinnedRender] Skeleton has %u bones, filling %u into buffer of %u",
                boneCount, bonesToFill, MAX_BONES);
            s_loggedBoneCount = true;
        }

        for (u32 i = 0; i < bonesToFill; i++) {
            const Fmatrix& bone = Parent->LL_GetTransform_R(u16(i));
            boneData[i].m[0][0] = bone._11; boneData[i].m[0][1] = bone._21; boneData[i].m[0][2] = bone._31; boneData[i].m[0][3] = bone._41;
            boneData[i].m[1][0] = bone._12; boneData[i].m[1][1] = bone._22; boneData[i].m[1][2] = bone._32; boneData[i].m[1][3] = bone._42;
            boneData[i].m[2][0] = bone._13; boneData[i].m[2][1] = bone._23; boneData[i].m[2][2] = bone._33; boneData[i].m[2][3] = bone._43;
        }

        cmdList->writeBuffer(dynTransformsCB, &dynTransData, sizeof(dynTransData));

        cmdList->writeBuffer(bonesSB, boneData.data(), MAX_BONES * sizeof(Float3x4));

        struct SkinnedMaterialCB {
            u32 materialID;
            u32 pad0, pad1, pad2;
        } matIdData;
        matIdData.materialID = batch.bindlessMaterialID;
        matIdData.pad0 = matIdData.pad1 = matIdData.pad2 = 0;
        cmdList->writeBuffer(matIdCB, &matIdData, sizeof(matIdData));

        nvrhi::BindingSetDesc skinnedBindDesc;
        skinnedBindDesc.bindings = {
            nvrhi::BindingSetItem::ConstantBuffer(0, dynTransformsCB),
            nvrhi::BindingSetItem::ConstantBuffer(2, staticGlobalsCB),
            nvrhi::BindingSetItem::StructuredBuffer_SRV(3, bonesSB),
            nvrhi::BindingSetItem::ConstantBuffer(4, matIdCB),
            nvrhi::BindingSetItem::StructuredBuffer_SRV(8, matBuffer.GetBuffer()),
            nvrhi::BindingSetItem::Sampler(0, s_linearSampler),
        };
        auto skinnedBindingSet = nvDevice->createBindingSet(skinnedBindDesc, s_skinnedLayout);

        skinnedState.bindings = { skinnedBindingSet };
        if (bindlessTable) {
            skinnedState.addBindingSet(bindlessTable);
        }

        skinnedState.vertexBuffers = { {batch.vertexBuffer, 0, 0} };
        skinnedState.indexBuffer = { batch.indexBuffer, nvrhi::Format::R16_UINT, 0 };

        cmdList->setGraphicsState(skinnedState);

        cmdList->drawIndexed(
            nvrhi::DrawArguments()
                .setVertexCount(batch.indexCount)
                .setStartIndexLocation(batch.startIndex)
                .setStartVertexLocation(batch.baseVertex)
        );
    }
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

            renderBindlessSkinned(
                ctx,
                data.device,
                data.geometry,
                colorRT,
                depthRT,
                data.bindlessConfig
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
