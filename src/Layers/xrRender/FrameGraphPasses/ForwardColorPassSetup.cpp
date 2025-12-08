// xrRender/FrameGraphPasses/ForwardColorPassSetup.cpp
#include "stdafx.h"
#include "ForwardColorPassSetup.h"
#include "ShaderConstants.h"  // CB layout definitions and FillGlobalConstants/FillDynamicTransforms
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"  // For loading bindless shaders
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
#include "Layers/xrRender/GPUCullingManager.h"  // For IndirectDrawArgs struct
#include "Layers/xrRender/Backend/D3D12Backend.h"  // For SM6 bindless descriptor heap
#include "Layers/xrRender/Bindless/MaterialBuffer.h"
#include "xrCore/FMesh.hpp"  // For MT_NORMAL, MT_TREE, etc.

namespace xray::render::RENDER_NAMESPACE
{
    extern float r__dtex_range;
}

namespace xray::render::RENDER_NAMESPACE::passes {

// Render phase enum for marker tracking
enum class RenderPhase { None, Opaque, AlphaTested, Transparent };

// ═══════════════════════════════════════════════════════
//  BINDLESS FORWARD RENDERING (GPU-DRIVEN)
// ═══════════════════════════════════════════════════════
// Single PSO, single multi-draw, all materials via buffer
// This is the high-performance GPU-driven rendering path

// Static resources for bindless rendering (initialized once)
// SM6 bindless: Textures accessed via ResourceDescriptorHeap[index]
static nvrhi::GraphicsPipelineHandle s_bindlessPipeline;
static nvrhi::BindingLayoutHandle s_bindlessLayout;
static nvrhi::InputLayoutHandle s_bindlessInputLayout;
static nvrhi::SamplerHandle s_linearSampler;
static nvrhi::ShaderHandle s_bindlessVS;
static nvrhi::ShaderHandle s_bindlessPS;
static nvrhi::BufferHandle s_drawIndexBuffer;  // Per-instance buffer with [0,1,2,3,...]
static bool s_bindlessInitialized = false;

// ═══════════════════════════════════════════════════════
//  SKINNED MESH PIPELINE (Per-draw bone matrices)
// ═══════════════════════════════════════════════════════
// Separate pipeline for skeleton meshes with different vertex format
// Non-HQ: 24 bytes (SHORT4 position, SHORT2 UV)
static nvrhi::GraphicsPipelineHandle s_skinnedPipeline;
static nvrhi::BindingLayoutHandle s_skinnedLayout;
static nvrhi::InputLayoutHandle s_skinnedInputLayout;
static nvrhi::ShaderHandle s_skinnedVS;
static nvrhi::ShaderHandle s_skinnedPS;
static bool s_skinnedInitialized = false;

// HQ Skinned 1W: 36 bytes (FLOAT4 position, FLOAT2 UV, bone index in normal.w)
// Covers 1W_HQ (36 bytes) - RenderMode 2 (RM_SINGLE_HQ) and 4 (RM_SKINNING_1B_HQ)
static nvrhi::GraphicsPipelineHandle s_skinnedHQ1WPipeline;
static nvrhi::InputLayoutHandle s_skinnedHQ1WInputLayout;
static nvrhi::ShaderHandle s_skinnedHQ1WVS;
static bool s_skinnedHQ1WInitialized = false;

// HQ Skinned 4W: 40 bytes (FLOAT4 position, FLOAT2 UV, BLENDINDICES)
// Covers 4W_HQ (40 bytes) - RenderMode 10 (RM_SKINNING_4B_HQ)
static nvrhi::GraphicsPipelineHandle s_skinnedHQ4WPipeline;
static nvrhi::InputLayoutHandle s_skinnedHQ4WInputLayout;
static nvrhi::ShaderHandle s_skinnedHQ4WVS;
static bool s_skinnedHQ4WInitialized = false;

// HQ Skinned 2W/3W: 44 bytes (FLOAT4 position, FLOAT4 UV+indices)
// Covers 2W_HQ and 3W_HQ (44 bytes) - RenderModes 6, 8
static nvrhi::GraphicsPipelineHandle s_skinnedHQ2WPipeline;
static nvrhi::InputLayoutHandle s_skinnedHQ2WInputLayout;
static nvrhi::ShaderHandle s_skinnedHQ2WVS;
static bool s_skinnedHQ2WInitialized = false;

// ═══════════════════════════════════════════════════════════════════════════
//  FORWARD DECLARATIONS
// ═══════════════════════════════════════════════════════════════════════════
static void InitializeBindlessPipeline(ng::RenderDevice* device, nvrhi::IFramebuffer* framebuffer);
static void InitializeSkinnedPipelines(ng::RenderDevice* device, nvrhi::IFramebuffer* framebuffer);

// ═══════════════════════════════════════════════════════════════════════════
//  EAGER PIPELINE INITIALIZATION
// ═══════════════════════════════════════════════════════════════════════════
// Called once at device creation to avoid frame hitches on first render

void InitializeForwardPipelines(ng::RenderDevice* device)
{
    if (!device)
        return;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    if (!nvDevice)
        return;

    Msg("* [ForwardPass] Initializing forward pipelines...");

    // Create dummy framebuffer with expected formats for pipeline creation
    // Forward pass uses: RGBA16_FLOAT color, D24S8 depth
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

    // Initialize all pipelines
    InitializeBindlessPipeline(device, dummyFramebuffer);
    InitializeSkinnedPipelines(device, dummyFramebuffer);

    Msg("* [ForwardPass] Pipeline initialization complete");
}

void ShutdownForwardPipelines()
{
    // Release all pipeline resources
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

// ═══════════════════════════════════════════════════════════════════════════
//  BINDLESS PIPELINE INITIALIZATION
// ═══════════════════════════════════════════════════════════════════════════
static void InitializeBindlessPipeline(ng::RenderDevice* device, nvrhi::IFramebuffer* framebuffer)
{
    if (s_bindlessInitialized)
        return;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    if (!nvDevice)
        return;

    // Load shaders using ShaderLoader
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

    // Create sampler
    nvrhi::SamplerDesc samplerDesc;
    samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Repeat);
    samplerDesc.setAllFilters(true);
    samplerDesc.setMaxAnisotropy(16.0f);
    s_linearSampler = nvDevice->createSampler(samplerDesc);

    // Create binding layout
    nvrhi::BindingLayoutDesc layoutDesc;
    layoutDesc.visibility = nvrhi::ShaderType::All;
    layoutDesc.bindings = {
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(2),  // static_globals (b2)
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(4),  // LightingConstants (b4)
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(8),    // g_Materials
        nvrhi::BindingLayoutItem::Sampler(0),
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(14),   // g_InstanceData
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(15),   // g_CompactBatchIndices
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(16),   // g_CompactMaterialIDs
    };
    s_bindlessLayout = nvDevice->createBindingLayout(layoutDesc);

    // Create input layout matching UnifiedVertex format (48 bytes)
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

    // Create draw index buffer (per-instance): [0,1,2,3,...]
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

    // Upload draw indices
    if (GEnv.Backend) {
        GEnv.Backend->UploadBufferData(s_drawIndexBuffer, drawIndices.data(), MAX_DRAWS * sizeof(u32));
    }

    Msg("* [BindlessForward] Created draw index buffer (65536 entries)");

    // Create graphics pipeline
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

// ═══════════════════════════════════════════════════════════════════════════
//  SKINNED PIPELINE INITIALIZATION
// ═══════════════════════════════════════════════════════════════════════════
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

    // Create shared skinned binding layout and pixel shader (if not already created)
    if (!s_skinnedLayout) {
        nvrhi::BindingLayoutDesc skinnedLayoutDesc;
        skinnedLayoutDesc.visibility = nvrhi::ShaderType::All;
        skinnedLayoutDesc.bindings = {
            nvrhi::BindingLayoutItem::VolatileConstantBuffer(0),  // dynamic_transforms (b0)
            nvrhi::BindingLayoutItem::VolatileConstantBuffer(2),  // static_globals (b2)
            nvrhi::BindingLayoutItem::VolatileConstantBuffer(3),  // bones (b3)
            nvrhi::BindingLayoutItem::VolatileConstantBuffer(4),  // SkinnedMaterialCB (b4)
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(8),    // g_Materials
            nvrhi::BindingLayoutItem::Sampler(0),
        };
        s_skinnedLayout = nvDevice->createBindingLayout(skinnedLayoutDesc);
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

    // Create sampler if not exists
    if (!s_linearSampler) {
        nvrhi::SamplerDesc samplerDesc;
        samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Repeat);
        samplerDesc.setAllFilters(true);
        samplerDesc.setMaxAnisotropy(16.0f);
        s_linearSampler = nvDevice->createSampler(samplerDesc);
    }

    // ═══════════════════════════════════════════════════════
    //  SKINNED NON-HQ PIPELINE (24-byte format)
    // ═══════════════════════════════════════════════════════
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

            s_skinnedPipeline = nvDevice->createGraphicsPipeline(pipeDesc, framebuffer);
            Msg("* [SkinnedPipeline] Initialized 24-byte non-HQ pipeline");
        }
        s_skinnedInitialized = true;
    }

    // ═══════════════════════════════════════════════════════
    //  SKINNED HQ 1W PIPELINE (36-byte format)
    // ═══════════════════════════════════════════════════════
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

            s_skinnedHQ1WPipeline = nvDevice->createGraphicsPipeline(pipeDesc, framebuffer);
            Msg("* [SkinnedHQ1W] Initialized 36-byte 1W_HQ pipeline");
        }
        s_skinnedHQ1WInitialized = true;
    }

    // ═══════════════════════════════════════════════════════
    //  SKINNED HQ 4W PIPELINE (40-byte format)
    // ═══════════════════════════════════════════════════════
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

            s_skinnedHQ4WPipeline = nvDevice->createGraphicsPipeline(pipeDesc, framebuffer);
            Msg("* [SkinnedHQ4W] Initialized 40-byte 4W_HQ pipeline");
        }
        s_skinnedHQ4WInitialized = true;
    }

    // ═══════════════════════════════════════════════════════
    //  SKINNED HQ 2W/3W PIPELINE (44-byte format)
    // ═══════════════════════════════════════════════════════
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

            s_skinnedHQ2WPipeline = nvDevice->createGraphicsPipeline(pipeDesc, framebuffer);
            Msg("* [SkinnedHQ2W] Initialized 44-byte 2W_HQ pipeline");
        }
        s_skinnedHQ2WInitialized = true;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  SKINNED MESH RENDERING (Separate Pass)
