// xrRender/FrameGraphPasses/SkinningPassSetup.cpp
// Consolidated skinned mesh rendering pass with World and HUD phases
// Uses GPU-driven global bone buffer for efficient skinning
#include "stdafx.h"
#include "SkinningPassSetup.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/Geometry/GeometryBatch.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/SkeletonCustom.h"
#include "Layers/xrRender/FSkinned.h"
#include "Layers/xrRender/SkeletonX.h"
#include "Layers/xrRender/Backend/D3D12Backend.h"
#include "Layers/xrRender/Bindless/MaterialBuffer.h"
#include "Layers/xrRender/Bindless/TerrainMaterialBuffer.h"
#include "Layers/xrRender/Bindless/VariantTextureBuffer.h"
#include "Layers/xrRender/GPUCullingManager.h"
#include "Layers/xrRender/ShaderVariant/ShaderVariantRegistry.h"
#include "Layers/xrRender/ShaderVariant/VariantPSOCache.h"
#include "xrCore/FMesh.hpp"

extern ENGINE_API float psHUD_FOV;

namespace xray::render::RENDER_NAMESPACE::passes {
using namespace bindless;

// ═══════════════════════════════════════════════════════════════════════════
//  SKINNED PIPELINE INFRASTRUCTURE (shared by World and HUD phases)
// ═══════════════════════════════════════════════════════════════════════════

// Non-HQ: 24 bytes (SHORT4 position, SHORT2 UV)
static nvrhi::GraphicsPipelineHandle s_skinnedPipeline;
static nvrhi::InputLayoutHandle s_skinnedInputLayout;
static nvrhi::ShaderHandle s_skinnedVS;

// HQ 1W: 36 bytes (FLOAT4 position, bone index in normal.w)
static nvrhi::GraphicsPipelineHandle s_skinnedHQ1WPipeline;
static nvrhi::InputLayoutHandle s_skinnedHQ1WInputLayout;
static nvrhi::ShaderHandle s_skinnedHQ1WVS;

// HQ 4W: 40 bytes (FLOAT4 position, BLENDINDICES)
static nvrhi::GraphicsPipelineHandle s_skinnedHQ4WPipeline;
static nvrhi::InputLayoutHandle s_skinnedHQ4WInputLayout;
static nvrhi::ShaderHandle s_skinnedHQ4WVS;

// HQ 2W: 44 bytes (FLOAT4 position, FLOAT4 UV+indices) - lerp blend
static nvrhi::GraphicsPipelineHandle s_skinnedHQ2WPipeline;
static nvrhi::InputLayoutHandle s_skinnedHQ2WInputLayout;
static nvrhi::ShaderHandle s_skinnedHQ2WVS;

// HQ 3W: 44 bytes (same layout as 2W, different weight formula)
static nvrhi::GraphicsPipelineHandle s_skinnedHQ3WPipeline;
static nvrhi::InputLayoutHandle s_skinnedHQ3WInputLayout;
static nvrhi::ShaderHandle s_skinnedHQ3WVS;

// Shared resources
static nvrhi::BindingLayoutHandle s_skinnedLayout;
static nvrhi::ShaderHandle s_skinnedPS;
static nvrhi::SamplerHandle s_linearSampler;
static bool s_skinnedInitialized = false;

// ═══════════════════════════════════════════════════════════════════════════
//  HUD FOV ADJUSTMENT
// ═══════════════════════════════════════════════════════════════════════════
static Fmatrix ApplyHUDFOVAdjustment(const Fmatrix& worldMatrix)
{
    float fovScale = 1.0f / psHUD_FOV;
    Fmatrix viewMatrix = Device.mView;
    Fmatrix invView;
    invView.invert(viewMatrix);

    Fmatrix fovScaleMatrix;
    fovScaleMatrix.identity();
    fovScaleMatrix._11 = fovScale;
    fovScaleMatrix._22 = fovScale;
    fovScaleMatrix._33 = 1.0f;

    Fmatrix temp1, temp2, result;
    temp1.mul(viewMatrix, worldMatrix);
    temp2.mul(fovScaleMatrix, temp1);
    result.mul(invView, temp2);

    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
//  PIPELINE INITIALIZATION
// ═══════════════════════════════════════════════════════════════════════════

void InitializeSkinningPipelines(ng::RenderDevice* device)
{
    if (s_skinnedInitialized)
        return;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    if (!nvDevice)
        return;

    Msg("* [SkinningPass] Initializing skinned pipelines...");

    // Create dummy framebuffer for pipeline creation
    nvrhi::TextureDesc colorDesc;
    colorDesc.width = 64;
    colorDesc.height = 64;
    colorDesc.format = nvrhi::Format::RGBA16_FLOAT;
    colorDesc.isRenderTarget = true;
    colorDesc.initialState = nvrhi::ResourceStates::RenderTarget;
    colorDesc.keepInitialState = true;
    colorDesc.debugName = "SkinningInit_DummyColor";
    auto dummyColorRT = nvDevice->createTexture(colorDesc);

    nvrhi::TextureDesc depthDesc;
    depthDesc.width = 64;
    depthDesc.height = 64;
    depthDesc.format = nvrhi::Format::D32;  // Changed from D24S8 to match framegraph
    depthDesc.isRenderTarget = true;
    depthDesc.initialState = nvrhi::ResourceStates::DepthWrite;
    depthDesc.keepInitialState = true;
    depthDesc.debugName = "SkinningInit_DummyDepth";
    auto dummyDepthRT = nvDevice->createTexture(depthDesc);

    nvrhi::FramebufferDesc fbDesc;
    fbDesc.addColorAttachment(dummyColorRT);
    fbDesc.setDepthAttachment(dummyDepthRT);
    auto framebuffer = nvDevice->createFramebuffer(fbDesc);

    if (!framebuffer) {
        Msg("! [SkinningPass] Failed to create dummy framebuffer");
        return;
    }

    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    if (!shaderLoader)
        return;

    auto* backend = device->GetBackend();
    nvrhi::IBindingLayout* bindlessLayout = backend ? backend->GetBindlessLayout() : nullptr;

    // Create shared binding layout
    // GPU-driven skinning uses:
    //   b0: dynamic_transforms - legacy world matrix (for compatibility)
    //   b1: shader_params - alpha ref, detail params (from common.h)
    //   b2: static_globals - m_VP, lighting, etc.
    //   b4: SkinnedMaterialCB - per-draw material ID (from pixel shader)
    //   t3: g_BoneMatrices - global bone buffer (from GPUCullingManager)
    //   t4: g_SkinnedInstances - per-instance data (world, materialID, boneOffset)
    //   t8: g_Materials - bindless material data
    //   t9: g_TerrainMaterials - declared in bindless_common.h (must include even if unused)
    //   s0: linear sampler
    nvrhi::BindingLayoutDesc skinnedLayoutDesc;
    skinnedLayoutDesc.visibility = nvrhi::ShaderType::All;
    skinnedLayoutDesc.bindings = {
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(0),  // dynamic_transforms (b0)
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(1),  // shader_params (b1) - from common.h
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(2),  // static_globals (b2)
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(3),    // g_BoneMatrices (t3) - GLOBAL bone buffer
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(4),    // g_SkinnedInstances (t4) - per-instance data
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(4),  // SkinnedMaterialCB (b4) - per-draw material ID
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(8),    // g_Materials (t8)
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(9),    // g_TerrainMaterials (t9) - from bindless_common.h
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(10),   // g_VariantTextures (t10)
        nvrhi::BindingLayoutItem::Sampler(0),
    };
    s_skinnedLayout = nvDevice->createBindingLayout(skinnedLayoutDesc);

    // Load shared pixel shader
    auto skinnedPsResult = shaderLoader->LoadPixelShader("bindless_skinned", "main");
    if (!skinnedPsResult.handle) {
        Msg("! [SkinningPass] Failed to load pixel shader");
        return;
    }
    s_skinnedPS = skinnedPsResult.handle;

    // Create sampler
    nvrhi::SamplerDesc samplerDesc;
    samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Repeat);
    samplerDesc.setAllFilters(true);
    samplerDesc.setMaxAnisotropy(16.0f);
    s_linearSampler = nvDevice->createSampler(samplerDesc);

    // ═══════════════════════════════════════════════════════
    //  NON-HQ PIPELINE (24-byte format)
    // ═══════════════════════════════════════════════════════
    {
        auto vsResult = shaderLoader->LoadVertexShader("bindless_skinned", "main");
        if (vsResult.handle) {
            s_skinnedVS = vsResult.handle;

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
            Msg("* [SkinningPass] Non-HQ pipeline (24-byte): %s", s_skinnedPipeline ? "OK" : "FAILED");

            // CRITICAL FIX: Query binding layout from pipeline
            if (s_skinnedPipeline) {
                const nvrhi::GraphicsPipelineDesc& actualDesc = s_skinnedPipeline->getDesc();
                if (!actualDesc.bindingLayouts.empty()) {
                    s_skinnedLayout = actualDesc.bindingLayouts[0];
                }
            }
        }
    }

    // ═══════════════════════════════════════════════════════
    //  HQ 1W PIPELINE (36-byte format)
    // ═══════════════════════════════════════════════════════
    {
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
            Msg("* [SkinningPass] HQ 1W pipeline (36-byte): %s", s_skinnedHQ1WPipeline ? "OK" : "FAILED");

            // CRITICAL FIX: Query binding layout from pipeline
            if (s_skinnedHQ1WPipeline) {
                const nvrhi::GraphicsPipelineDesc& actualDesc = s_skinnedHQ1WPipeline->getDesc();
                if (!actualDesc.bindingLayouts.empty()) {
                    s_skinnedLayout = actualDesc.bindingLayouts[0];
                }
            }
        }
    }

