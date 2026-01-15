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
#include "Layers/xrRender/FTreeVisual.h"
#include "Layers/xrRender/ConstantSystem/FGConstantSystem.h"
#include "Layers/xrRender/GPUCullingManager.h"  // For IndirectDrawArgs struct
#include "Layers/xrRender/Backend/D3D12Backend.h"  // For SM6 bindless descriptor heap
#include "Layers/xrRender/Bindless/MaterialBuffer.h"
#include "Layers/xrRender/Bindless/TerrainMaterialBuffer.h"  // For terrain rendering
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

// Terrain rendering (4-layer detail blending)
static nvrhi::GraphicsPipelineHandle s_terrainPipeline;
static nvrhi::BindingLayoutHandle s_terrainLayout;
static nvrhi::ShaderHandle s_terrainPS;
static bool s_terrainInitialized = false;

// ═══════════════════════════════════════════════════════════════════════════
//  FORWARD DECLARATIONS
// ═══════════════════════════════════════════════════════════════════════════
static void InitializeBindlessPipeline(ng::RenderDevice* device, nvrhi::IFramebuffer* framebuffer);

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
    // Forward pass uses: RGBA16_FLOAT color, D32_FLOAT depth
    // IMPORTANT: Must match framegraph's actual depth format (53 = D32_FLOAT, not D24S8)
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
    depthDesc.format = nvrhi::Format::D32;  // Changed from D24S8 to match framegraph
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

    // Initialize bindless pipeline (skinned pipelines moved to SkinningPassSetup)
    InitializeBindlessPipeline(device, dummyFramebuffer);

    Msg("* [ForwardPass] Pipeline initialization complete");
}