// ═══════════════════════════════════════════════════════════════════════════
// Renders skeleton meshes with per-draw bone matrices.
// Separated from bindless forward pass for easier debugging in RenderDoc.

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

    // Finalize any pending materials (register textures to bindless descriptor heap)
    if (materialCache) {
        materialCache->FinalizePendingMaterials(ctx);
    }

    // Upload material buffer to GPU
    auto& matBuffer = MaterialBuffer::Instance();
    matBuffer.Upload(ctx);

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    nvrhi::ICommandList* cmdList = ctx->GetCommandList();
    if (!cmdList)
        return;

    // ═══════════════════════════════════════════════════════
    //  LAZY INITIALIZATION OF BINDLESS RESOURCES
    // ═══════════════════════════════════════════════════════
    if (!s_bindlessInitialized) {
        // Load shaders using ShaderLoader
        auto* shaderLoader = GEnv.Render->GetShaderLoader();
        if (!shaderLoader)
            return;

        auto vsResult = shaderLoader->LoadVertexShader("bindless_forward", "main");
        auto psResult = shaderLoader->LoadPixelShader("bindless_forward", "main");

        if (!vsResult.handle || !psResult.handle)
            return;

        s_bindlessVS = vsResult.handle;
        s_bindlessPS = psResult.handle;

        // Create sampler
        nvrhi::SamplerDesc samplerDesc;
        samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Repeat);
        samplerDesc.setAllFilters(true);
        samplerDesc.setMaxAnisotropy(16.0f);
        s_linearSampler = nvDevice->createSampler(samplerDesc);

        // Create binding layout
        // SM6 Bindless: Textures accessed via ResourceDescriptorHeap[index]
        // No explicit texture bindings needed - just materials buffer with indices
        nvrhi::BindingLayoutDesc layoutDesc;
        layoutDesc.visibility = nvrhi::ShaderType::All;
        layoutDesc.bindings = {
            nvrhi::BindingLayoutItem::VolatileConstantBuffer(2),  // static_globals (b2) - engine matrices - volatile
            nvrhi::BindingLayoutItem::VolatileConstantBuffer(4),  // LightingConstants (b4) - volatile
            // b5 removed - shader uses SV_DrawID for multi-draw instead of per-draw CB
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(8),    // g_Materials (contains descriptor indices)
            nvrhi::BindingLayoutItem::Sampler(0),
            // SM6 bindless: No texture arrays - textures accessed via ResourceDescriptorHeap[NonUniformResourceIndex(index)]
            // GPU-driven rendering buffers (for culled rendering)
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(14),   // g_InstanceData (world matrices)
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(15),   // g_CompactBatchIndices
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(16),   // g_CompactMaterialIDs
        };
        s_bindlessLayout = nvDevice->createBindingLayout(layoutDesc);

        // Create input layout matching UnifiedVertex format for GPU-driven rendering
        // UnifiedVertex has pre-unpacked UVs and vertex color:
        //   Position:  float3    at offset  0 (12 bytes)
        //   Normal:    D3DCOLOR  at offset 12 (4 bytes) - packed [-1,1] -> [0,1], A=hemi
        //   Tangent:   D3DCOLOR  at offset 16 (4 bytes) - packed
        //   Binormal:  D3DCOLOR  at offset 20 (4 bytes) - packed
        //   TexCoord0: float2    at offset 24 (8 bytes) - PRE-UNPACKED base UV
        //   TexCoord1: float2    at offset 32 (8 bytes) - lightmap UV
        //   Color:     D3DCOLOR  at offset 40 (4 bytes) - vertex color
        //   Flags:     uint      at offset 44 (4 bytes) - reserved
        // Total stride: 48 bytes
        //
        // PLUS per-instance draw index (buffer index 1):
        //   DrawIndex: uint at offset 0 (4 bytes) - PER_INSTANCE step rate 1
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
                .setFormat(nvrhi::Format::BGRA8_UNORM)  // D3DCOLOR = packed normal + hemi
                .setBufferIndex(0)
                .setOffset(12)
                .setElementStride(vertexStride),
            nvrhi::VertexAttributeDesc()
                .setName("TANGENT")
                .setFormat(nvrhi::Format::BGRA8_UNORM)  // D3DCOLOR = packed tangent
                .setBufferIndex(0)
                .setOffset(16)
                .setElementStride(vertexStride),
            nvrhi::VertexAttributeDesc()
                .setName("BINORMAL")
                .setFormat(nvrhi::Format::BGRA8_UNORM)  // D3DCOLOR = packed binormal
                .setBufferIndex(0)
                .setOffset(20)
                .setElementStride(vertexStride),
            nvrhi::VertexAttributeDesc()
                .setName("TEXCOORD")
                .setFormat(nvrhi::Format::RG32_FLOAT)   // float2 = pre-unpacked UV
                .setArraySize(2)                        // TEXCOORD0 at offset 24, TEXCOORD1 at offset 32
                .setBufferIndex(0)
                .setOffset(24)
                .setElementStride(vertexStride),
            nvrhi::VertexAttributeDesc()
                .setName("COLOR")
                .setFormat(nvrhi::Format::BGRA8_UNORM)  // D3DCOLOR = vertex color
                .setBufferIndex(0)
                .setOffset(40)
                .setElementStride(vertexStride),
            // Per-instance draw index (from second vertex buffer)
            nvrhi::VertexAttributeDesc()
                .setName("DRAWINDEX")
                .setFormat(nvrhi::Format::R32_UINT)     // uint draw index
                .setBufferIndex(1)                      // Second vertex buffer slot
                .setOffset(0)
                .setElementStride(4)                    // 4 bytes per uint
                .setIsInstanced(true),                  // PER_INSTANCE with step rate 1
        };
        s_bindlessInputLayout = nvDevice->createInputLayout(vertexAttribs, 7, s_bindlessVS);

        // Create draw index buffer (per-instance): [0,1,2,3,...]
        // This allows StartInstanceLocation in indirect args to select the draw index
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

        // Upload draw indices using backend upload (handles sync)
        if (GEnv.Backend) {
            GEnv.Backend->UploadBufferData(s_drawIndexBuffer, drawIndices.data(), MAX_DRAWS * sizeof(u32));
        }

        Msg("* [BindlessForward] Created draw index buffer (65536 entries)");

        // SM6 bindless: No placeholder texture needed
        // Missing textures handled by INVALID_TEXTURE_INDEX check in shader

        // Create graphics pipeline
        // SM6.6 bindless requires both our regular layout AND the D3D12 backend's bindless layout
        nvrhi::GraphicsPipelineDesc pipeDesc;
        pipeDesc.VS = s_bindlessVS;
        pipeDesc.PS = s_bindlessPS;
        pipeDesc.inputLayout = s_bindlessInputLayout;

        // Get bindless layout from D3D12 backend for ResourceDescriptorHeap access
        auto* backend = device->GetBackend();
        nvrhi::IBindingLayout* bindlessLayout = backend ? backend->GetBindlessLayout() : nullptr;

        if (bindlessLayout) {
            pipeDesc.bindingLayouts = { s_bindlessLayout, bindlessLayout };
        } else {
            pipeDesc.bindingLayouts = { s_bindlessLayout };
        }

        pipeDesc.primType = nvrhi::PrimitiveType::TriangleList;
        // Use LessEqual to support alpha-tested geometry (not in depth prepass)
        // TODO: Create separate pipelines for opaque (Equal) and alpha-tested (LessEqual)
        pipeDesc.renderState.depthStencilState.depthTestEnable = true;
        pipeDesc.renderState.depthStencilState.depthWriteEnable = true;  // Alpha-tested needs to write depth
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

    // ═══════════════════════════════════════════════════════
    //  SETUP FRAMEBUFFER AND RENDER STATE
    // ═══════════════════════════════════════════════════════
    nvrhi::FramebufferDesc fbDesc;
    fbDesc.addColorAttachment(colorRT);
    fbDesc.setDepthAttachment(depthRT);
    auto framebuffer = nvDevice->createFramebuffer(fbDesc);

    // ═══════════════════════════════════════════════════════
    //  CREATE CONSTANT BUFFERS
    // ═══════════════════════════════════════════════════════
    // Note: Buffer size MUST match struct size exactly for D3D11 constant buffers
    // (partial updates via UpdateSubresource are not allowed)

    // Lighting constants struct - 96 bytes (6 x float4)
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
    cbDesc.byteSize = sizeof(LightingConstants);  // Must match exactly!
    cbDesc.isConstantBuffer = true;
    cbDesc.isVolatile = true;  // Per-frame data, no state tracking needed
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

    // NOTE: PerDrawConstants (b5) removed - shader uses SV_DrawID for multi-draw instead

    // ═══════════════════════════════════════════════════════
    //  CREATE STATIC_GLOBALS BUFFER (engine matrices + lighting)
    // ═══════════════════════════════════════════════════════
    cbDesc.byteSize = sizeof(StaticGlobals);  // 768 bytes
    auto staticGlobalsCB = nvDevice->createBuffer(cbDesc);

    // Fill static globals with view/projection matrices
    StaticGlobals staticGlobals;
    FillGlobalConstants(staticGlobals);

    // CRITICAL: Fill sun lighting data! Shader reads L_sun_color/L_sun_dir_w from static_globals
    SunLightData sunData;
    GetSunLightData(sunData, 2.0f);  // HDR multiplier
    FillSunConstants(staticGlobals, sunData);

    cmdList->writeBuffer(staticGlobalsCB, &staticGlobals, sizeof(staticGlobals));

    // ═══════════════════════════════════════════════════════
    //  CREATE BINDING SET (SM6 Bindless - No texture arrays)
    // ═══════════════════════════════════════════════════════
    // SM6 bindless uses ResourceDescriptorHeap[index] for texture access
    // MaterialData contains descriptor indices (diffuseIndex, normalIndex, etc.)

    nvrhi::BindingSetDesc bindDesc;
    bindDesc.bindings = {
        nvrhi::BindingSetItem::ConstantBuffer(2, staticGlobalsCB),  // b2 - engine matrices (m_VP, etc.)
        nvrhi::BindingSetItem::ConstantBuffer(4, lightingCB),       // b4 - our lighting constants
        // b5 removed - shader uses SV_DrawID instead of per-draw CB for multi-draw
        nvrhi::BindingSetItem::StructuredBuffer_SRV(8, matBuffer.GetBuffer()),  // g_Materials (has descriptor indices)
        nvrhi::BindingSetItem::Sampler(0, s_linearSampler),
        // SM6 bindless: No texture arrays - textures accessed via ResourceDescriptorHeap
        // GPU-driven rendering buffers (may be null if not using GPU culling)
        nvrhi::BindingSetItem::StructuredBuffer_SRV(14, config.instanceBuffer),           // g_InstanceData
        nvrhi::BindingSetItem::StructuredBuffer_SRV(15, config.compactBatchIndicesBuffer), // g_CompactBatchIndices
        nvrhi::BindingSetItem::StructuredBuffer_SRV(16, config.compactMaterialIDBuffer),  // g_CompactMaterialIDs
    };

    auto bindingSet = nvDevice->createBindingSet(bindDesc, s_bindlessLayout);
    if (!bindingSet)
        return;

    // ═══════════════════════════════════════════════════════
    //  RENDER VISIBLE BATCHES
    // ═══════════════════════════════════════════════════════
    // Set viewport
    const auto& rtDesc = colorRT->getDesc();
    nvrhi::Viewport viewport(0.0f, static_cast<float>(rtDesc.width), 0.0f, static_cast<float>(rtDesc.height), 0.0f, 1.0f);

    // Track stats
    static u32 s_gpuDrivenDraws = 0;

    // ═══════════════════════════════════════════════════════
    //  GPU-DRIVEN CULLED RENDERING
    // ═══════════════════════════════════════════════════════
    // Only visible batches are drawn using indirect draw commands
    // Draw args come from GPU culling's compact buffer
    if (!config.UseGPUCulling() || !config.UseMegaBuffers()) {
        // GPU culling not available - skip bindless forward
        // (regular deferred path will handle rendering)
        return;
    }

    // Draw ALL object slots - culled batches have instanceCount=0 (GPU skips them)
    // This avoids needing CPU readback of visible count from previous frame
    const u32 visibleCount = config.totalObjectCount;

    // Validate all required resources before draw loop
    if (!s_bindlessPipeline || !framebuffer || !bindingSet) {
        return;
    }

    // sizeof(IndirectDrawArgs) = 20 bytes per draw
    constexpr u32 INDIRECT_ARGS_SIZE = 20;

    // Set up base graphics state with indirect params buffer
    nvrhi::GraphicsState state;
    state.pipeline = s_bindlessPipeline;
    state.framebuffer = framebuffer;
    state.bindings = { bindingSet };

    // SM6.6 bindless: Add the descriptor table from D3D12 backend
    // IDescriptorTable derives from IBindingSet, so we add it to bindings
    auto* backend = device->GetBackend();
    if (backend) {
        auto* bindlessTable = backend->GetBindlessDescriptorTable();
        if (bindlessTable) {
            state.addBindingSet(bindlessTable);
        }
    }

    // Validate draw index buffer exists
    if (!s_drawIndexBuffer) {
        Msg("! [BindlessForward] Draw index buffer not initialized!");
        return;
    }

    state.vertexBuffers = {
        {config.megaVertexBuffer, 0, 0},    // Slot 0: Per-vertex geometry (stride 48)
        {s_drawIndexBuffer, 1, 0}           // Slot 1: Per-instance draw indices (stride 4)
    };
    state.indexBuffer = { config.megaIndexBuffer, nvrhi::Format::R32_UINT, 0 };
    state.indirectParams = config.compactDrawArgsBuffer;  // Indirect args buffer
    state.viewport.addViewport(viewport);
    state.viewport.addScissorRect(nvrhi::Rect(rtDesc.width, rtDesc.height));

    // ═══════════════════════════════════════════════════════
    //  SINGLE MULTI-DRAW INDIRECT CALL (GPU-DRIVEN)
    // ═══════════════════════════════════════════════════════
    // This is the ENTIRE POINT of GPU-driven rendering:
    // - ONE pipeline bind
    // - ONE vertex/index buffer bind
    // - ONE multi-draw call for ALL objects
    // Shader uses SV_DrawID to index into g_CompactBatchIndices/g_CompactMaterialIDs

    cmdList->setGraphicsState(state);

    // Single multi-draw indirect: Draw ALL visible objects in one API call
    // D3D12 ExecuteIndirect reads visibleCount draw args from compactDrawArgsBuffer
    cmdList->drawIndexedIndirect(0, visibleCount);

    s_gpuDrivenDraws += visibleCount;

    // Skinned meshes are rendered in a separate pass (renderBindlessSkinned)
}