    // ═══════════════════════════════════════════════════════
    //  HQ 4W PIPELINE (40-byte format)
    // ═══════════════════════════════════════════════════════
    {
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
            Msg("* [SkinningPass] HQ 4W pipeline (40-byte): %s", s_skinnedHQ4WPipeline ? "OK" : "FAILED");

            // CRITICAL FIX: Query binding layout from pipeline
            if (s_skinnedHQ4WPipeline) {
                const nvrhi::GraphicsPipelineDesc& actualDesc = s_skinnedHQ4WPipeline->getDesc();
                if (!actualDesc.bindingLayouts.empty()) {
                    s_skinnedLayout = actualDesc.bindingLayouts[0];
                }
            }
        }
    }

    // ═══════════════════════════════════════════════════════
    //  HQ 2W/3W PIPELINE (44-byte format)
    // ═══════════════════════════════════════════════════════
    {
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
            Msg("* [SkinningPass] HQ 2W pipeline (44-byte): %s", s_skinnedHQ2WPipeline ? "OK" : "FAILED");

            // CRITICAL FIX: Query binding layout from pipeline
            if (s_skinnedHQ2WPipeline) {
                const nvrhi::GraphicsPipelineDesc& actualDesc = s_skinnedHQ2WPipeline->getDesc();
                if (!actualDesc.bindingLayouts.empty()) {
                    s_skinnedLayout = actualDesc.bindingLayouts[0];
                }
            }
        }
    }

    // ═══════════════════════════════════════════════════════
    //  HQ 3W PIPELINE (44-byte format, different weight formula)
    // ═══════════════════════════════════════════════════════
    {
        auto vsResult = shaderLoader->LoadVertexShader("bindless_skinned_3w", "main");
        if (vsResult.handle) {
            s_skinnedHQ3WVS = vsResult.handle;

            constexpr u32 stride = 44;
            nvrhi::VertexAttributeDesc attribs[] = {
                nvrhi::VertexAttributeDesc().setName("POSITION").setFormat(nvrhi::Format::RGBA32_FLOAT).setOffset(0).setElementStride(stride),
                nvrhi::VertexAttributeDesc().setName("NORMAL").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(16).setElementStride(stride),
                nvrhi::VertexAttributeDesc().setName("TANGENT").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(20).setElementStride(stride),
                nvrhi::VertexAttributeDesc().setName("BINORMAL").setFormat(nvrhi::Format::BGRA8_UNORM).setOffset(24).setElementStride(stride),
                nvrhi::VertexAttributeDesc().setName("TEXCOORD").setFormat(nvrhi::Format::RGBA32_FLOAT).setOffset(28).setElementStride(stride),
            };
            s_skinnedHQ3WInputLayout = nvDevice->createInputLayout(attribs, 5, s_skinnedHQ3WVS);

            nvrhi::GraphicsPipelineDesc pipeDesc;
            pipeDesc.VS = s_skinnedHQ3WVS;
            pipeDesc.PS = s_skinnedPS;
            pipeDesc.inputLayout = s_skinnedHQ3WInputLayout;
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

            s_skinnedHQ3WPipeline = nvDevice->createGraphicsPipeline(pipeDesc, framebuffer);
            Msg("* [SkinningPass] HQ 3W pipeline (44-byte): %s", s_skinnedHQ3WPipeline ? "OK" : "FAILED");

            // CRITICAL FIX: Query binding layout from pipeline
            if (s_skinnedHQ3WPipeline) {
                const nvrhi::GraphicsPipelineDesc& actualDesc = s_skinnedHQ3WPipeline->getDesc();
                if (!actualDesc.bindingLayouts.empty()) {
                    s_skinnedLayout = actualDesc.bindingLayouts[0];
                }
            }
        }
    }

    // Note: Global bone buffer is now managed by GPUCullingManager
    // and initialized via CreateSkinnedCullingBuffers()

    s_skinnedInitialized = true;
    Msg("* [SkinningPass] Pipeline initialization complete");
}

