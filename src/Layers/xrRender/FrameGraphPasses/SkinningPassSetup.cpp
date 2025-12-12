// xrRender/FrameGraphPasses/SkinningPassSetup.cpp
// Consolidated skinned mesh rendering pass with World and HUD phases
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

// HQ 2W/3W: 44 bytes (FLOAT4 position, FLOAT4 UV+indices)
static nvrhi::GraphicsPipelineHandle s_skinnedHQ2WPipeline;
static nvrhi::InputLayoutHandle s_skinnedHQ2WInputLayout;
static nvrhi::ShaderHandle s_skinnedHQ2WVS;

// Shared resources
static nvrhi::BindingLayoutHandle s_skinnedLayout;
static nvrhi::ShaderHandle s_skinnedPS;
static nvrhi::SamplerHandle s_linearSampler;
static bool s_skinnedInitialized = false;

// Bone matrix format for StructuredBuffer (48 bytes per bone)
struct Float3x4 {
    float m[3][4];
};

constexpr u32 MAX_BONES = 78;
constexpr u32 BONE_STRIDE = sizeof(Float3x4);  // 48 bytes

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
    nvrhi::BindingLayoutDesc skinnedLayoutDesc;
    skinnedLayoutDesc.visibility = nvrhi::ShaderType::All;
    skinnedLayoutDesc.bindings = {
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(0),  // dynamic_transforms (b0)
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(2),  // static_globals (b2)
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(3),    // g_BoneMatrices (t3)
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(4),  // SkinnedMaterialCB (b4)
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(8),    // g_Materials
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

    s_skinnedLayout = nullptr;
    s_skinnedPS = nullptr;
    s_linearSampler = nullptr;
    s_skinnedInitialized = false;

    Msg("* [SkinningPass] Pipeline resources released");
}

// ═══════════════════════════════════════════════════════════════════════════
//  PIPELINE SELECTION HELPER
// ═══════════════════════════════════════════════════════════════════════════
static nvrhi::IGraphicsPipeline* SelectSkinnedPipeline(u32 vertexStride)
{
    if (vertexStride == 36)
        return s_skinnedHQ1WPipeline.Get();
    if (vertexStride == 40)
        return s_skinnedHQ4WPipeline.Get();
    if (vertexStride == 44)
        return s_skinnedHQ2WPipeline.Get();
    if (vertexStride == 24)
        return s_skinnedPipeline.Get();

    // Fallback for unknown strides
    if (vertexStride >= 36)
        return s_skinnedHQ1WPipeline.Get();
    return s_skinnedPipeline.Get();
}