// ═══════════════════════════════════════════════════════════════════════════
//  SKINNED MESH RENDERING PASS
// ═══════════════════════════════════════════════════════════════════════════
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

    // ═══════════════════════════════════════════════════════
    //  SETUP FRAMEBUFFER AND RENDER STATE
    // ═══════════════════════════════════════════════════════
    nvrhi::FramebufferDesc fbDesc;
    fbDesc.addColorAttachment(colorRT);
    fbDesc.setDepthAttachment(depthRT);
    auto framebuffer = nvDevice->createFramebuffer(fbDesc);
    if (!framebuffer)
        return;

    // Set viewport
    const auto& rtDesc = colorRT->getDesc();
    nvrhi::Viewport viewport(0.0f, static_cast<float>(rtDesc.width), 0.0f, static_cast<float>(rtDesc.height), 0.0f, 1.0f);

    // Get material buffer reference
    auto& matBuffer = MaterialBuffer::Instance();

    // Create static_globals buffer for skinned rendering
    nvrhi::BufferDesc staticGlobalsCbDesc;
    staticGlobalsCbDesc.byteSize = sizeof(StaticGlobals);
    staticGlobalsCbDesc.isConstantBuffer = true;
    staticGlobalsCbDesc.isVolatile = true;
    staticGlobalsCbDesc.maxVersions = 16;
    auto staticGlobalsCB = nvDevice->createBuffer(staticGlobalsCbDesc);

    // Fill static globals
    StaticGlobals staticGlobals;
    FillGlobalConstants(staticGlobals);
    SunLightData sunData;
    GetSunLightData(sunData, 2.0f);
    FillSunConstants(staticGlobals, sunData);
    cmdList->writeBuffer(staticGlobalsCB, &staticGlobals, sizeof(staticGlobals));

    // Get D3D12 backend for bindless descriptor table
    auto* backend = device->GetBackend();

    const auto& batches = geometry->GetBatches();

    // Count skinned batches
    u32 skinnedCount = 0;
    for (const auto& batch : batches) {
        if (batch.isSkinned)
            skinnedCount++;
    }

    if (skinnedCount == 0)
        return;

    // ═══════════════════════════════════════════════════════
    //  LAZY INITIALIZATION OF SKINNED MESH PIPELINE
    // ═══════════════════════════════════════════════════════
    // Separate pipeline for skeleton meshes with per-draw bone matrices
    if (!s_skinnedInitialized) {
        auto* shaderLoader = GEnv.Render->GetShaderLoader();
        if (!shaderLoader)
            return;

        auto skinnedVsResult = shaderLoader->LoadVertexShader("bindless_skinned", "main");
        auto skinnedPsResult = shaderLoader->LoadPixelShader("bindless_skinned", "main");

        if (!skinnedVsResult.handle || !skinnedPsResult.handle) {
            Msg("! [SkinnedPipeline] Failed to load bindless_skinned shaders");
            // Don't return - allow static geometry to render
        }
        else {
            s_skinnedVS = skinnedVsResult.handle;
            s_skinnedPS = skinnedPsResult.handle;

            // Create skinned binding layout
            // b0=dynamic_transforms (m_W, m_VP), b1=shader_params, b2=static_globals
            // b3=bones, b4=SkinnedMaterialCB
            nvrhi::BindingLayoutDesc skinnedLayoutDesc;
            skinnedLayoutDesc.visibility = nvrhi::ShaderType::All;
            skinnedLayoutDesc.bindings = {
                nvrhi::BindingLayoutItem::VolatileConstantBuffer(0),  // dynamic_transforms (b0) - m_W, m_VP, etc.
                nvrhi::BindingLayoutItem::VolatileConstantBuffer(2),  // static_globals (b2) - engine matrices, lighting
                nvrhi::BindingLayoutItem::VolatileConstantBuffer(3),  // sbones_array (b3) - 78 * float3x4 = 3744 bytes
                nvrhi::BindingLayoutItem::VolatileConstantBuffer(4),  // SkinnedMaterialCB (b4) - material ID
                nvrhi::BindingLayoutItem::StructuredBuffer_SRV(8),    // g_Materials
                nvrhi::BindingLayoutItem::Sampler(0),
            };
            s_skinnedLayout = nvDevice->createBindingLayout(skinnedLayoutDesc);

            // Create skinned input layout matching vertHW_1W format (24 bytes)
            // From FSkinnedTypes.h: Position(SHORT4), Normal(D3DCOLOR), Tangent(D3DCOLOR), Binormal(D3DCOLOR), TexCoord(SHORT2)
            constexpr u32 skinnedStride = 24;
            nvrhi::VertexAttributeDesc skinnedAttribs[] = {
                nvrhi::VertexAttributeDesc()
                    .setName("POSITION")
                    .setFormat(nvrhi::Format::RGBA16_SNORM)   // SHORT4 quantized position
                    .setBufferIndex(0)
                    .setOffset(0)
                    .setElementStride(skinnedStride),
                nvrhi::VertexAttributeDesc()
                    .setName("NORMAL")
                    .setFormat(nvrhi::Format::BGRA8_UNORM)    // D3DCOLOR: normal.xyz + bone index in .w
                    .setBufferIndex(0)
                    .setOffset(8)
                    .setElementStride(skinnedStride),
                nvrhi::VertexAttributeDesc()
                    .setName("TANGENT")
                    .setFormat(nvrhi::Format::BGRA8_UNORM)    // D3DCOLOR: tangent
                    .setBufferIndex(0)
                    .setOffset(12)
                    .setElementStride(skinnedStride),
                nvrhi::VertexAttributeDesc()
                    .setName("BINORMAL")
                    .setFormat(nvrhi::Format::BGRA8_UNORM)    // D3DCOLOR: binormal
                    .setBufferIndex(0)
                    .setOffset(16)
                    .setElementStride(skinnedStride),
                nvrhi::VertexAttributeDesc()
                    .setName("TEXCOORD")
                    .setFormat(nvrhi::Format::RG16_SNORM)     // SHORT2 quantized UV
                    .setBufferIndex(0)
                    .setOffset(20)
                    .setElementStride(skinnedStride),
            };
            s_skinnedInputLayout = nvDevice->createInputLayout(skinnedAttribs, 5, s_skinnedVS);

            // Create skinned graphics pipeline
            nvrhi::GraphicsPipelineDesc skinnedPipeDesc;
            skinnedPipeDesc.VS = s_skinnedVS;
            skinnedPipeDesc.PS = s_skinnedPS;
            skinnedPipeDesc.inputLayout = s_skinnedInputLayout;

            // Get bindless layout from D3D12 backend for ResourceDescriptorHeap access
            auto* backend = device->GetBackend();
            nvrhi::IBindingLayout* bindlessLayout = backend ? backend->GetBindlessLayout() : nullptr;

            if (bindlessLayout) {
                skinnedPipeDesc.bindingLayouts = { s_skinnedLayout, bindlessLayout };
            }
            else {
                skinnedPipeDesc.bindingLayouts = { s_skinnedLayout };
            }

            skinnedPipeDesc.primType = nvrhi::PrimitiveType::TriangleList;
            skinnedPipeDesc.renderState.depthStencilState.depthTestEnable = true;
            skinnedPipeDesc.renderState.depthStencilState.depthWriteEnable = true;
            skinnedPipeDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::LessOrEqual;
            skinnedPipeDesc.renderState.rasterState.frontCounterClockwise = false;
            skinnedPipeDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::Back;

            nvrhi::FramebufferDesc skinnedFbDesc;
            skinnedFbDesc.addColorAttachment(colorRT);
            skinnedFbDesc.setDepthAttachment(depthRT);
            auto skinnedFramebuffer = nvDevice->createFramebuffer(skinnedFbDesc);

            s_skinnedPipeline = nvDevice->createGraphicsPipeline(skinnedPipeDesc, skinnedFramebuffer);
            if (!s_skinnedPipeline) {
                Msg("! [SkinnedPipeline] Failed to create skinned graphics pipeline");
            }
            else {
                Msg("* [SkinnedPipeline] Initialized skinned mesh pipeline");
            }

            s_skinnedInitialized = true;

            // ═══════════════════════════════════════════════════════
            //  SKINNED HQ 1W PIPELINE (36-byte format) - Most common!
            // ═══════════════════════════════════════════════════════
            // RenderMode 2 (RM_SINGLE_HQ) and 4 (RM_SKINNING_1B_HQ)
            // Bone index is in normal.w (same shader as non-HQ, just different input layout)
            {
                // Reuse the same HQ vertex shader (handles both 1W and 4W by checking USE_4W_FORMAT)
                auto skinnedHQ1WVsResult = shaderLoader->LoadVertexShader("bindless_skinned_hq", "main");
                if (!skinnedHQ1WVsResult.handle) {
                    Msg("! [SkinnedHQ1W] Failed to load bindless_skinned_hq.vs");
                }
                else {
                    s_skinnedHQ1WVS = skinnedHQ1WVsResult.handle;
                    Msg("* [SkinnedHQ1W] Loaded HQ 1W vertex shader");

                    // Create 1W_HQ input layout (36 bytes) - NO BLENDINDICES
                    // From FSkinnedTypes.h dwDecl_01W_HQ:
                    //   FLOAT4 Position at 0, D3DCOLOR Normal at 16 (bone_idx*3 in .w),
                    //   D3DCOLOR Tangent at 20, D3DCOLOR Binormal at 24, FLOAT2 TexCoord at 28
                    constexpr u32 stride1W = 36;
                    nvrhi::VertexAttributeDesc attribs1W[] = {
                        nvrhi::VertexAttributeDesc()
                            .setName("POSITION")
                            .setFormat(nvrhi::Format::RGBA32_FLOAT)
                            .setBufferIndex(0)
                            .setOffset(0)
                            .setElementStride(stride1W),
                        nvrhi::VertexAttributeDesc()
                            .setName("NORMAL")
                            .setFormat(nvrhi::Format::BGRA8_UNORM)  // bone_idx*3 in .w
                            .setBufferIndex(0)
                            .setOffset(16)
                            .setElementStride(stride1W),
                        nvrhi::VertexAttributeDesc()
                            .setName("TANGENT")
                            .setFormat(nvrhi::Format::BGRA8_UNORM)
                            .setBufferIndex(0)
                            .setOffset(20)
                            .setElementStride(stride1W),
                        nvrhi::VertexAttributeDesc()
                            .setName("BINORMAL")
                            .setFormat(nvrhi::Format::BGRA8_UNORM)
                            .setBufferIndex(0)
                            .setOffset(24)
                            .setElementStride(stride1W),
                        nvrhi::VertexAttributeDesc()
                            .setName("TEXCOORD")
                            .setFormat(nvrhi::Format::RG32_FLOAT)
                            .setBufferIndex(0)
                            .setOffset(28)
                            .setElementStride(stride1W),
                    };
                    s_skinnedHQ1WInputLayout = nvDevice->createInputLayout(attribs1W, 5, s_skinnedHQ1WVS);

                    nvrhi::GraphicsPipelineDesc pipeDesc1W;
                    pipeDesc1W.VS = s_skinnedHQ1WVS;
                    pipeDesc1W.PS = s_skinnedPS;
                    pipeDesc1W.inputLayout = s_skinnedHQ1WInputLayout;

                    if (bindlessLayout) {
                        pipeDesc1W.bindingLayouts = { s_skinnedLayout, bindlessLayout };
                    }
                    else {
                        pipeDesc1W.bindingLayouts = { s_skinnedLayout };
                    }

                    pipeDesc1W.primType = nvrhi::PrimitiveType::TriangleList;
                    pipeDesc1W.renderState.depthStencilState.depthTestEnable = true;
                    pipeDesc1W.renderState.depthStencilState.depthWriteEnable = true;
                    pipeDesc1W.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::LessOrEqual;
                    pipeDesc1W.renderState.rasterState.frontCounterClockwise = false;
                    pipeDesc1W.renderState.rasterState.cullMode = nvrhi::RasterCullMode::Back;

                    s_skinnedHQ1WPipeline = nvDevice->createGraphicsPipeline(pipeDesc1W, skinnedFramebuffer);
                    if (!s_skinnedHQ1WPipeline) {
                        Msg("! [SkinnedHQ1W] Failed to create pipeline");
                    }
                    else {
                        Msg("* [SkinnedHQ1W] Initialized 36-byte 1W_HQ pipeline");
                    }
                }
                s_skinnedHQ1WInitialized = true;
            }

            // ═══════════════════════════════════════════════════════
            //  SKINNED HQ 4W PIPELINE (40-byte format)
            // ═══════════════════════════════════════════════════════
            // RenderMode 10 (RM_SKINNING_4B_HQ)
            {
                auto skinnedHQ4WVsResult = shaderLoader->LoadVertexShader("bindless_skinned_4w", "main");
                if (!skinnedHQ4WVsResult.handle) {
                    Msg("! [SkinnedHQ4W] Failed to load bindless_skinned_4w.vs");
                }
                else {
                    s_skinnedHQ4WVS = skinnedHQ4WVsResult.handle;

                    // Create 4W_HQ input layout (40 bytes) - WITH BLENDINDICES
                    constexpr u32 stride4W = 40;
                    nvrhi::VertexAttributeDesc attribs4W[] = {
                        nvrhi::VertexAttributeDesc()
                            .setName("POSITION")
                            .setFormat(nvrhi::Format::RGBA32_FLOAT)
                            .setBufferIndex(0)
                            .setOffset(0)
                            .setElementStride(stride4W),
                        nvrhi::VertexAttributeDesc()
                            .setName("NORMAL")
                            .setFormat(nvrhi::Format::BGRA8_UNORM)
                            .setBufferIndex(0)
                            .setOffset(16)
                            .setElementStride(stride4W),
                        nvrhi::VertexAttributeDesc()
                            .setName("TANGENT")
                            .setFormat(nvrhi::Format::BGRA8_UNORM)
                            .setBufferIndex(0)
                            .setOffset(20)
                            .setElementStride(stride4W),
                        nvrhi::VertexAttributeDesc()
                            .setName("BINORMAL")
                            .setFormat(nvrhi::Format::BGRA8_UNORM)
                            .setBufferIndex(0)
                            .setOffset(24)
                            .setElementStride(stride4W),
                        nvrhi::VertexAttributeDesc()
                            .setName("TEXCOORD")
                            .setFormat(nvrhi::Format::RG32_FLOAT)
                            .setBufferIndex(0)
                            .setOffset(28)
                            .setElementStride(stride4W),
                        nvrhi::VertexAttributeDesc()
                            .setName("BLENDINDICES")
                            .setFormat(nvrhi::Format::BGRA8_UNORM)
                            .setBufferIndex(0)
                            .setOffset(36)
                            .setElementStride(stride4W),
                    };
                    s_skinnedHQ4WInputLayout = nvDevice->createInputLayout(attribs4W, 6, s_skinnedHQ4WVS);

                    nvrhi::GraphicsPipelineDesc pipeDesc4W;
                    pipeDesc4W.VS = s_skinnedHQ4WVS;
                    pipeDesc4W.PS = s_skinnedPS;
                    pipeDesc4W.inputLayout = s_skinnedHQ4WInputLayout;

                    if (bindlessLayout) {
                        pipeDesc4W.bindingLayouts = { s_skinnedLayout, bindlessLayout };
                    }
                    else {
                        pipeDesc4W.bindingLayouts = { s_skinnedLayout };
                    }

                    pipeDesc4W.primType = nvrhi::PrimitiveType::TriangleList;
                    pipeDesc4W.renderState.depthStencilState.depthTestEnable = true;
                    pipeDesc4W.renderState.depthStencilState.depthWriteEnable = true;
                    pipeDesc4W.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::LessOrEqual;
                    pipeDesc4W.renderState.rasterState.frontCounterClockwise = false;
                    pipeDesc4W.renderState.rasterState.cullMode = nvrhi::RasterCullMode::Back;

                    s_skinnedHQ4WPipeline = nvDevice->createGraphicsPipeline(pipeDesc4W, skinnedFramebuffer);
                    if (!s_skinnedHQ4WPipeline) {
                        Msg("! [SkinnedHQ4W] Failed to create pipeline");
                    }
                    else {
                        Msg("* [SkinnedHQ4W] Initialized 40-byte 4W_HQ pipeline");
                    }
                }
                s_skinnedHQ4WInitialized = true;
            }

            // ═══════════════════════════════════════════════════════
            //  SKINNED HQ 2W/3W PIPELINE (44-byte format)
            // ═══════════════════════════════════════════════════════
            // RenderModes 6 (RM_SKINNING_2B_HQ) and 8 (RM_SKINNING_3B_HQ)
            // Uses FLOAT4 TEXCOORD to store UV + bone indices
            {
                auto skinnedHQ2WVsResult = shaderLoader->LoadVertexShader("bindless_skinned_2w", "main");
                if (!skinnedHQ2WVsResult.handle) {
                    Msg("! [SkinnedHQ2W] Failed to load bindless_skinned_2w.vs");
                }
                else {
                    s_skinnedHQ2WVS = skinnedHQ2WVsResult.handle;

                    // Create 2W_HQ input layout (44 bytes)
                    // FLOAT4 POSITION at 0 (16 bytes)
                    // D3DCOLOR NORMAL at 16 (4 bytes) - .w = weight
                    // D3DCOLOR TANGENT at 20 (4 bytes)
                    // D3DCOLOR BINORMAL at 24 (4 bytes)
                    // FLOAT4 TEXCOORD at 28 (16 bytes) - .xy = UV, .zw = bone indices
                    constexpr u32 stride2W = 44;
                    nvrhi::VertexAttributeDesc attribs2W[] = {
                        nvrhi::VertexAttributeDesc()
                            .setName("POSITION")
                            .setFormat(nvrhi::Format::RGBA32_FLOAT)
                            .setBufferIndex(0)
                            .setOffset(0)
                            .setElementStride(stride2W),
                        nvrhi::VertexAttributeDesc()
                            .setName("NORMAL")
                            .setFormat(nvrhi::Format::BGRA8_UNORM)
                            .setBufferIndex(0)
                            .setOffset(16)
                            .setElementStride(stride2W),
                        nvrhi::VertexAttributeDesc()
                            .setName("TANGENT")
                            .setFormat(nvrhi::Format::BGRA8_UNORM)
                            .setBufferIndex(0)
                            .setOffset(20)
                            .setElementStride(stride2W),
                        nvrhi::VertexAttributeDesc()
                            .setName("BINORMAL")
                            .setFormat(nvrhi::Format::BGRA8_UNORM)
                            .setBufferIndex(0)
                            .setOffset(24)
                            .setElementStride(stride2W),
                        nvrhi::VertexAttributeDesc()
                            .setName("TEXCOORD")
                            .setFormat(nvrhi::Format::RGBA32_FLOAT)  // FLOAT4 for UV + bone indices
                            .setBufferIndex(0)
                            .setOffset(28)
                            .setElementStride(stride2W),
                    };
                    s_skinnedHQ2WInputLayout = nvDevice->createInputLayout(attribs2W, 5, s_skinnedHQ2WVS);

                    nvrhi::GraphicsPipelineDesc pipeDesc2W;
                    pipeDesc2W.VS = s_skinnedHQ2WVS;
                    pipeDesc2W.PS = s_skinnedPS;
                    pipeDesc2W.inputLayout = s_skinnedHQ2WInputLayout;

                    if (bindlessLayout) {
                        pipeDesc2W.bindingLayouts = { s_skinnedLayout, bindlessLayout };
                    }
                    else {
                        pipeDesc2W.bindingLayouts = { s_skinnedLayout };
                    }

                    pipeDesc2W.primType = nvrhi::PrimitiveType::TriangleList;
                    pipeDesc2W.renderState.depthStencilState.depthTestEnable = true;
                    pipeDesc2W.renderState.depthStencilState.depthWriteEnable = true;
                    pipeDesc2W.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::LessOrEqual;
                    pipeDesc2W.renderState.rasterState.frontCounterClockwise = false;
                    pipeDesc2W.renderState.rasterState.cullMode = nvrhi::RasterCullMode::Back;

                    s_skinnedHQ2WPipeline = nvDevice->createGraphicsPipeline(pipeDesc2W, skinnedFramebuffer);
                    if (!s_skinnedHQ2WPipeline) {
                        Msg("! [SkinnedHQ2W] Failed to create pipeline");
                    }
                    else {
                        Msg("* [SkinnedHQ2W] Initialized 44-byte 2W_HQ pipeline");
                    }
                }
                s_skinnedHQ2WInitialized = true;
            }
        }
    }


    // Check if skinned pipelines are initialized
    if (!s_skinnedPipeline && !s_skinnedHQ1WPipeline && !s_skinnedHQ4WPipeline && !s_skinnedHQ2WPipeline)
        return;

    // Create dynamic_transforms buffer (b0) - contains m_W, m_VP for skeleton
    nvrhi::BufferDesc dynTransCbDesc;
    dynTransCbDesc.byteSize = sizeof(DynamicTransforms);  // ~224 bytes
    dynTransCbDesc.isConstantBuffer = true;
    dynTransCbDesc.isVolatile = true;
    dynTransCbDesc.maxVersions = 128;  // Many skeleton draws per frame
    auto dynTransformsCB = nvDevice->createBuffer(dynTransCbDesc);

    // Create bone matrix constant buffer (b3) - 78 bones * float3x4 = 3744 bytes
    constexpr u32 MAX_BONES = 78;
    constexpr u32 BONE_CB_SIZE = MAX_BONES * sizeof(float) * 12;  // float3x4 per bone

    nvrhi::BufferDesc boneCbDesc;
    boneCbDesc.byteSize = BONE_CB_SIZE;
    boneCbDesc.isConstantBuffer = true;
    boneCbDesc.isVolatile = true;
    boneCbDesc.maxVersions = 128;
    auto bonesCB = nvDevice->createBuffer(boneCbDesc);

    // Create material ID constant buffer (b4) - 16 bytes for alignment
    nvrhi::BufferDesc matIdCbDesc;
    matIdCbDesc.byteSize = 16;  // uint materialID + 3 padding
    matIdCbDesc.isConstantBuffer = true;
    matIdCbDesc.isVolatile = true;
    matIdCbDesc.maxVersions = 128;
    auto matIdCB = nvDevice->createBuffer(matIdCbDesc);

    // Bone matrices as float3x4 (3 rows of float4)
    // Declare struct outside loop for reuse
    struct Float3x4 {
        float m[3][4];
    };

    // Pre-allocate full bone array (78 bones) - initialized to identity
    xr_vector<Float3x4> boneData(MAX_BONES);
    for (u32 i = 0; i < MAX_BONES; i++) {
        // Identity matrix
        boneData[i].m[0][0] = 1.0f; boneData[i].m[0][1] = 0.0f; boneData[i].m[0][2] = 0.0f; boneData[i].m[0][3] = 0.0f;
        boneData[i].m[1][0] = 0.0f; boneData[i].m[1][1] = 1.0f; boneData[i].m[1][2] = 0.0f; boneData[i].m[1][3] = 0.0f;
        boneData[i].m[2][0] = 0.0f; boneData[i].m[2][1] = 0.0f; boneData[i].m[2][2] = 1.0f; boneData[i].m[2][3] = 0.0f;
    }

    // Add bindless descriptor table for ResourceDescriptorHeap access
    nvrhi::IDescriptorTable* bindlessTable = nullptr;
    if (backend) {
        bindlessTable = backend->GetBindlessDescriptorTable();
    }

    // Render each skinned batch with its own bone matrices
    for (const auto& batch : batches) {
        if (!batch.isSkinned)
            continue;

        if (!batch.vertexBuffer || !batch.indexBuffer)
            continue;

        // Select appropriate pipeline based on vertex stride
        // - stride 24: non-HQ (SHORT4 position, SHORT2 UV)
        // - stride 36: 1W_HQ (FLOAT4 position, bone index in normal.w)
        // - stride 40: 4W_HQ (FLOAT4 position, BLENDINDICES)
        // - stride 44: 2W_HQ (FLOAT4 position, FLOAT4 TEXCOORD with bone indices)
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

        // Log pipeline selection (once per stride)
        static u32 s_lastLoggedStride = 0;
        if (batch.vertexStride != s_lastLoggedStride) {
            Msg("* [SkinnedRender] Using %s pipeline for stride %u, VB=%p IB=%p",
                pipelineName, batch.vertexStride,
                batch.vertexBuffer.Get(), batch.indexBuffer.Get());
            s_lastLoggedStride = batch.vertexStride;
        }

        // Set up graphics state for this batch
        nvrhi::GraphicsState skinnedState;
        skinnedState.pipeline = selectedPipeline;
        skinnedState.framebuffer = framebuffer;
        skinnedState.viewport.addViewport(viewport);
        skinnedState.viewport.addScissorRect(nvrhi::Rect(rtDesc.width, rtDesc.height));

        // Get parent skeleton for bone data
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

        // Calculate bones
        Parent->CalculateBones(TRUE);

        // Prepare dynamic_transforms (b0) with skeleton's world matrix
        DynamicTransforms dynTransData = {};
        FillDynamicTransforms(dynTransData, batch.worldMatrix);

        // Build bone matrix array (float3x4 format for shader)
        u32 boneCount = Parent->LL_BoneCount();
        u32 bonesToFill = std::min(boneCount, MAX_BONES);

        // Debug: Log bone count once
        static bool s_loggedBoneCount = false;
        if (!s_loggedBoneCount) {
            Msg("* [SkinnedRender] Skeleton has %u bones, filling %u into buffer of %u",
                boneCount, bonesToFill, MAX_BONES);
            s_loggedBoneCount = true;
        }

        // Fill bone data into pre-allocated array
        for (u32 i = 0; i < bonesToFill; i++) {
            const Fmatrix& bone = Parent->LL_GetTransform_R(u16(i));
            // Convert Fmatrix (column-major) to float3x4 row-major for shader
            boneData[i].m[0][0] = bone._11; boneData[i].m[0][1] = bone._21; boneData[i].m[0][2] = bone._31; boneData[i].m[0][3] = bone._41;
            boneData[i].m[1][0] = bone._12; boneData[i].m[1][1] = bone._22; boneData[i].m[1][2] = bone._32; boneData[i].m[1][3] = bone._42;
            boneData[i].m[2][0] = bone._13; boneData[i].m[2][1] = bone._23; boneData[i].m[2][2] = bone._33; boneData[i].m[2][3] = bone._43;
        }

        // Upload dynamic_transforms (b0) with skeleton's world matrix
        cmdList->writeBuffer(dynTransformsCB, &dynTransData, sizeof(dynTransData));

        // Upload ALL bone matrices (b3)
        cmdList->writeBuffer(bonesCB, boneData.data(), MAX_BONES * sizeof(Float3x4));

        // Upload material ID (b4)
        struct SkinnedMaterialCB {
            u32 materialID;
            u32 pad0, pad1, pad2;
        } matIdData;
        matIdData.materialID = batch.bindlessMaterialID;
        matIdData.pad0 = matIdData.pad1 = matIdData.pad2 = 0;
        cmdList->writeBuffer(matIdCB, &matIdData, sizeof(matIdData));

        // Create binding set AFTER writing buffer data
        nvrhi::BindingSetDesc skinnedBindDesc;
        skinnedBindDesc.bindings = {
            nvrhi::BindingSetItem::ConstantBuffer(0, dynTransformsCB),
            nvrhi::BindingSetItem::ConstantBuffer(2, staticGlobalsCB),
            nvrhi::BindingSetItem::ConstantBuffer(3, bonesCB),
            nvrhi::BindingSetItem::ConstantBuffer(4, matIdCB),
            nvrhi::BindingSetItem::StructuredBuffer_SRV(8, matBuffer.GetBuffer()),
            nvrhi::BindingSetItem::Sampler(0, s_linearSampler),
        };
        auto skinnedBindingSet = nvDevice->createBindingSet(skinnedBindDesc, s_skinnedLayout);

        // Set up bindings for this draw
        skinnedState.bindings = { skinnedBindingSet };
        if (bindlessTable) {
            skinnedState.addBindingSet(bindlessTable);
        }

        // Set vertex/index buffers for this skeleton mesh
        skinnedState.vertexBuffers = { {batch.vertexBuffer, 0, 0} };
        skinnedState.indexBuffer = { batch.indexBuffer, nvrhi::Format::R16_UINT, 0 };

        cmdList->setGraphicsState(skinnedState);

        // Draw
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

    // PassData structure to hold data between setup and execute
    struct ForwardColorPassData {
        VirtualResourceHandle depth;
        VirtualResourceHandle color;              // Single HDR color buffer (RGBA16_FLOAT)
        VirtualResourceHandle drawArgsBuffer;     // GPU culling draw args (optional)
        ng::RenderDevice* device;
        const GeometryCollector* geometry;
        MaterialCache* materialCache;
        DefaultOutputLayout outputs;
        u32 width;
        u32 height;
        BindlessForwardConfig bindlessConfig;     // Bindless rendering configuration
    };

    auto& passData = fg.addCallbackPass<ForwardColorPassData>(
        "Forward+ Color Pass",

        // ═══════════════════════════════════════════════════════
        //  SETUP LAMBDA (Declares resource usage)
        // ═══════════════════════════════════════════════════════
        [&, width, height, colorInput, drawArgsInput, bindlessConfig](FrameGraph& builder, PassHandle passHandle, ForwardColorPassData& data) {
            // Store pass configuration
            data.width = width;
            data.height = height;
            data.device = device;
            data.geometry = geometry;
            data.materialCache = materialCache;
            data.bindlessConfig = bindlessConfig;

            // Declare resource usage
            RenderPassBuilder passBuilder(builder, passHandle);

            // Depth buffer from prepass (READ-WRITE for early-Z)
            data.depth = passBuilder.readWrite(depthInput, ResourceState::DepthStencilWrite);

            // Color buffer from sky pass (READ-WRITE to preserve sky background)
            // CRITICAL: Using readWrite() creates a dependency on SkyPass!
            // This ensures sky renders first (as background), then forward geometry on top.
            data.color = passBuilder.readWrite(colorInput, ResourceState::RenderTarget);

            // Draw args buffer from GPU culling (READ for indirect draw)
            // CRITICAL: This creates proper dependency on GPU culling pass!
            // Without this, culling and forward passes can race, causing "exploding geometry"
            if (drawArgsInput.is_valid()) {
                data.drawArgsBuffer = passBuilder.read(drawArgsInput, ResourceState::IndirectArgument);
            }

            // Store outputs in PassData for MaterialCache
            // Forward+ uses single RT output (all shaders converted to f_forward)
            data.outputs.albedo = data.color;    // Single HDR color output
            data.outputs.depth = data.depth;
        },

        // ═══════════════════════════════════════════════════════
        //  EXECUTE LAMBDA (Renders geometry)
        // ═══════════════════════════════════════════════════════
        [](const ForwardColorPassData& data,
            const FrameGraph& fg,
            ng::RenderContext* ctx) {

            // Get physical resources from virtual handles
            auto* depthRT = fg.GetPhysicalTexture(data.depth);
            auto* colorRT = fg.GetPhysicalTexture(data.color);

            if (!depthRT || !colorRT)
                return;

            // Check if we have geometry to render
            if (!data.geometry)
                return;

            // Get draw args buffer through framegraph (proper dependency tracking)
            // This ensures state transition UAV -> IndirectArgument happened
            nvrhi::IBuffer* drawArgsBuffer = nullptr;
            if (data.drawArgsBuffer.is_valid()) {
                drawArgsBuffer = fg.GetPhysicalBuffer(data.drawArgsBuffer);
            }

            // ═══════════════════════════════════════════════════════
            //  BINDLESS RENDERING PATH (GPU-DRIVEN MULTI-DRAW)
            // ═══════════════════════════════════════════════════════
            // Render static geometry with GPU-driven multi-draw
            renderBindlessForward(
                ctx,
                data.device,
                data.geometry,
                colorRT,
                depthRT,
                data.bindlessConfig,
                data.materialCache
            );

            // ═══════════════════════════════════════════════════════
            //  SKINNED MESH RENDERING (Per-draw bone matrices)
            // ═══════════════════════════════════════════════════════
            // Render skeleton meshes separately with bone matrices
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

    // Return the forward color outputs
    // All outputs point to the same color buffer for legacy shader compatibility
    DefaultOutputLayout outputs;
    outputs.albedo = passData.color;      // Primary color output
    outputs.normal = passData.color;      // Same buffer (legacy compatibility)
    outputs.material = passData.color;    // Same buffer (legacy compatibility)
    outputs.depth = passData.depth;
    return outputs;
}
} // namespace xray::render::RENDER_NAMESPACE::passes