void ShutdownSkinningPipelines()
{
    s_skinnedPipeline = nullptr;
    s_skinnedInputLayout = nullptr;
    s_skinnedVS = nullptr;

    s_skinnedHQ1WPipeline = nullptr;
    s_skinnedHQ1WInputLayout = nullptr;
    s_skinnedHQ1WVS = nullptr;

    s_skinnedHQ4WPipeline = nullptr;
    s_skinnedHQ4WInputLayout = nullptr;
    s_skinnedHQ4WVS = nullptr;

    s_skinnedHQ2WPipeline = nullptr;
    s_skinnedHQ2WInputLayout = nullptr;
    s_skinnedHQ2WVS = nullptr;

    s_skinnedHQ3WPipeline = nullptr;
    s_skinnedHQ3WInputLayout = nullptr;
    s_skinnedHQ3WVS = nullptr;

    s_skinnedLayout = nullptr;
    s_skinnedPS = nullptr;
    s_linearSampler = nullptr;
    s_skinnedInitialized = false;

    Msg("* [SkinningPass] Pipeline resources released");
}

// ═══════════════════════════════════════════════════════════════════════════
//  PIPELINE SELECTION HELPER
// ═══════════════════════════════════════════════════════════════════════════
// Render mode enum values from CSkeletonX (must match SkeletonX.h)
enum {
    RM_SKINNING_SOFT = 0,
    RM_SINGLE = 1,
    RM_SINGLE_HQ = 2,
    RM_SKINNING_1B = 3,
    RM_SKINNING_1B_HQ = 4,
    RM_SKINNING_2B = 5,
    RM_SKINNING_2B_HQ = 6,
    RM_SKINNING_3B = 7,
    RM_SKINNING_3B_HQ = 8,
    RM_SKINNING_4B = 9,
    RM_SKINNING_4B_HQ = 10
};