void ShutdownForwardPipelines()
{
    // Release bindless pipeline resources (skinned pipelines moved to SkinningPassSetup)
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

    // CRITICAL FIX: Query binding layout from pipeline
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

    // Finalize any pending materials (register textures to bindless descriptor heap)
    if (materialCache) {
        materialCache->FinalizePendingMaterials(ctx);
        materialCache->FinalizePendingTerrainMaterials(ctx);  // Terrain materials (4-layer detail blending)
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

        if (!s_bindlessLayout)
            return;

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

        // CRITICAL FIX: Query binding layout from pipeline
        const nvrhi::GraphicsPipelineDesc& actualDesc = s_bindlessPipeline->getDesc();
        if (!actualDesc.bindingLayouts.empty()) {
            s_bindlessLayout = actualDesc.bindingLayouts[0];
            Msg("* [FGDetailManager] Updated bindless forward bindingLayout from pipeline (pointer: %p)", s_bindlessLayout.Get());
        }

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
    state.indirectParams = config.compactDrawArgsBuffer;
    state.indirectCountBuffer = config.compactCountBuffer;
    state.indirectCountOffset = 0;
    state.viewport.addViewport(viewport);
    state.viewport.addScissorRect(nvrhi::Rect(rtDesc.width, rtDesc.height));

    cmdList->setGraphicsState(state);
    cmdList->drawIndexedIndirectCount(0, config.totalObjectCount);

    s_gpuDrivenDraws += config.totalObjectCount;

    // ═══════════════════════════════════════════════════════
    //  TERRAIN RENDERING (4-layer detail blending)
    // ═══════════════════════════════════════════════════════
    // Terrain uses separate pipeline and TerrainMaterialBuffer
    if (config.HasTerrain() && config.UseMegaBuffers()) {
        R_ASSERT2(config.UseTerrainCompaction(), "Terrain compaction buffers missing");

        // Lazy init terrain pipeline
        if (!s_terrainInitialized) {
            auto* shaderLoader = GEnv.Render->GetShaderLoader();
            if (shaderLoader) {
                auto terrainPsResult = shaderLoader->LoadPixelShader("bindless_terrain", "main");
                if (terrainPsResult.handle) {
                    s_terrainPS = terrainPsResult.handle;

                    // Create terrain binding layout (adds t9 for TerrainMaterialBuffer)
                    nvrhi::BindingLayoutDesc terrainLayoutDesc;
                    terrainLayoutDesc.visibility = nvrhi::ShaderType::All;
                    terrainLayoutDesc.bindings = {
                        nvrhi::BindingLayoutItem::VolatileConstantBuffer(2),  // static_globals (b2)
                        nvrhi::BindingLayoutItem::VolatileConstantBuffer(4),  // LightingConstants (b4)
                        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(8),    // g_Materials (unused but keep for layout compat)
                        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(9),    // g_TerrainMaterials
                        nvrhi::BindingLayoutItem::Sampler(0),
                        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(14),   // g_InstanceData
                        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(15),   // g_CompactBatchIndices
                        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(16),   // g_CompactMaterialIDs (terrain material IDs)
                    };
                    s_terrainLayout = nvDevice->createBindingLayout(terrainLayoutDesc);

                    if (s_terrainLayout && s_bindlessVS) {
                        // Create terrain pipeline (same VS as regular, different PS)
                        nvrhi::GraphicsPipelineDesc terrainPipeDesc;
                        terrainPipeDesc.VS = s_bindlessVS;  // Reuse forward VS
                        terrainPipeDesc.PS = s_terrainPS;
                        terrainPipeDesc.inputLayout = s_bindlessInputLayout;

                        auto* backendDevice = device->GetBackend();
                        nvrhi::IBindingLayout* bindlessLayout = backendDevice ? backendDevice->GetBindlessLayout() : nullptr;
                        if (bindlessLayout) {
                            terrainPipeDesc.bindingLayouts = { s_terrainLayout, bindlessLayout };
                            Msg("* [TerrainForward] Pipeline created WITH bindless layout");
                        } else {
                            terrainPipeDesc.bindingLayouts = { s_terrainLayout };
                            Msg("! [TerrainForward] Pipeline created WITHOUT bindless layout - textures will be black!");
                        }

                        terrainPipeDesc.primType = nvrhi::PrimitiveType::TriangleList;
                        terrainPipeDesc.renderState.depthStencilState.depthTestEnable = true;
                        terrainPipeDesc.renderState.depthStencilState.depthWriteEnable = true;
                        terrainPipeDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::LessOrEqual;
                        terrainPipeDesc.renderState.rasterState.frontCounterClockwise = false;
                        terrainPipeDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::Back;

                        s_terrainPipeline = nvDevice->createGraphicsPipeline(terrainPipeDesc, framebuffer);
                        if (s_terrainPipeline) {
                            s_terrainInitialized = true;
                            Msg("* [BindlessForward] Terrain pipeline initialized");
                        }
                    }
                }
            }
        }

        // Render terrain if pipeline is ready
        R_ASSERT2(s_terrainInitialized && s_terrainPipeline, "Terrain pipeline not initialized");
        if (s_terrainInitialized && s_terrainPipeline) {
            // Upload terrain materials
            auto& terrainMatBuffer = bindless::TerrainMaterialBuffer::Instance();
            terrainMatBuffer.Upload(ctx);

            // Validate all required terrain-specific buffers are available
            R_ASSERT2(terrainMatBuffer.GetBuffer() && config.terrainInstanceBuffer &&
                          config.terrainCompactBatchIndicesBuffer && config.terrainCompactMaterialIDBuffer &&
                          config.terrainCompactDrawArgsBuffer && config.terrainCompactCountBuffer,
                "Terrain buffers not ready for compaction rendering");

            // Begin tracking terrain buffer states for this command list
            cmdList->beginTrackingBufferState(config.terrainInstanceBuffer, nvrhi::ResourceStates::ShaderResource);
            cmdList->beginTrackingBufferState(config.terrainCompactBatchIndicesBuffer, nvrhi::ResourceStates::ShaderResource);
            cmdList->beginTrackingBufferState(config.terrainCompactMaterialIDBuffer, nvrhi::ResourceStates::ShaderResource);
            cmdList->beginTrackingBufferState(config.terrainCompactDrawArgsBuffer, nvrhi::ResourceStates::IndirectArgument);
            cmdList->beginTrackingBufferState(config.terrainCompactCountBuffer, nvrhi::ResourceStates::IndirectArgument);

            // Create terrain binding set (includes TerrainMaterialBuffer at t9)
            // NOTE: Terrain uses its own instance/batch buffers, not the regular ones
            nvrhi::BindingSetDesc terrainBindDesc;
            terrainBindDesc.bindings = {
                nvrhi::BindingSetItem::ConstantBuffer(2, staticGlobalsCB),   // b2 - static_globals
                nvrhi::BindingSetItem::ConstantBuffer(4, lightingCB),        // b4 - lighting
                nvrhi::BindingSetItem::StructuredBuffer_SRV(8, matBuffer.GetBuffer()),  // t8 - regular materials (unused)
                nvrhi::BindingSetItem::StructuredBuffer_SRV(9, terrainMatBuffer.GetBuffer()),  // t9 - terrain materials
                nvrhi::BindingSetItem::Sampler(0, s_linearSampler),
                nvrhi::BindingSetItem::StructuredBuffer_SRV(14, config.terrainInstanceBuffer),       // Terrain world transforms
                nvrhi::BindingSetItem::StructuredBuffer_SRV(15, config.terrainCompactBatchIndicesBuffer),   // Compact batch indices
                nvrhi::BindingSetItem::StructuredBuffer_SRV(16, config.terrainCompactMaterialIDBuffer),     // Compact material IDs
            };

            auto terrainBindingSet = nvDevice->createBindingSet(terrainBindDesc, s_terrainLayout);
            R_ASSERT2(terrainBindingSet, "Terrain binding set creation failed");

            // Set up terrain graphics state
            nvrhi::GraphicsState terrainState;
            terrainState.pipeline = s_terrainPipeline;
            terrainState.framebuffer = framebuffer;
            terrainState.bindings = { terrainBindingSet };

            // Add bindless descriptor table
            if (backend) {
                auto* bindlessTable = backend->GetBindlessDescriptorTable();
                if (bindlessTable) {
                    terrainState.addBindingSet(bindlessTable);
                } else {
                    static bool s_warnOnce = false;
                    if (!s_warnOnce) {
                        Msg("! [TerrainForward] GetBindlessDescriptorTable() returned null!");
                        s_warnOnce = true;
                    }
                }
            } else {
                static bool s_warnOnce = false;
                if (!s_warnOnce) {
                    Msg("! [TerrainForward] backend is null - bindless textures won't work!");
                    s_warnOnce = true;
                }
            }

            terrainState.vertexBuffers = {
                {config.megaVertexBuffer, 0, 0},
                {s_drawIndexBuffer, 1, 0}
            };
            terrainState.indexBuffer = { config.megaIndexBuffer, nvrhi::Format::R32_UINT, 0 };
            terrainState.indirectParams = config.terrainCompactDrawArgsBuffer;
            terrainState.indirectCountBuffer = config.terrainCompactCountBuffer;
            terrainState.indirectCountOffset = 0;
            terrainState.viewport.addViewport(viewport);
            terrainState.viewport.addScissorRect(nvrhi::Rect(rtDesc.width, rtDesc.height));

            cmdList->setGraphicsState(terrainState);

            // Debug: Log terrain draw info once
            static bool s_terrainDrawLogged = false;
            if (!s_terrainDrawLogged) {
                Msg("* [TerrainDraw] Drawing %u terrain batches (compacted), megaVB=%p, megaIB=%p, drawArgs=%p, count=%p, matIDs=%p, inst=%p, batchIdx=%p",
                    config.terrainObjectCount, config.megaVertexBuffer, config.megaIndexBuffer,
                    config.terrainCompactDrawArgsBuffer, config.terrainCompactCountBuffer,
                    config.terrainCompactMaterialIDBuffer, config.terrainInstanceBuffer,
                    config.terrainCompactBatchIndicesBuffer);
                s_terrainDrawLogged = true;
            }

            // Draw terrain batches with single MDI call (count comes from compaction)
            cmdList->drawIndexedIndirectCount(0, config.terrainObjectCount);

            s_gpuDrivenDraws += config.terrainObjectCount;
        }
    }

    // Skinned meshes are rendered in the SkinningPass (see SkinningPassSetup.cpp)
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

            // Clear depth buffer (temporal Hi-Z: no depth prepass, so clear here)
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            if (cmdList) {
                cmdList->clearDepthStencilTexture(depthRT, nvrhi::AllSubresources, true, 1.0f, false, 0);
            }

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
            // NOTE: Skinned meshes are rendered in SkinningPass (see SkinningPassSetup.cpp)
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