// ═══════════════════════════════════════════════════════════════════════════
//  RENDER SKINNED BATCH (shared by World and HUD phases)
// ═══════════════════════════════════════════════════════════════════════════
static void RenderSkinnedBatch(
    nvrhi::ICommandList* cmdList,
    nvrhi::IDevice* nvDevice,
    nvrhi::IFramebuffer* framebuffer,
    nvrhi::IBuffer* dynTransformsCB,
    nvrhi::IBuffer* staticGlobalsCB,
    nvrhi::IBuffer* bonesSB,
    nvrhi::IBuffer* matIdCB,
    nvrhi::IDescriptorTable* bindlessTable,
    const nvrhi::Viewport& viewport,
    const nvrhi::Rect& scissor,
    const GeometryBatch& batch,
    const Fmatrix& worldMatrix,
    xr_vector<Float3x4>& boneData)
{
    using namespace RENDER_NAMESPACE;
    using namespace RENDER_NAMESPACE::bindless;

    if (!batch.vertexBuffer || !batch.indexBuffer)
        return;

    // Select pipeline based on vertex stride
    nvrhi::IGraphicsPipeline* pipeline = SelectSkinnedPipeline(batch.vertexStride);
    if (!pipeline)
        return;

    // Get parent skeleton
    CKinematics* Parent = nullptr;
    u32 visualType = batch.visual ? batch.visual->getType() : 0;

    if (visualType == MT_SKELETON_GEOMDEF_ST) {
        auto* skeletonMesh = static_cast<CSkeletonX_ST*>(batch.visual);
        Parent = skeletonMesh->GetParent();
    } else if (visualType == MT_SKELETON_GEOMDEF_PM) {
        auto* skeletonMesh = static_cast<CSkeletonX_PM*>(batch.visual);
        Parent = skeletonMesh->GetParent();
    }

    if (!Parent)
        return;

    // Calculate bones
    Parent->CalculateBones(TRUE);

    // Fill dynamic transforms with world matrix
    DynamicTransforms dynTransData = {};
    FillDynamicTransforms(dynTransData, worldMatrix);

    // Build bone matrix array (float3x4 format)
    u32 boneCount = Parent->LL_BoneCount();
    u32 bonesToFill = std::min(boneCount, MAX_BONES);

    for (u32 i = 0; i < bonesToFill; i++) {
        const Fmatrix& bone = Parent->LL_GetTransform_R(u16(i));
        // Convert Fmatrix (column-major) to float3x4 row-major for shader
        boneData[i].m[0][0] = bone._11; boneData[i].m[0][1] = bone._21; boneData[i].m[0][2] = bone._31; boneData[i].m[0][3] = bone._41;
        boneData[i].m[1][0] = bone._12; boneData[i].m[1][1] = bone._22; boneData[i].m[1][2] = bone._32; boneData[i].m[1][3] = bone._42;
        boneData[i].m[2][0] = bone._13; boneData[i].m[2][1] = bone._23; boneData[i].m[2][2] = bone._33; boneData[i].m[2][3] = bone._43;
    }

    // Upload buffers
    cmdList->writeBuffer(dynTransformsCB, &dynTransData, sizeof(dynTransData));
    cmdList->writeBuffer(bonesSB, boneData.data(), MAX_BONES * sizeof(Float3x4));

    // Upload material ID
    struct SkinnedMaterialCB {
        u32 materialID;
        u32 pad0, pad1, pad2;
    } matIdData;
    matIdData.materialID = batch.bindlessMaterialID;
    matIdData.pad0 = matIdData.pad1 = matIdData.pad2 = 0;
    cmdList->writeBuffer(matIdCB, &matIdData, sizeof(matIdData));

    // Create binding set
    auto& matBuffer = MaterialBuffer::Instance();
    nvrhi::BindingSetDesc bindDesc;
    bindDesc.bindings = {
        nvrhi::BindingSetItem::ConstantBuffer(0, dynTransformsCB),
        nvrhi::BindingSetItem::ConstantBuffer(2, staticGlobalsCB),
        nvrhi::BindingSetItem::StructuredBuffer_SRV(3, bonesSB),
        nvrhi::BindingSetItem::ConstantBuffer(4, matIdCB),
        nvrhi::BindingSetItem::StructuredBuffer_SRV(8, matBuffer.GetBuffer()),
        nvrhi::BindingSetItem::Sampler(0, s_linearSampler),
    };
    auto bindingSet = nvDevice->createBindingSet(bindDesc, s_skinnedLayout);

    // Set up graphics state
    nvrhi::GraphicsState state;
    state.pipeline = pipeline;
    state.framebuffer = framebuffer;
    state.bindings = { bindingSet };
    if (bindlessTable) {
        state.addBindingSet(bindlessTable);
    }
    state.vertexBuffers = { {batch.vertexBuffer, 0, 0} };
    state.indexBuffer = { batch.indexBuffer, nvrhi::Format::R16_UINT, 0 };
    state.viewport.addViewport(viewport);
    state.viewport.addScissorRect(scissor);

    cmdList->setGraphicsState(state);

    // Draw
    cmdList->drawIndexed(
        nvrhi::DrawArguments()
            .setVertexCount(batch.indexCount)
            .setStartIndexLocation(batch.startIndex)
            .setStartVertexLocation(batch.baseVertex)
    );
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
    u32 height)
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
    };

    auto& passData = fg.addCallbackPass<SkinningPassData>(
        "Skinning Pass",

        // ═══════════════════════════════════════════════════════
        //  SETUP LAMBDA
        // ═══════════════════════════════════════════════════════
        [&, width, height](FrameGraph& builder, PassHandle passHandle, SkinningPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.width = width;
            data.height = height;
            data.device = device;
            data.geometry = geometry;
            data.hudBatches = hudBatches;
            data.materialCache = materialCache;

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

            if (worldSkinnedCount == 0 && !hasHUDSkinned)
                return;

            // Ensure pipelines are initialized
            if (!s_skinnedInitialized)
                return;

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

            // Create bone structured buffer
            nvrhi::BufferDesc boneSbDesc;
            boneSbDesc.byteSize = MAX_BONES * BONE_STRIDE;
            boneSbDesc.structStride = BONE_STRIDE;
            boneSbDesc.initialState = nvrhi::ResourceStates::ShaderResource;
            boneSbDesc.keepInitialState = true;
            boneSbDesc.debugName = "SkinningPass_Bones_SB";
            auto bonesSB = nvDevice->createBuffer(boneSbDesc);

            // Create material ID buffer
            nvrhi::BufferDesc matIdCbDesc;
            matIdCbDesc.byteSize = 16;
            matIdCbDesc.isConstantBuffer = true;
            matIdCbDesc.isVolatile = true;
            matIdCbDesc.maxVersions = 128;
            auto matIdCB = nvDevice->createBuffer(matIdCbDesc);

            // Pre-allocate bone data array
            xr_vector<Float3x4> boneData(MAX_BONES);
            for (u32 i = 0; i < MAX_BONES; i++) {
                // Identity matrix
                boneData[i].m[0][0] = 1.0f; boneData[i].m[0][1] = 0.0f; boneData[i].m[0][2] = 0.0f; boneData[i].m[0][3] = 0.0f;
                boneData[i].m[1][0] = 0.0f; boneData[i].m[1][1] = 1.0f; boneData[i].m[1][2] = 0.0f; boneData[i].m[1][3] = 0.0f;
                boneData[i].m[2][0] = 0.0f; boneData[i].m[2][1] = 0.0f; boneData[i].m[2][2] = 1.0f; boneData[i].m[2][3] = 0.0f;
            }

            // Finalize any pending materials (register textures to bindless descriptor heap)
            if (data.materialCache) {
                data.materialCache->FinalizePendingMaterials(ctx);
            }

            // Upload material buffer to GPU
            auto& matBuffer = MaterialBuffer::Instance();
            matBuffer.Upload(ctx);

            // Get bindless descriptor table
            auto* backend = data.device->GetBackend();
            nvrhi::IDescriptorTable* bindlessTable = backend ? backend->GetBindlessDescriptorTable() : nullptr;

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

                for (const auto& batch : data.geometry->GetBatches()) {
                    if (!batch.isSkinned)
                        continue;

                    RenderSkinnedBatch(
                        cmdList, nvDevice, framebuffer,
                        dynTransformsCB, staticGlobalsCB, bonesSB, matIdCB,
                        bindlessTable, worldViewport, scissor,
                        batch, batch.worldMatrix, boneData
                    );
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
                    // Apply HUD FOV adjustment to world matrix
                    Fmatrix adjustedWorldMatrix = ApplyHUDFOVAdjustment(batch.worldMatrix);

                    RenderSkinnedBatch(
                        cmdList, nvDevice, framebuffer,
                        dynTransformsCB, staticGlobalsCB, bonesSB, matIdCB,
                        bindlessTable, hudViewport, scissor,
                        batch, adjustedWorldMatrix, boneData
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