static nvrhi::IGraphicsPipeline* SelectSkinnedPipeline(u32 vertexStride, u16 renderMode)
{
    // Use renderMode to distinguish 2W vs 3W (both are 44 bytes)
    if (renderMode == RM_SKINNING_3B || renderMode == RM_SKINNING_3B_HQ) {
        return s_skinnedHQ3WPipeline.Get();
    }
    if (renderMode == RM_SKINNING_2B || renderMode == RM_SKINNING_2B_HQ) {
        return s_skinnedHQ2WPipeline.Get();
    }
    if (renderMode == RM_SKINNING_4B || renderMode == RM_SKINNING_4B_HQ) {
        return s_skinnedHQ4WPipeline.Get();
    }
    if (renderMode == RM_SKINNING_1B_HQ || renderMode == RM_SINGLE_HQ) {
        return s_skinnedHQ1WPipeline.Get();
    }
    if (renderMode == RM_SKINNING_1B || renderMode == RM_SINGLE) {
        return s_skinnedPipeline.Get();
    }

    // Fallback: use vertex stride
    if (vertexStride == 36)
        return s_skinnedHQ1WPipeline.Get();
    if (vertexStride == 40)
        return s_skinnedHQ4WPipeline.Get();
    if (vertexStride == 44)
        return s_skinnedHQ2WPipeline.Get();  // Default to 2W for 44-byte
    if (vertexStride == 24)
        return s_skinnedPipeline.Get();

    // Last resort fallback
    if (vertexStride >= 36)
        return s_skinnedHQ1WPipeline.Get();
    return s_skinnedPipeline.Get();
}

static u32 GetSkinnedVertexFormatID(u16 renderMode, u32 vertexStride)
{
    if (renderMode == RM_SKINNING_3B || renderMode == RM_SKINNING_3B_HQ) return VF_SKINNED_HQ3W;
    if (renderMode == RM_SKINNING_2B || renderMode == RM_SKINNING_2B_HQ) return VF_SKINNED_HQ2W;
    if (renderMode == RM_SKINNING_4B || renderMode == RM_SKINNING_4B_HQ) return VF_SKINNED_HQ4W;
    if (renderMode == RM_SKINNING_1B_HQ || renderMode == RM_SINGLE_HQ) return VF_SKINNED_HQ1W;
    if (renderMode == RM_SKINNING_1B || renderMode == RM_SINGLE) return VF_SKINNED_NONHQ;
    if (vertexStride == 36) return VF_SKINNED_HQ1W;
    if (vertexStride == 40) return VF_SKINNED_HQ4W;
    if (vertexStride == 44) return VF_SKINNED_HQ2W;
    return VF_SKINNED_NONHQ;
}

static nvrhi::IInputLayout* GetSkinnedInputLayout(u32 fmt)
{
    switch (fmt) {
    case VF_SKINNED_HQ1W: return s_skinnedHQ1WInputLayout.Get();
    case VF_SKINNED_HQ4W: return s_skinnedHQ4WInputLayout.Get();
    case VF_SKINNED_HQ2W: return s_skinnedHQ2WInputLayout.Get();
    case VF_SKINNED_HQ3W: return s_skinnedHQ3WInputLayout.Get();
    default: return s_skinnedInputLayout.Get();
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  SKELETON BONE OFFSET HELPER
// ═══════════════════════════════════════════════════════════════════════════
// Extracts parent skeleton from batch and uploads bones to global buffer.
// Returns offset into GPUCullingManager's global bone buffer.
static u32 GetSkeletonBoneOffset(
    nvrhi::ICommandList* cmdList,
    GPUCullingManager& gpuCullMgr,
    const GeometryBatch& batch)
{
    CKinematics* parent = nullptr;
    u32 visualType = batch.visual ? batch.visual->getType() : 0;

    if (visualType == MT_SKELETON_GEOMDEF_ST) {
        parent = static_cast<CSkeletonX_ST*>(batch.visual)->GetParent();
    } else if (visualType == MT_SKELETON_GEOMDEF_PM) {
        parent = static_cast<CSkeletonX_PM*>(batch.visual)->GetParent();
    }

    if (!parent)
        return 0;

    return gpuCullMgr.GetOrUploadSkeleton(cmdList, parent);
}

// ═══════════════════════════════════════════════════════════════════════════
//  RENDER SKINNED BATCH (GPU-Driven with global bone buffer)
// ═══════════════════════════════════════════════════════════════════════════
// Uses GPUCullingManager's global bone buffer (t3) with per-instance offset.
// Each draw uploads a single GPUSkinnedInstanceData to instance buffer (t4).
// This eliminates expensive per-draw bone matrix uploads (~3.7KB per draw).
static void RenderSkinnedBatch(
    nvrhi::ICommandList* cmdList,
    nvrhi::IDevice* nvDevice,
    nvrhi::IFramebuffer* framebuffer,
    nvrhi::IBuffer* dynTransformsCB,
    nvrhi::IBuffer* shaderParamsCB,
    nvrhi::IBuffer* staticGlobalsCB,
    nvrhi::IBuffer* materialIdCB,
    nvrhi::IBuffer* instanceSB,
    nvrhi::IBuffer* globalBoneBuffer,
    nvrhi::IBuffer* terrainMaterialsSB,
    nvrhi::IDescriptorTable* bindlessTable,
    nvrhi::IBindingLayout* bindlessLayout,
    const nvrhi::Viewport& viewport,
    const nvrhi::Rect& scissor,
    const GeometryBatch& batch,
    const Fmatrix& worldMatrix,
    u32 skeletonBoneOffset)
{
    using namespace RENDER_NAMESPACE;
    using namespace RENDER_NAMESPACE::bindless;

    if (!batch.vertexBuffer || !batch.indexBuffer)
        return;

    DynamicTransforms dynTransData = {};
    FillDynamicTransforms(dynTransData, worldMatrix);
    cmdList->writeBuffer(dynTransformsCB, &dynTransData, sizeof(dynTransData));

    GPUSkinnedInstanceData instData;
    instData.world.transpose(worldMatrix);
    instData.materialID = batch.bindlessMaterialID;
    instData.skeletonBoneOffset = skeletonBoneOffset;
    instData.batchIndex = 0;
    instData.flags = 0;
    cmdList->writeBuffer(instanceSB, &instData, sizeof(instData));

    SkinnedMaterialCB matIdData = {};
    matIdData.materialID = batch.bindlessMaterialID;
    cmdList->writeBuffer(materialIdCB, &matIdData, sizeof(matIdData));

    if (!globalBoneBuffer)
        return;

    auto& matBuffer = MaterialBuffer::Instance();
    nvrhi::BindingSetDesc bindDesc;
    bindDesc.bindings = {
        nvrhi::BindingSetItem::ConstantBuffer(0, dynTransformsCB),
        nvrhi::BindingSetItem::ConstantBuffer(1, shaderParamsCB),
        nvrhi::BindingSetItem::ConstantBuffer(2, staticGlobalsCB),
        nvrhi::BindingSetItem::StructuredBuffer_SRV(3, globalBoneBuffer),
        nvrhi::BindingSetItem::StructuredBuffer_SRV(4, instanceSB),
        nvrhi::BindingSetItem::ConstantBuffer(4, materialIdCB),
        nvrhi::BindingSetItem::StructuredBuffer_SRV(8, matBuffer.GetBuffer()),
        nvrhi::BindingSetItem::StructuredBuffer_SRV(9, terrainMaterialsSB),
        nvrhi::BindingSetItem::StructuredBuffer_SRV(10, bindless::VariantTextureBuffer::Instance().GetBuffer()),
        nvrhi::BindingSetItem::Sampler(0, s_linearSampler),
    };
    auto bindingSet = nvDevice->createBindingSet(bindDesc, s_skinnedLayout);
    if (!bindingSet)
        return;

    u32 variantIdx = MaterialBuffer::Instance().GetShaderVariant(batch.bindlessMaterialID);
    const ShaderVariantDesc* variant = nullptr;
    u32 passCount = 1;
    if (variantIdx > 0) {
        variant = ShaderVariantRegistry::Instance().GetVariantByIndex(variantIdx);
        if (variant)
            passCount = variant->GetPassCount();
    }

    for (u32 p = 0; p < passCount; p++) {
        nvrhi::IGraphicsPipeline* pipeline;
        if (variant) {
            u32 fmt = GetSkinnedVertexFormatID(batch.skinningRenderMode, batch.vertexStride);
            pipeline = VariantPSOCache::Instance().GetOrCreatePSO(
                nvDevice, framebuffer, variantIdx, *variant, p, fmt,
                GetSkinnedInputLayout(fmt), s_skinnedLayout, bindlessLayout);
        } else {
            pipeline = SelectSkinnedPipeline(batch.vertexStride, batch.skinningRenderMode);
        }
        if (!pipeline)
            continue;

        nvrhi::GraphicsState state;
        state.pipeline = pipeline;
        state.framebuffer = framebuffer;
        state.bindings = { bindingSet };
        if (bindlessTable)
            state.addBindingSet(bindlessTable);
        state.vertexBuffers = { {batch.vertexBuffer, 0, 0} };
        state.indexBuffer = { batch.indexBuffer, nvrhi::Format::R16_UINT, 0 };
        state.viewport.addViewport(viewport);
        state.viewport.addScissorRect(scissor);

        cmdList->setGraphicsState(state);
        cmdList->drawIndexed(
            nvrhi::DrawArguments()
                .setVertexCount(batch.indexCount)
                .setStartIndexLocation(batch.startIndex)
                .setStartVertexLocation(batch.baseVertex));
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  SKINNING PASS SETUP
// ═══════════════════════════════════════════════════════════════════════════

framegraph::DefaultOutputLayout setupSkinningPass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    const framegraph::DefaultOutputLayout& inputs,
    const GeometryCollector* geometry,
    const xr_vector<GeometryBatch>* hudBatches,
    MaterialCache* materialCache,
    u32 width,
    u32 height,
    const SkinnedVisibilityData& visibilityData)
{
    using namespace framegraph;

    struct SkinningPassData {
        VirtualResourceHandle color;
        VirtualResourceHandle depth;
        ng::RenderDevice* device;
        const GeometryCollector* geometry;
        const xr_vector<GeometryBatch>* hudBatches;
        MaterialCache* materialCache;
        u32 width, height;
        DefaultOutputLayout outputs;
        // GPU culling visibility data
        SkinnedVisibilityData visibilityData;
    };

    auto& passData = fg.addCallbackPass<SkinningPassData>(
        "Skinning Pass",

        // ═══════════════════════════════════════════════════════
        //  SETUP LAMBDA
        // ═══════════════════════════════════════════════════════
        [&, width, height, visibilityData](FrameGraph& builder, PassHandle passHandle, SkinningPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.width = width;
            data.height = height;
            data.device = device;
            data.geometry = geometry;
            data.hudBatches = hudBatches;
            data.materialCache = materialCache;
            data.visibilityData = visibilityData;

            // Read-write color and depth (renders on top of forward pass)
            data.color = passBuilder.readWrite(inputs.albedo, ResourceState::RenderTarget);
            data.depth = passBuilder.readWrite(inputs.depth, ResourceState::DepthStencilWrite);

            data.outputs.albedo = data.color;
            data.outputs.depth = data.depth;
        },

        // ═══════════════════════════════════════════════════════
        //  EXECUTE LAMBDA
        // ═══════════════════════════════════════════════════════
        [](const SkinningPassData& data, const FrameGraph& fg, ng::RenderContext* ctx) {
            using namespace RENDER_NAMESPACE;

            // Check if any skinned batches to render
            bool hasWorldSkinned = data.geometry && !data.geometry->GetBatches().empty();
            bool hasHUDSkinned = data.hudBatches && !data.hudBatches->empty();

            // Count actual skinned batches in world geometry
            u32 worldSkinnedCount = 0;
            if (hasWorldSkinned) {
                for (const auto& batch : data.geometry->GetBatches()) {
                    if (batch.isSkinned) worldSkinnedCount++;
                }
            }

            if (worldSkinnedCount == 0 && !hasHUDSkinned) {
                // Msg("! [SkinningPass] No skinned batches to render");
                return;
            }

            // Ensure pipelines are initialized
            if (!s_skinnedInitialized) {
                Msg("! [SkinningPass] Pipelines not initialized!");
                return;
            }

            // Get physical resources
            auto* colorRT = fg.GetPhysicalTexture(data.color);
            auto* depthRT = fg.GetPhysicalTexture(data.depth);
            if (!colorRT || !depthRT)
                return;

            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            if (!nvDevice || !cmdList)
                return;

            // Create framebuffer
            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(colorRT);
            fbDesc.setDepthAttachment(depthRT);
            auto framebuffer = nvDevice->createFramebuffer(fbDesc);
            if (!framebuffer)
                return;

            const auto& rtDesc = colorRT->getDesc();

            // Get GPUCullingManager for bone buffer (passed via visibility data userData)
            GPUCullingManager* gpuCullMgr = data.visibilityData.visibilityByVisualUserData
                ? static_cast<GPUCullingManager*>(data.visibilityData.visibilityByVisualUserData)
                : nullptr;

            if (!gpuCullMgr || !gpuCullMgr->GetGlobalBoneBuffer()) {
                Msg("! [SkinningPass] GPUCullingManager bone buffer not available!");
                return;
            }

            // Begin frame - reset skeleton allocations
            gpuCullMgr->BeginSkinnedFrame();

            // Get global bone buffer for shader binding
            nvrhi::IBuffer* globalBoneBuffer = gpuCullMgr->GetGlobalBoneBuffer();

            // Create shared buffers for all skinned draws
            nvrhi::BufferDesc dynTransCbDesc;
            dynTransCbDesc.byteSize = sizeof(DynamicTransforms);
            dynTransCbDesc.isConstantBuffer = true;
            dynTransCbDesc.isVolatile = true;
            dynTransCbDesc.maxVersions = 128;
            auto dynTransformsCB = nvDevice->createBuffer(dynTransCbDesc);

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

            // Create per-instance data buffer (replaces per-draw bone + material ID buffers)
            // GPU-driven skinning uses this with SV_InstanceID = 0
            nvrhi::BufferDesc instanceSbDesc;
            instanceSbDesc.byteSize = sizeof(GPUSkinnedInstanceData);
            instanceSbDesc.structStride = sizeof(GPUSkinnedInstanceData);
            instanceSbDesc.initialState = nvrhi::ResourceStates::ShaderResource;
            instanceSbDesc.keepInitialState = true;
            instanceSbDesc.debugName = "SkinningPass_Instance_SB";
            auto instanceSB = nvDevice->createBuffer(instanceSbDesc);

            // Create shader_params buffer (b1) - required by common.h
            // Uses ShaderParams from ShaderConstants.h
            nvrhi::BufferDesc shaderParamsCbDesc;
            shaderParamsCbDesc.byteSize = sizeof(ShaderParams);
            shaderParamsCbDesc.isConstantBuffer = true;
            shaderParamsCbDesc.isVolatile = true;
            shaderParamsCbDesc.maxVersions = 128;
            auto shaderParamsCB = nvDevice->createBuffer(shaderParamsCbDesc);

            // Fill shader params with defaults
            ShaderParams shaderParams = {};
            shaderParams.m_AlphaRef = 0.5f;  // Default alpha ref
            shaderParams.dt_params.set(1.0f, 0.0f, 1.0f, 50.0f);  // dt_mul, dt_add, unused, detail distance
            cmdList->writeBuffer(shaderParamsCB, &shaderParams, sizeof(shaderParams));

            // Create material ID constant buffer (b4) - uses SkinnedMaterialCB from ShaderConstants.h
            nvrhi::BufferDesc materialIdCbDesc;
            materialIdCbDesc.byteSize = sizeof(SkinnedMaterialCB);
            materialIdCbDesc.isConstantBuffer = true;
            materialIdCbDesc.isVolatile = true;
            materialIdCbDesc.maxVersions = 256;  // Many skinned draws
            auto materialIdCB = nvDevice->createBuffer(materialIdCbDesc);

            // Get terrain material buffer (t9) - required by bindless_common.h
            auto& terrainMatBuffer = TerrainMaterialBuffer::Instance();
            if (!terrainMatBuffer.GetBuffer()) {
                Msg("! [SkinningPass] TerrainMaterialBuffer is NULL - cannot render skinned meshes");
                return;
            }

            // Finalize any pending materials (register textures to bindless descriptor heap)
            if (data.materialCache) {
                data.materialCache->FinalizePendingMaterials(ctx);
            }

            // Upload material buffer to GPU
            auto& matBuffer = MaterialBuffer::Instance();
            matBuffer.Upload(ctx);

            auto* backend = data.device->GetBackend();
            nvrhi::IDescriptorTable* bindlessTable = backend ? backend->GetBindlessDescriptorTable() : nullptr;
            nvrhi::IBindingLayout* bindlessLayout = backend ? backend->GetBindlessLayout() : nullptr;

            // Scissor rect (same for both phases)
            nvrhi::Rect scissor(rtDesc.width, rtDesc.height);

            // ═══════════════════════════════════════════════════════
            //  PHASE 1: WORLD SKINNED MESHES (depth [0.0, 1.0])
            // ═══════════════════════════════════════════════════════
            if (worldSkinnedCount > 0) {
                nvrhi::Viewport worldViewport(
                    0.0f, static_cast<float>(rtDesc.width),
                    0.0f, static_cast<float>(rtDesc.height),
                    0.0f, 1.0f  // Normal depth range
                );

                // Use GPU culling visibility if available (visual-based lookup)
                const bool useGPUCulling = data.visibilityData.enabled &&
                                           data.visibilityData.visibilityByVisualCallback != nullptr;

                if (useGPUCulling) {
                    // Render using visibility data (GPU-culled path)
                    // Visual-based lookup handles batch reordering correctly
                    // NOTE: We use data from N-2 frames ago (double-buffer readback delay)

                    u32 renderedCount = 0;
                    u32 culledCount = 0;

                    // Iterate over current frame's batches
                    for (const auto& batch : data.geometry->GetBatches()) {
                        if (!batch.isSkinned)
                            continue;

                        // Check visibility using visual pointer lookup
                        u32 visibilityValue = batch.visual
                            ? data.visibilityData.visibilityByVisualCallback(
                                batch.visual, data.visibilityData.visibilityByVisualUserData)
                            : 0;

                        if (visibilityValue == 0) {
                            culledCount++;
                            continue;
                        }

                        // Get bone offset from global buffer
                        u32 boneOffset = GetSkeletonBoneOffset(cmdList, *gpuCullMgr, batch);

                        RenderSkinnedBatch(
                            cmdList, nvDevice, framebuffer,
                            dynTransformsCB, shaderParamsCB, staticGlobalsCB, materialIdCB, instanceSB,
                            globalBoneBuffer, terrainMatBuffer.GetBuffer(),
                            bindlessTable, bindlessLayout, worldViewport, scissor,
                            batch, batch.worldMatrix, boneOffset
                        );
                        renderedCount++;
                    }

                    // Report stats via callback
                    if (data.visibilityData.statsCallback) {
                        data.visibilityData.statsCallback(renderedCount, culledCount,
                            data.visibilityData.statsUserData);
                    }
                } else {
                    // Render all skinned batches (no GPU culling)
                    u32 drawCount = 0;
                    for (const auto& batch : data.geometry->GetBatches()) {
                        if (!batch.isSkinned)
                            continue;

                        // Get bone offset from global buffer
                        u32 boneOffset = GetSkeletonBoneOffset(cmdList, *gpuCullMgr, batch);

                        RenderSkinnedBatch(
                            cmdList, nvDevice, framebuffer,
                            dynTransformsCB, shaderParamsCB, staticGlobalsCB, materialIdCB, instanceSB,
                            globalBoneBuffer, terrainMatBuffer.GetBuffer(),
                            bindlessTable, bindlessLayout, worldViewport, scissor,
                            batch, batch.worldMatrix, boneOffset
                        );
                        drawCount++;
                    }
                }
            }

            // ═══════════════════════════════════════════════════════
            //  PHASE 2: HUD SKINNED MESHES (depth [0.0, 0.1])
            // ═══════════════════════════════════════════════════════
            if (hasHUDSkinned) {
                nvrhi::Viewport hudViewport(
                    0.0f, static_cast<float>(rtDesc.width),
                    0.0f, static_cast<float>(rtDesc.height),
                    0.0f, 0.1f  // Compressed depth range for HUD
                );

                for (const auto& batch : *data.hudBatches) {
                    Fmatrix adjustedWorldMatrix = ApplyHUDFOVAdjustment(batch.worldMatrix);
                    u32 boneOffset = GetSkeletonBoneOffset(cmdList, *gpuCullMgr, batch);

                    RenderSkinnedBatch(
                        cmdList, nvDevice, framebuffer,
                        dynTransformsCB, shaderParamsCB, staticGlobalsCB, materialIdCB, instanceSB,
                        globalBoneBuffer, terrainMatBuffer.GetBuffer(),
                        bindlessTable, bindlessLayout, hudViewport, scissor,
                        batch, adjustedWorldMatrix, boneOffset
                    );
                }
            }
        }
    );

    // Return outputs
    DefaultOutputLayout outputs;
    outputs.albedo = passData.color;
    outputs.depth = passData.depth;
    return outputs;
}

} // namespace xray::render::RENDER_NAMESPACE::passes
