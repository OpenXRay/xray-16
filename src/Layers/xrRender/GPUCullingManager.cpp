// xrRender/GPUCullingManager.cpp
#include "stdafx.h"
#include "GPUCullingManager.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/Geometry/GeometryBatch.h"
#include "Layers/xrRender/xrRender_console.h"
#include "Layers/xrRender/FrameGraphPasses/ParticlePassSetup.h"
#include "Layers/xrRender/FBasicVisual.h"
#include "Layers/xrRender/Bindless/VertexConverter.h"

namespace RENDER_NAMESPACE
{
    extern CRender RImplementation;
}

namespace xray::render::RENDER_NAMESPACE {

// ═══════════════════════════════════════════════════════
//  CONSTANTS
// ═══════════════════════════════════════════════════════

constexpr u32 MAX_CULLING_OBJECTS = 65536;  // Maximum objects per frame
constexpr u32 CULL_THREAD_GROUP_SIZE = 64;  // Must match [numthreads] in shader

// ═══════════════════════════════════════════════════════
//  CONSTANT BUFFER STRUCTURE (must match HLSL)
// ═══════════════════════════════════════════════════════

struct CullParamsCB {
    Fmatrix viewProj;           // View-projection matrix
    Fvector cameraPos;          // Camera world position
    float maxDistanceSq;        // Maximum render distance (squared)
    Fvector4 frustumPlanes[6];  // View frustum planes
    u32 objectCount;            // Total objects to cull
    u32 hizWidth;               // Hi-Z pyramid width
    u32 hizHeight;              // Hi-Z pyramid height
    u32 hizMipLevels;           // Hi-Z mip levels
};

// ═══════════════════════════════════════════════════════
//  DEBUG CONSTANT BUFFER (must match HLSL)
// ═══════════════════════════════════════════════════════

struct CullDebugParamsCB {
    Fmatrix viewProj;
    Fvector cameraPos;
    float maxDistanceSq;
    Fvector4 frustumPlanes[6];
    u32 objectCount;
    u32 hizWidth;
    u32 hizHeight;
    u32 hizMipLevels;
    float occluderThreshold;
    u32 debugOffset;
    float padding[2];
};

struct CullDebugVSParamsCB {
    Fmatrix view;               // View matrix (for billboard orientation)
    Fmatrix viewProj;           // View-projection matrix
    u32 objectCount;            // Number of objects
    float wireframeAlpha;       // Wireframe transparency
    float padding[2];
};

// ═══════════════════════════════════════════════════════
//  STATIC STATE
// ═══════════════════════════════════════════════════════

static ref_cs s_object_cull_cs;
static ref_cs s_object_cull_debug_cs;
static ref_vs s_cull_debug_vs;
static ref_ps s_cull_debug_ps;
static ref_cs s_particle_cull_cs;
static ref_cs s_particle_cull_debug_cs;
static ref_cs s_batch_compact_cs;

// ═══════════════════════════════════════════════════════
//  CONSTRUCTOR / DESTRUCTOR
// ═══════════════════════════════════════════════════════

constexpr u32 MAX_CULLING_PARTICLES = 16384;

GPUCullingManager::GPUCullingManager()
    : m_objectCount(0)
    , m_maxObjects(MAX_CULLING_OBJECTS)
    , m_initialized(false)
    , m_computeEnabled(false)
    , m_particleCount(0)
    , m_maxParticles(MAX_CULLING_PARTICLES)
    , m_particleCullEnabled(false)
{
    m_objectData.reserve(MAX_CULLING_OBJECTS);
    m_particleData.reserve(MAX_CULLING_PARTICLES);
}

GPUCullingManager::~GPUCullingManager()
{
    Shutdown();
}

// ═══════════════════════════════════════════════════════
//  INITIALIZATION
// ═══════════════════════════════════════════════════════

void GPUCullingManager::Initialize(ng::RenderDevice* device)
{
    if (m_initialized)
        return;

    m_device = device;
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    if (!nvDevice) {
        Msg("! [GPUCulling] NVRHI device not available");
        return;
    }

    // Load compute shader
    s_object_cull_cs.create("object_cull");
    if (s_object_cull_cs && s_object_cull_cs->nvrhiShader) {
        Msg("* [GPUCulling] Loaded object_cull compute shader: OK");
        m_computeEnabled = true;
    } else {
        Msg("! [GPUCulling] object_cull.cs not found - GPU culling disabled");
        m_computeEnabled = false;
        m_initialized = true;
        return;
    }

    CreateBuffers(device);
    CreateComputePipeline(device);
    CreateCompactionResources(device);
    CreateDebugResources(device);
    CreateParticleResources(device);

    m_initialized = true;
    Msg("* [GPUCulling] Initialized (max objects: %d, max particles: %d, compact: %s)",
        m_maxObjects, m_maxParticles, m_compactEnabled ? "yes" : "no");
}

void GPUCullingManager::CreateBuffers(ng::RenderDevice* device)
{
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();

    // Object buffer: Structured buffer of GPUObjectData
    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_Objects";
        desc.byteSize = m_maxObjects * sizeof(GPUObjectData);
        desc.structStride = sizeof(GPUObjectData);
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;

        m_objectBuffer = nvDevice->createBuffer(desc);
        if (!m_objectBuffer) {
            Msg("! [GPUCulling] Failed to create object buffer");
            m_computeEnabled = false;
        }
    }

    // Visible index buffer: Structured buffer of u32
    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_VisibleIndices";
        desc.byteSize = m_maxObjects * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.canHaveUAVs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;

        m_visibleIndexBuffer = nvDevice->createBuffer(desc);
        if (!m_visibleIndexBuffer) {
            Msg("! [GPUCulling] Failed to create visible index buffer");
            m_computeEnabled = false;
        }
    }

    // Visible count buffer: Single u32 atomic counter
    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_VisibleCount";
        desc.byteSize = sizeof(u32);
        desc.canHaveUAVs = true;
        desc.canHaveRawViews = true;  // For InterlockedAdd
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;

        m_visibleCountBuffer = nvDevice->createBuffer(desc);
        if (!m_visibleCountBuffer) {
            Msg("! [GPUCulling] Failed to create visible count buffer");
            m_computeEnabled = false;
        }
    }

    // Draw arguments buffer: Indirect draw args for each batch
    // NOTE: Cannot use structured buffer with indirect args on D3D11
    // Must use raw buffer (byte address buffer) instead
    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_DrawArgs";
        desc.byteSize = m_maxObjects * sizeof(IndirectDrawArgs);
        // No structStride - use as raw buffer
        desc.canHaveUAVs = true;
        desc.canHaveRawViews = true;  // Allow RWByteAddressBuffer access
        desc.isDrawIndirectArgs = true;  // CRITICAL: Allows use with DrawIndexedIndirect
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;  // Let NVRHI handle state transitions

        m_drawArgsBuffer = nvDevice->createBuffer(desc);
        if (!m_drawArgsBuffer) {
            Msg("! [GPUCulling] Failed to create draw args buffer");
            m_computeEnabled = false;
        }
    }

    // Constant buffer
    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_Params";
        desc.byteSize = sizeof(CullParamsCB);
        desc.isConstantBuffer = true;
        desc.isVolatile = true;
        desc.maxVersions = 16;

        m_cullParamsCB = nvDevice->createBuffer(desc);
        if (!m_cullParamsCB) {
            Msg("! [GPUCulling] Failed to create constant buffer");
            m_computeEnabled = false;
        }
    }

    // Point sampler for Hi-Z (false = point filtering, true = linear)
    {
        nvrhi::SamplerDesc desc;
        desc.minFilter = false;  // Point filtering
        desc.magFilter = false;  // Point filtering
        desc.mipFilter = false;  // Point filtering
        desc.addressU = nvrhi::SamplerAddressMode::Clamp;
        desc.addressV = nvrhi::SamplerAddressMode::Clamp;
        desc.addressW = nvrhi::SamplerAddressMode::Clamp;

        m_pointSampler = nvDevice->createSampler(desc);
    }

    // ───────────────────────────────────────────────────────
    //  TERRAIN-SPECIFIC BUFFERS
    // ───────────────────────────────────────────────────────
    // Terrain uses same culling but separate draw call with terrain shader
    m_maxTerrainObjects = 4096;  // Typical level has ~1000-2000 terrain batches

    // Terrain object buffer
    // NOTE: Do NOT use keepInitialState - buffer needs state transitions for writeBuffer!
    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_TerrainObjects";
        desc.byteSize = m_maxTerrainObjects * sizeof(GPUObjectData);
        desc.structStride = sizeof(GPUObjectData);
        desc.initialState = nvrhi::ResourceStates::Common;  // Will be transitioned as needed

        m_terrainObjectBuffer = nvDevice->createBuffer(desc);
        if (!m_terrainObjectBuffer) {
            Msg("! [GPUCulling] Failed to create terrain object buffer");
        }
    }

    // Terrain visible index buffer
    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_TerrainVisibleIndices";
        desc.byteSize = m_maxTerrainObjects * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.canHaveUAVs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;

        m_terrainVisibleIndexBuffer = nvDevice->createBuffer(desc);
    }

    // Terrain visible count buffer
    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_TerrainVisibleCount";
        desc.byteSize = sizeof(u32);
        desc.canHaveUAVs = true;
        desc.canHaveRawViews = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;

        m_terrainVisibleCountBuffer = nvDevice->createBuffer(desc);
    }

    // Terrain draw arguments buffer
    // NOTE: Do NOT use keepInitialState - needs state transitions for writeBuffer AND culling shader!
    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_TerrainDrawArgs";
        desc.byteSize = m_maxTerrainObjects * sizeof(IndirectDrawArgs);
        desc.canHaveUAVs = true;
        desc.canHaveRawViews = true;
        desc.isDrawIndirectArgs = true;
        desc.initialState = nvrhi::ResourceStates::Common;  // Will be transitioned as needed

        m_terrainDrawArgsBuffer = nvDevice->createBuffer(desc);
    }

    // Terrain material ID buffer
    // NOTE: Do NOT use keepInitialState - buffer needs state transitions for writeBuffer!
    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_TerrainMaterialIDs";
        desc.byteSize = m_maxTerrainObjects * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.initialState = nvrhi::ResourceStates::Common;  // Will be transitioned as needed

        m_terrainMaterialIDBuffer = nvDevice->createBuffer(desc);
    }

    // Terrain instance buffer (world transforms - matches GPUInstanceData)
    // NOTE: Do NOT use keepInitialState - buffer needs state transitions for writeBuffer!
    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_TerrainInstanceData";
        desc.byteSize = m_maxTerrainObjects * sizeof(GPUInstanceData);
        desc.structStride = sizeof(GPUInstanceData);
        desc.initialState = nvrhi::ResourceStates::Common;  // Will be transitioned as needed

        m_terrainInstanceBuffer = nvDevice->createBuffer(desc);
    }

    // Terrain batch indices buffer (identity mapping: 0,1,2,3... for direct indexing)
    // NOTE: Do NOT use keepInitialState - buffer needs state transitions for writeBuffer!
    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_TerrainBatchIndices";
        desc.byteSize = m_maxTerrainObjects * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.initialState = nvrhi::ResourceStates::Common;  // Will be transitioned as needed

        m_terrainBatchIndicesBuffer = nvDevice->createBuffer(desc);
    }

    Msg("* [GPUCulling] Terrain buffers created (max: %u objects)", m_maxTerrainObjects);
}

void GPUCullingManager::CreateComputePipeline(ng::RenderDevice* device)
{
    if (!m_computeEnabled)
        return;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();

    // Create binding layout for main culling pass
    // Matches object_cull.cs bindings:
    // b5: CullParams (constant buffer)
    // t0: g_Objects (structured buffer SRV)
    // t1: g_HiZPyramid (texture SRV)
    // s0: g_PointSampler
    // u0: g_VisibleIndices (structured buffer UAV)
    // u1: g_VisibleCount (raw buffer UAV)
    // u2: g_DrawArgs (raw buffer UAV - can't be structured with indirect args on D3D11)
    {
        nvrhi::BindingLayoutDesc layoutDesc;
        layoutDesc.visibility = nvrhi::ShaderType::Compute;
        layoutDesc.bindings = {
            nvrhi::BindingLayoutItem::VolatileConstantBuffer(5),  // volatile CB
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(0),
            nvrhi::BindingLayoutItem::Texture_SRV(1),
            nvrhi::BindingLayoutItem::Sampler(0),
            nvrhi::BindingLayoutItem::StructuredBuffer_UAV(0),
            nvrhi::BindingLayoutItem::RawBuffer_UAV(1),
            nvrhi::BindingLayoutItem::RawBuffer_UAV(2)  // Draw args (raw buffer for D3D11 compatibility)
        };

        m_cullLayout = nvDevice->createBindingLayout(layoutDesc);
        if (!m_cullLayout) {
            Msg("! [GPUCulling] Failed to create binding layout");
            m_computeEnabled = false;
            return;
        }
    }

    // Create compute pipeline for main culling
    {
        nvrhi::ComputePipelineDesc pipeDesc;
        pipeDesc.CS = s_object_cull_cs->nvrhiShader;
        pipeDesc.bindingLayouts = { m_cullLayout };

        m_cullPipeline = nvDevice->createComputePipeline(pipeDesc);
        if (!m_cullPipeline) {
            Msg("! [GPUCulling] Failed to create compute pipeline");
            m_computeEnabled = false;
            return;
        }
    }

    Msg("* [GPUCulling] Compute pipeline created successfully");
}

void GPUCullingManager::CreateCompactionResources(ng::RenderDevice* device)
{
    if (!m_computeEnabled)
        return;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();

    s_batch_compact_cs.create("batch_compact");
    if (!s_batch_compact_cs || !s_batch_compact_cs->nvrhiShader) {
        Msg("! [GPUCulling] batch_compact.cs not found - compaction disabled");
        return;
    }

    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_CompactDrawArgs";
        desc.byteSize = m_maxObjects * sizeof(IndirectDrawArgs);
        // No structStride - raw buffer for D3D11 indirect args compatibility
        desc.canHaveUAVs = true;
        desc.canHaveRawViews = true;  // RWByteAddressBuffer access
        desc.isDrawIndirectArgs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;  // Let NVRHI handle state transitions

        m_compactDrawArgsBuffer = nvDevice->createBuffer(desc);
        if (!m_compactDrawArgsBuffer) {
            Msg("! [GPUCulling] Failed to create compact draw args buffer");
            return;
        }
    }

    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_CompactBatchIndices";
        desc.byteSize = m_maxObjects * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.canHaveUAVs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;  // Let NVRHI handle state transitions

        m_compactBatchIndicesBuffer = nvDevice->createBuffer(desc);
        if (!m_compactBatchIndicesBuffer) {
            Msg("! [GPUCulling] Failed to create compact batch indices buffer");
            return;
        }
    }

    // Material ID buffers for bindless rendering
    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_MaterialIDs";
        desc.byteSize = m_maxObjects * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;

        m_materialIDBuffer = nvDevice->createBuffer(desc);
        if (!m_materialIDBuffer) {
            Msg("! [GPUCulling] Failed to create material ID buffer");
            return;
        }
    }

    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_CompactMaterialIDs";
        desc.byteSize = m_maxObjects * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.canHaveUAVs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;  // Let NVRHI handle state transitions

        m_compactMaterialIDBuffer = nvDevice->createBuffer(desc);
        if (!m_compactMaterialIDBuffer) {
            Msg("! [GPUCulling] Failed to create compact material ID buffer");
            return;
        }
    }

    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_CompactCount";
        desc.byteSize = sizeof(u32);
        desc.canHaveUAVs = true;
        desc.canHaveRawViews = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;

        m_compactCountBuffer = nvDevice->createBuffer(desc);
        if (!m_compactCountBuffer) {
            Msg("! [GPUCulling] Failed to create compact count buffer");
            return;
        }
    }

    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_CompactParams";
        desc.byteSize = 16;
        desc.isConstantBuffer = true;
        desc.isVolatile = true;
        desc.maxVersions = 16;

        m_compactParamsCB = nvDevice->createBuffer(desc);
        if (!m_compactParamsCB) {
            Msg("! [GPUCulling] Failed to create compact params CB");
            return;
        }
    }

    {
        // Binding layout for batch_compact.cs:
        // b5: CompactParams (constant buffer)
        // t0: g_InputDrawArgs (ByteAddressBuffer - raw buffer SRV)
        // t1: g_InputMaterialIDs (StructuredBuffer<uint> - SRV)
        // u0: g_OutputDrawArgs (RWByteAddressBuffer - raw buffer UAV)
        // u1: g_VisibleBatchIndices (RWStructuredBuffer<uint>)
        // u2: g_VisibleCount (RWByteAddressBuffer)
        // u3: g_OutputMaterialIDs (RWStructuredBuffer<uint>)
        nvrhi::BindingLayoutDesc layoutDesc;
        layoutDesc.visibility = nvrhi::ShaderType::Compute;
        layoutDesc.bindings = {
            nvrhi::BindingLayoutItem::VolatileConstantBuffer(5),  // volatile CB
            nvrhi::BindingLayoutItem::RawBuffer_SRV(0),          // Input draw args
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(1),   // Input material IDs
            nvrhi::BindingLayoutItem::RawBuffer_UAV(0),          // Output draw args
            nvrhi::BindingLayoutItem::StructuredBuffer_UAV(1),   // Batch indices
            nvrhi::BindingLayoutItem::RawBuffer_UAV(2),          // Visible count
            nvrhi::BindingLayoutItem::StructuredBuffer_UAV(3)    // Output material IDs
        };

        m_compactLayout = nvDevice->createBindingLayout(layoutDesc);
        if (!m_compactLayout) {
            Msg("! [GPUCulling] Failed to create compact binding layout");
            return;
        }
    }

    {
        nvrhi::ComputePipelineDesc pipeDesc;
        pipeDesc.CS = s_batch_compact_cs->nvrhiShader;
        pipeDesc.bindingLayouts = { m_compactLayout };

        m_compactPipeline = nvDevice->createComputePipeline(pipeDesc);
        if (!m_compactPipeline) {
            Msg("! [GPUCulling] Failed to create compact pipeline");
            return;
        }
    }

    m_compactEnabled = true;
    Msg("* [GPUCulling] Compaction resources created");
}

void GPUCullingManager::Shutdown()
{
    m_objectBuffer = nullptr;
    m_visibleIndexBuffer = nullptr;
    m_visibleCountBuffer = nullptr;
    m_cullParamsCB = nullptr;
    m_cullPipeline = nullptr;
    m_cullLayout = nullptr;
    m_pointSampler = nullptr;

    m_compactDrawArgsBuffer = nullptr;
    m_compactBatchIndicesBuffer = nullptr;
    m_compactCountBuffer = nullptr;
    m_compactParamsCB = nullptr;
    m_compactPipeline = nullptr;
    m_compactLayout = nullptr;
    m_materialIDBuffer = nullptr;
    m_compactMaterialIDBuffer = nullptr;

    m_debugBuffer = nullptr;
    m_debugComputeParamsCB = nullptr;
    m_debugGraphicsParamsCB = nullptr;
    m_debugComputePipeline = nullptr;
    m_particleDebugComputePipeline = nullptr;
    m_debugComputeLayout = nullptr;
    m_debugGraphicsPipeline = nullptr;
    m_debugGraphicsLayout = nullptr;
    m_debugInputLayout = nullptr;

    m_particleBuffer = nullptr;
    m_particleDrawArgsBuffer = nullptr;
    m_particleVisibleCountBuffer = nullptr;
    m_particleCullParamsCB = nullptr;
    m_particleCullPipeline = nullptr;
    m_particleCullLayout = nullptr;

    // Mega-buffer resources
    m_megaVertexBuffer = nullptr;
    m_megaIndexBuffer = nullptr;
    m_instanceBuffer = nullptr;
    m_megaVertices.clear();
    m_megaIndices.clear();
    m_instanceData.clear();
    m_totalVertexCount = 0;
    m_totalIndexCount = 0;
    m_maxMegaVertices = 0;
    m_maxMegaIndices = 0;
    m_megaBuffersReady = false;
    m_levelLoadInProgress = false;

    // Terrain buffers
    m_terrainObjectBuffer = nullptr;
    m_terrainDrawArgsBuffer = nullptr;
    m_terrainVisibleIndexBuffer = nullptr;
    m_terrainVisibleCountBuffer = nullptr;
    m_terrainInstanceBuffer = nullptr;
    m_terrainBatchIndicesBuffer = nullptr;
    m_terrainMaterialIDBuffer = nullptr;
    m_terrainCompactDrawArgsBuffer = nullptr;
    m_terrainCompactBatchIndicesBuffer = nullptr;
    m_terrainCompactCountBuffer = nullptr;
    m_terrainObjectData.clear();
    m_terrainDrawArgsData.clear();
    m_terrainMaterialIDData.clear();
    m_terrainInstanceData.clear();
    m_terrainObjectCount = 0;

    m_initialized = false;
    m_computeEnabled = false;
    m_compactEnabled = false;
    m_particleCullEnabled = false;
    m_objectCount = 0;
    m_particleCount = 0;
}

// ═══════════════════════════════════════════════════════
//  UPLOAD SCENE OBJECTS
// ═══════════════════════════════════════════════════════

void GPUCullingManager::UploadSceneObjects(ng::RenderContext* ctx, const GeometryCollector* geometry)
{
    if (!m_computeEnabled || !geometry)
        return;

    const auto& batches = geometry->GetBatches();
    u32 totalBatches = static_cast<u32>(batches.size());

    if (totalBatches == 0)
        return;

    // Build object data, draw args, and material ID arrays
    // NOTE: Skip skinned batches - they use separate per-draw rendering with bone matrices
    // NOTE: Terrain batches are tracked separately for terrain shader rendering
    m_objectData.clear();
    m_objectData.reserve(totalBatches);
    m_drawArgsData.clear();
    m_drawArgsData.reserve(totalBatches);
    m_materialIDData.clear();
    m_materialIDData.reserve(totalBatches);

    // Terrain-specific arrays
    m_terrainObjectData.clear();
    m_terrainObjectData.reserve(totalBatches / 4);  // Terrain is typically ~25% of batches
    m_terrainDrawArgsData.clear();
    m_terrainDrawArgsData.reserve(totalBatches / 4);
    m_terrainMaterialIDData.clear();
    m_terrainMaterialIDData.reserve(totalBatches / 4);
    m_terrainInstanceData.clear();
    m_terrainInstanceData.reserve(totalBatches / 4);

    for (u32 i = 0; i < totalBatches; i++) {
        const auto& batch = batches[i];

        // Skip skinned batches - they use a separate per-draw rendering path
        // with bone matrices and cannot use the GPU-driven multi-draw system
        if (batch.isSkinned)
            continue;

        // Route terrain batches to separate arrays for terrain shader rendering
        if (batch.isTerrain) {
            // ─────────────────────────────────────────────────────
            //  TERRAIN BATCH - Goes to terrain arrays
            // ─────────────────────────────────────────────────────
            GPUObjectData obj;
            obj.position = batch.worldBoundsCenter;
            obj.radius = batch.worldBoundsRadius;
            obj.batchIndex = static_cast<u32>(m_terrainObjectData.size());
            obj.flags = GPU_OBJECT_OPAQUE;  // Terrain is always opaque
            obj.pad0 = 0.0f;
            obj.pad1 = 0.0f;
            m_terrainObjectData.push_back(obj);

            // Terrain draw args
            IndirectDrawArgs args;
            args.indexCountPerInstance = batch.indexCount;
            args.instanceCount = 0;  // Set by culling shader if visible
            if (batch.megaBufferAlloc.valid) {
                args.startIndexLocation = batch.megaBufferAlloc.indexOffset;
                args.baseVertexLocation = static_cast<s32>(batch.megaBufferAlloc.vertexOffset);
            } else {
                args.startIndexLocation = batch.startIndex;
                args.baseVertexLocation = batch.baseVertex;
            }
            // CRITICAL: StartInstanceLocation provides draw index to shader
            // This indexes into terrain material/instance buffers
            args.startInstanceLocation = static_cast<u32>(m_terrainDrawArgsData.size());
            m_terrainDrawArgsData.push_back(args);

            // Terrain material ID (index into TerrainMaterialBuffer, not regular MaterialBuffer)
            m_terrainMaterialIDData.push_back(batch.terrainMaterialID);

            // Terrain instance data (world transform)
            GPUInstanceData inst;
            inst.world.transpose(batch.worldMatrix);
            inst.materialID = batch.terrainMaterialID;  // Terrain material ID
            inst.flags = GPU_OBJECT_OPAQUE;  // Terrain is always opaque
            inst.pad0 = 0.0f;
            inst.pad1 = 0.0f;
            m_terrainInstanceData.push_back(inst);
            continue;
        }

        // ─────────────────────────────────────────────────────
        //  BUILD OBJECT DATA (for culling)
        // ─────────────────────────────────────────────────────
        GPUObjectData obj;

        // Use pre-computed world-space bounding sphere from batch
        // This handles different visual types correctly:
        // - Static geometry: identity transform applied (no-op)
        // - Trees: no transform applied (sphere already world-space)
        // - Dynamic objects: worldMatrix transform applied
        obj.position = batch.worldBoundsCenter;
        obj.radius = batch.worldBoundsRadius;

        // Use GPU-local index (position in filtered array) for instance data lookup
        obj.batchIndex = static_cast<u32>(m_objectData.size());

        // Set flags based on batch properties
        obj.flags = 0;
        if (batch.IsOpaque())
            obj.flags |= GPU_OBJECT_OPAQUE;
        if (batch.IsAlphaTested())
            obj.flags |= GPU_OBJECT_ALPHA_TEST;
        if (batch.IsStrictB2F())
            obj.flags |= GPU_OBJECT_TRANSPARENT;

        obj.pad0 = 0.0f;
        obj.pad1 = 0.0f;

        m_objectData.push_back(obj);

        // ─────────────────────────────────────────────────────
        //  BUILD DRAW ARGS (for indirect draw)
        // ─────────────────────────────────────────────────────
        // Use mega-buffer offsets for GPU-driven rendering
        IndirectDrawArgs args;
        args.indexCountPerInstance = batch.indexCount;
        args.instanceCount = 0;  // Will be set to 1 by culling shader if visible
        if (batch.megaBufferAlloc.valid) {
            // GPU-driven path: use mega-buffer offsets
            args.startIndexLocation = batch.megaBufferAlloc.indexOffset;
            args.baseVertexLocation = static_cast<s32>(batch.megaBufferAlloc.vertexOffset);
        } else {
            // Fallback: use per-batch offsets (won't work with mega-buffer bound)
            args.startIndexLocation = batch.startIndex;
            args.baseVertexLocation = batch.baseVertex;
        }
        args.startInstanceLocation = 0;

        m_drawArgsData.push_back(args);

        // ─────────────────────────────────────────────────────
        //  MATERIAL ID (for bindless rendering)
        // ─────────────────────────────────────────────────────
        m_materialIDData.push_back(batch.bindlessMaterialID);
    }

    // Set object count based on how many non-skinned batches we added
    m_objectCount = std::min(static_cast<u32>(m_objectData.size()), m_maxObjects);

    if (m_objectCount == 0)
        return;

    // Upload to GPU
    nvrhi::ICommandList* cmdList = ctx->GetCommandList();

    // ─────────────────────────────────────────────────────
    //  MEGA-BUFFER UPLOAD (one-time, for GPU-driven rendering)
    // ─────────────────────────────────────────────────────
    // Upload mega vertex/index data on first frame after level load
    static bool s_megaDataUploaded = false;
    if (!s_megaDataUploaded && m_megaBuffersReady &&
        !m_megaVertices.empty() && !m_megaIndices.empty() &&
        m_megaVertexBuffer && m_megaIndexBuffer) {

        cmdList->writeBuffer(m_megaVertexBuffer,
            m_megaVertices.data(),
            m_megaVertices.size() * sizeof(bindless::UnifiedVertex));

        cmdList->writeBuffer(m_megaIndexBuffer,
            m_megaIndices.data(),
            m_megaIndices.size() * sizeof(u32));

        s_megaDataUploaded = true;

        Msg("* [GPUCulling] Mega-buffer data uploaded: %zu vertices, %zu indices",
            m_megaVertices.size(), m_megaIndices.size());

        // Free CPU memory after upload
        m_megaVertices.clear();
        m_megaVertices.shrink_to_fit();
        m_megaIndices.clear();
        m_megaIndices.shrink_to_fit();
    }

    cmdList->writeBuffer(m_objectBuffer, m_objectData.data(), m_objectCount * sizeof(GPUObjectData));
    cmdList->writeBuffer(m_drawArgsBuffer, m_drawArgsData.data(), m_objectCount * sizeof(IndirectDrawArgs));

    // Upload material IDs if compaction is enabled
    if (m_compactEnabled && m_materialIDBuffer) {
        cmdList->writeBuffer(m_materialIDBuffer, m_materialIDData.data(), m_objectCount * sizeof(u32));
    }

    // ─────────────────────────────────────────────────────
    //  TERRAIN BUFFER UPLOADS
    // ─────────────────────────────────────────────────────
    m_terrainObjectCount = std::min(static_cast<u32>(m_terrainObjectData.size()), m_maxTerrainObjects);

    if (m_terrainObjectCount > 0 && m_terrainObjectBuffer && m_terrainDrawArgsBuffer) {
        // Upload terrain object data (for culling) with explicit state tracking
        cmdList->beginTrackingBufferState(m_terrainObjectBuffer, nvrhi::ResourceStates::CopyDest);
        cmdList->writeBuffer(m_terrainObjectBuffer,
            m_terrainObjectData.data(),
            m_terrainObjectCount * sizeof(GPUObjectData));
        cmdList->setBufferState(m_terrainObjectBuffer, nvrhi::ResourceStates::ShaderResource);

        // Upload terrain draw args with explicit state tracking
        // This buffer contains StartInstanceLocation which is CRITICAL for material ID lookup!
        cmdList->beginTrackingBufferState(m_terrainDrawArgsBuffer, nvrhi::ResourceStates::CopyDest);
        cmdList->writeBuffer(m_terrainDrawArgsBuffer,
            m_terrainDrawArgsData.data(),
            m_terrainObjectCount * sizeof(IndirectDrawArgs));
        // Leave in IndirectArgument state for drawIndexedIndirect
        cmdList->setBufferState(m_terrainDrawArgsBuffer, nvrhi::ResourceStates::IndirectArgument);

        // Upload terrain material IDs with explicit state tracking
        if (m_terrainMaterialIDBuffer) {
            cmdList->beginTrackingBufferState(m_terrainMaterialIDBuffer, nvrhi::ResourceStates::CopyDest);
            cmdList->writeBuffer(m_terrainMaterialIDBuffer,
                m_terrainMaterialIDData.data(),
                m_terrainObjectCount * sizeof(u32));
            cmdList->setBufferState(m_terrainMaterialIDBuffer, nvrhi::ResourceStates::ShaderResource);

            // Debug: Check for invalid terrain material IDs
            static bool s_materialIDLogged = false;
            if (!s_materialIDLogged) {
                u32 invalidCount = 0;
                u32 maxID = 0;
                for (u32 i = 0; i < m_terrainObjectCount && i < m_terrainMaterialIDData.size(); i++) {
                    if (m_terrainMaterialIDData[i] == UINT32_MAX)
                        invalidCount++;
                    else if (m_terrainMaterialIDData[i] > maxID)
                        maxID = m_terrainMaterialIDData[i];
                }
                Msg("* [GPUCulling] Terrain material IDs: count=%u, maxID=%u, invalid=%u",
                    m_terrainObjectCount, maxID, invalidCount);
                if (invalidCount > 0) {
                    Msg("! [GPUCulling] WARNING: %u terrain batches have invalid material IDs!", invalidCount);
                }
                s_materialIDLogged = true;
            }
        }

        // Upload terrain instance data (world transforms) with explicit state tracking
        if (m_terrainInstanceBuffer && !m_terrainInstanceData.empty()) {
            cmdList->beginTrackingBufferState(m_terrainInstanceBuffer, nvrhi::ResourceStates::CopyDest);
            cmdList->writeBuffer(m_terrainInstanceBuffer,
                m_terrainInstanceData.data(),
                m_terrainObjectCount * sizeof(GPUInstanceData));
            cmdList->setBufferState(m_terrainInstanceBuffer, nvrhi::ResourceStates::ShaderResource);
        }

        // Upload terrain batch indices (identity mapping: 0,1,2,3...) with explicit state tracking
        // Terrain uses direct indexing without compaction
        if (m_terrainBatchIndicesBuffer) {
            xr_vector<u32> identityIndices(m_terrainObjectCount);
            for (u32 i = 0; i < m_terrainObjectCount; i++)
                identityIndices[i] = i;
            cmdList->beginTrackingBufferState(m_terrainBatchIndicesBuffer, nvrhi::ResourceStates::CopyDest);
            cmdList->writeBuffer(m_terrainBatchIndicesBuffer,
                identityIndices.data(),
                m_terrainObjectCount * sizeof(u32));
            cmdList->setBufferState(m_terrainBatchIndicesBuffer, nvrhi::ResourceStates::ShaderResource);
        }

        static bool s_terrainLogged = false;
        if (!s_terrainLogged && m_terrainObjectCount > 0) {
            Msg("* [GPUCulling] Terrain batches: %u objects uploaded", m_terrainObjectCount);
            s_terrainLogged = true;
        }
    }

    // Upload instance data (world matrices) for GPU-driven rendering
    UploadInstanceData(ctx, geometry);
}

// ═══════════════════════════════════════════════════════
//  FRUSTUM PLANE EXTRACTION
// ═══════════════════════════════════════════════════════

void GPUCullingManager::ExtractFrustumPlanes(Fmatrix& M, Fvector4* outPlanes)
{
    // Use CFrustum::CreateFromMatrix - EXACTLY matches detail_cull.cs setup
    // Extract LRTB + FAR planes (5 planes), skip NEAR to avoid culling close objects
    CFrustum frustum;
    frustum.CreateFromMatrix(M, FRUSTUM_P_LRTB | FRUSTUM_P_FAR);

    for (u32 i = 0; i < frustum.p_count && i < 6; i++)
    {
        outPlanes[i].set(
            frustum.planes[i].n.x,
            frustum.planes[i].n.y,
            frustum.planes[i].n.z,
            frustum.planes[i].d
        );
    }
}

// ═══════════════════════════════════════════════════════
//  SETUP CULLING PASS
// ═══════════════════════════════════════════════════════

GPUCullOutput GPUCullingManager::SetupCullingPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle hizPyramid,
    u32 hizWidth,
    u32 hizHeight,
    u32 hizMipLevels,
    const GeometryCollector* geometry)
{
    using namespace framegraph;

    GPUCullOutput output;
    output.maxObjects = m_maxObjects;

    // Early out if not enabled
    if (!m_computeEnabled) {
        output.visibleIndices = VirtualResourceHandle();
        output.visibleCount = VirtualResourceHandle();
        output.drawArgsBuffer = VirtualResourceHandle();
        return output;
    }

    // Pre-compute object count for pass data
    const auto& batches = geometry->GetBatches();
    m_objectCount = std::min(static_cast<u32>(batches.size()), m_maxObjects);

    if (m_objectCount == 0) {
        output.visibleIndices = VirtualResourceHandle();
        output.visibleCount = VirtualResourceHandle();
        output.drawArgsBuffer = VirtualResourceHandle();
        return output;
    }

    struct GPUCullPassData {
        VirtualResourceHandle hizPyramid;
        VirtualResourceHandle drawArgsBuffer;  // For framegraph tracking

        GPUCullingManager* manager;
        const GeometryCollector* geometry;  // For uploading during execute
        u32 objectCount;
        u32 hizWidth;
        u32 hizHeight;
        u32 hizMipLevels;
    };

    // Import draw args buffer into framegraph for proper state tracking
    // This allows forward pass to properly transition the buffer to IndirectArgument state
    ResourceDesc drawArgsDesc;
    drawArgsDesc.type = ResourceDesc::Type::Buffer;
    drawArgsDesc.debugName = "GPUCull_DrawArgs";
    drawArgsDesc.bufferSize = m_maxObjects * sizeof(IndirectDrawArgs);
    drawArgsDesc.structStride = sizeof(IndirectDrawArgs);
    drawArgsDesc.isUAV = true;
    drawArgsDesc.isTransient = false;  // Persistent - forward pass needs it

    VirtualResourceHandle drawArgsHandle = fg.ImportBuffer("gpu_cull_drawargs", m_drawArgsBuffer, drawArgsDesc);

    auto& passData = fg.addCallbackPass<GPUCullPassData>(
        "GPU Culling",

        // Setup lambda
        [&, hizWidth, hizHeight, hizMipLevels, drawArgsHandle, geometry](FrameGraph& builder, PassHandle passHandle, GPUCullPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.manager = this;
            data.geometry = geometry;  // Capture for upload during execute
            data.objectCount = m_objectCount;
            data.hizWidth = hizWidth;
            data.hizHeight = hizHeight;
            data.hizMipLevels = hizMipLevels;

            // Read Hi-Z pyramid
            data.hizPyramid = passBuilder.read(hizPyramid, ResourceState::ShaderResource);

            // Write draw args buffer (culling shader enables visible draws)
            data.drawArgsBuffer = passBuilder.write(drawArgsHandle, ResourceState::UnorderedAccess);
        },

        // Execute lambda
        [](const GPUCullPassData& data,
           const FrameGraph& fg,
           ng::RenderContext* ctx) {

            GPUCullingManager* mgr = data.manager;
            if (!mgr->m_computeEnabled || data.objectCount == 0)
                return;

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();

            nvrhi::IDevice* nvDevice = mgr->m_device->GetNVRHIDevice();

            // ─────────────────────────────────────────────────────
            //  UPLOAD SCENE OBJECTS (must happen during execute, not setup)
            // ─────────────────────────────────────────────────────
            // This ensures we use the correct command list
            mgr->UploadSceneObjects(ctx, data.geometry);

            // Get Hi-Z texture
            nvrhi::ITexture* hizTexture = fg.GetPhysicalTexture(data.hizPyramid);
            if (!hizTexture) {
                Msg("! [GPUCulling] Hi-Z texture not available");
                return;
            }

            // Clear visible count to 0
            u32 zero = 0;
            cmdList->writeBuffer(mgr->m_visibleCountBuffer, &zero, sizeof(u32));

            // Fill constant buffer
            CullParamsCB cb;
            // Must transpose matrices - Fmatrix storage is transposed relative to HLSL row-major
            cb.viewProj.transpose(Device.mFullTransform);
            cb.cameraPos = Device.vCameraPosition;
            // Use environment far plane for culling distance
            float farPlane = g_pGamePersistent ? g_pGamePersistent->Environment().CurrentEnv.far_plane : 300.0f;
            cb.maxDistanceSq = farPlane * farPlane;
            cb.objectCount = data.objectCount;
            cb.hizWidth = data.hizWidth;
            cb.hizHeight = data.hizHeight;
            cb.hizMipLevels = data.hizMipLevels;

            mgr->ExtractFrustumPlanes(Device.mFullTransform, cb.frustumPlanes);

            cmdList->writeBuffer(mgr->m_cullParamsCB, &cb, sizeof(cb));

            // Create binding set (includes draw args UAV at u2)
            nvrhi::BindingSetDesc bindDesc;
            bindDesc.bindings = {
                nvrhi::BindingSetItem::ConstantBuffer(5, mgr->m_cullParamsCB),
                nvrhi::BindingSetItem::StructuredBuffer_SRV(0, mgr->m_objectBuffer),
                nvrhi::BindingSetItem::Texture_SRV(1, hizTexture),
                nvrhi::BindingSetItem::Sampler(0, mgr->m_pointSampler),
                nvrhi::BindingSetItem::StructuredBuffer_UAV(0, mgr->m_visibleIndexBuffer),
                nvrhi::BindingSetItem::RawBuffer_UAV(1, mgr->m_visibleCountBuffer),
                nvrhi::BindingSetItem::RawBuffer_UAV(2, mgr->m_drawArgsBuffer)  // Raw buffer for D3D11
            };

            nvrhi::BindingSetHandle bindingSet = nvDevice->createBindingSet(bindDesc, mgr->m_cullLayout);
            if (!bindingSet) {
                Msg("! [GPUCulling] Failed to create binding set");
                return;
            }

            // Set compute state and dispatch culling
            nvrhi::ComputeState state;
            state.pipeline = mgr->m_cullPipeline;
            state.bindings = { bindingSet };
            cmdList->setComputeState(state);

            u32 groupCount = (data.objectCount + CULL_THREAD_GROUP_SIZE - 1) / CULL_THREAD_GROUP_SIZE;
            cmdList->dispatch(groupCount, 1, 1);

            // ─────────────────────────────────────────────────────
            //  BATCH COMPACTION PASS
            // ─────────────────────────────────────────────────────
            // Compacts visible batches into contiguous list for efficient multi-draw
            if (mgr->m_compactEnabled) {
                // Barrier: Draw args UAV -> SRV for compaction reading
                cmdList->setBufferState(mgr->m_drawArgsBuffer, nvrhi::ResourceStates::ShaderResource);

                // Clear compact count to 0
                u32 zeroCount = 0;
                cmdList->writeBuffer(mgr->m_compactCountBuffer, &zeroCount, sizeof(u32));

                // Clear compact draw args buffer - all slots get instanceCount=0 (no-op draws)
                // Compaction pass will set instanceCount=1 only for visible batches
                cmdList->clearBufferUInt(mgr->m_compactDrawArgsBuffer, 0);

                // Update compact params
                struct CompactParamsCB {
                    u32 batchCount;
                    u32 padding[3];
                };
                CompactParamsCB compactCB;
                compactCB.batchCount = data.objectCount;
                compactCB.padding[0] = compactCB.padding[1] = compactCB.padding[2] = 0;
                cmdList->writeBuffer(mgr->m_compactParamsCB, &compactCB, sizeof(compactCB));

                // Create binding set for compaction
                nvrhi::BindingSetDesc compactBindDesc;
                compactBindDesc.bindings = {
                    nvrhi::BindingSetItem::ConstantBuffer(5, mgr->m_compactParamsCB),
                    nvrhi::BindingSetItem::RawBuffer_SRV(0, mgr->m_drawArgsBuffer),
                    nvrhi::BindingSetItem::StructuredBuffer_SRV(1, mgr->m_materialIDBuffer),
                    nvrhi::BindingSetItem::RawBuffer_UAV(0, mgr->m_compactDrawArgsBuffer),
                    nvrhi::BindingSetItem::StructuredBuffer_UAV(1, mgr->m_compactBatchIndicesBuffer),
                    nvrhi::BindingSetItem::RawBuffer_UAV(2, mgr->m_compactCountBuffer),
                    nvrhi::BindingSetItem::StructuredBuffer_UAV(3, mgr->m_compactMaterialIDBuffer)
                };

                nvrhi::BindingSetHandle compactBindingSet = nvDevice->createBindingSet(compactBindDesc, mgr->m_compactLayout);
                if (compactBindingSet) {
                    nvrhi::ComputeState compactState;
                    compactState.pipeline = mgr->m_compactPipeline;
                    compactState.bindings = { compactBindingSet };
                    cmdList->setComputeState(compactState);
                    cmdList->dispatch(groupCount, 1, 1);

                    // Transition compact buffers to IndirectArgument for ExecuteIndirect with count
                    cmdList->setBufferState(mgr->m_compactDrawArgsBuffer, nvrhi::ResourceStates::IndirectArgument);
                    cmdList->setBufferState(mgr->m_compactCountBuffer, nvrhi::ResourceStates::IndirectArgument);
                }
            }

            // ─────────────────────────────────────────────────────
            //  TERRAIN CULLING PASS (uses same shader, different data)
            // ─────────────────────────────────────────────────────
            if (mgr->m_terrainObjectCount > 0 && mgr->m_terrainObjectBuffer && mgr->m_terrainDrawArgsBuffer) {
                // Clear terrain visible count
                u32 zeroTerrain = 0;
                cmdList->writeBuffer(mgr->m_terrainVisibleCountBuffer, &zeroTerrain, sizeof(u32));

                // Update constant buffer for terrain (reuse same CB, different object count)
                CullParamsCB terrainCB;
                terrainCB.viewProj.transpose(Device.mFullTransform);
                terrainCB.cameraPos = Device.vCameraPosition;
                float farPlane = g_pGamePersistent ? g_pGamePersistent->Environment().CurrentEnv.far_plane : 300.0f;
                terrainCB.maxDistanceSq = farPlane * farPlane;
                terrainCB.objectCount = mgr->m_terrainObjectCount;
                terrainCB.hizWidth = data.hizWidth;
                terrainCB.hizHeight = data.hizHeight;
                terrainCB.hizMipLevels = data.hizMipLevels;
                mgr->ExtractFrustumPlanes(Device.mFullTransform, terrainCB.frustumPlanes);
                cmdList->writeBuffer(mgr->m_cullParamsCB, &terrainCB, sizeof(terrainCB));

                // Create terrain binding set (same layout, different buffers)
                nvrhi::BindingSetDesc terrainBindDesc;
                terrainBindDesc.bindings = {
                    nvrhi::BindingSetItem::ConstantBuffer(5, mgr->m_cullParamsCB),
                    nvrhi::BindingSetItem::StructuredBuffer_SRV(0, mgr->m_terrainObjectBuffer),
                    nvrhi::BindingSetItem::Texture_SRV(1, hizTexture),
                    nvrhi::BindingSetItem::Sampler(0, mgr->m_pointSampler),
                    nvrhi::BindingSetItem::StructuredBuffer_UAV(0, mgr->m_terrainVisibleIndexBuffer),
                    nvrhi::BindingSetItem::RawBuffer_UAV(1, mgr->m_terrainVisibleCountBuffer),
                    nvrhi::BindingSetItem::RawBuffer_UAV(2, mgr->m_terrainDrawArgsBuffer)
                };

                nvrhi::BindingSetHandle terrainBindingSet = nvDevice->createBindingSet(terrainBindDesc, mgr->m_cullLayout);
                if (terrainBindingSet) {
                    nvrhi::ComputeState terrainState;
                    terrainState.pipeline = mgr->m_cullPipeline;
                    terrainState.bindings = { terrainBindingSet };
                    cmdList->setComputeState(terrainState);

                    u32 terrainGroupCount = (mgr->m_terrainObjectCount + CULL_THREAD_GROUP_SIZE - 1) / CULL_THREAD_GROUP_SIZE;
                    cmdList->dispatch(terrainGroupCount, 1, 1);

                    // Transition terrain draw args to IndirectArgument state for DrawIndexedIndirect
                    cmdList->setBufferState(mgr->m_terrainDrawArgsBuffer, nvrhi::ResourceStates::IndirectArgument);
                }
            }
        }
    );

    output.visibleIndices = VirtualResourceHandle();  // Not using framegraph for these
    output.visibleCount = VirtualResourceHandle();
    output.drawArgsBuffer = passData.drawArgsBuffer;

    // Set compact output handles if compaction is enabled
    if (m_compactEnabled) {
        // Import compact buffers into framegraph
        ResourceDesc compactArgsDesc;
        compactArgsDesc.type = ResourceDesc::Type::Buffer;
        compactArgsDesc.debugName = "GPUCull_CompactDrawArgs";
        compactArgsDesc.bufferSize = m_maxObjects * sizeof(IndirectDrawArgs);
        compactArgsDesc.isUAV = true;
        compactArgsDesc.isTransient = false;

        ResourceDesc compactIndicesDesc;
        compactIndicesDesc.type = ResourceDesc::Type::Buffer;
        compactIndicesDesc.debugName = "GPUCull_CompactBatchIndices";
        compactIndicesDesc.bufferSize = m_maxObjects * sizeof(u32);
        compactIndicesDesc.structStride = sizeof(u32);
        compactIndicesDesc.isUAV = true;
        compactIndicesDesc.isTransient = false;

        output.compactDrawArgs = fg.ImportBuffer("gpu_cull_compact_drawargs", m_compactDrawArgsBuffer, compactArgsDesc);
        output.compactBatchIndices = fg.ImportBuffer("gpu_cull_compact_batchindices", m_compactBatchIndicesBuffer, compactIndicesDesc);
    } else {
        output.compactDrawArgs = VirtualResourceHandle();
        output.compactBatchIndices = VirtualResourceHandle();
    }

    // ───────────────────────────────────────────────────────
    //  TERRAIN OUTPUT HANDLES
    // ───────────────────────────────────────────────────────
    output.terrainObjectCount = m_terrainObjectCount;

    if (m_terrainObjectCount > 0 && m_terrainDrawArgsBuffer) {
        // Import terrain draw args into framegraph
        ResourceDesc terrainArgsDesc;
        terrainArgsDesc.type = ResourceDesc::Type::Buffer;
        terrainArgsDesc.debugName = "GPUCull_TerrainDrawArgs";
        terrainArgsDesc.bufferSize = m_maxTerrainObjects * sizeof(IndirectDrawArgs);
        terrainArgsDesc.isUAV = true;
        terrainArgsDesc.isTransient = false;

        output.terrainDrawArgsBuffer = fg.ImportBuffer("gpu_cull_terrain_drawargs", m_terrainDrawArgsBuffer, terrainArgsDesc);

        // Note: Terrain compaction could be added later for optimization
        // For now, use simple per-batch indirect draw
        output.terrainCompactDrawArgs = VirtualResourceHandle();
        output.terrainCompactBatchIndices = VirtualResourceHandle();
    } else {
        output.terrainDrawArgsBuffer = VirtualResourceHandle();
        output.terrainCompactDrawArgs = VirtualResourceHandle();
        output.terrainCompactBatchIndices = VirtualResourceHandle();
    }

    return output;
}

// ═══════════════════════════════════════════════════════
//  DEBUG VISUALIZATION
// ═══════════════════════════════════════════════════════

bool GPUCullingManager::IsDebugEnabled() const
{
    return ps_r4_debug_gpu_culling != 0 && m_computeEnabled && m_debugComputePipeline && m_debugGraphicsPipeline;
}

void GPUCullingManager::CreateDebugResources(ng::RenderDevice* device)
{
    if (!m_computeEnabled)
        return;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();

    s_object_cull_debug_cs.create("object_cull_debug");
    s_particle_cull_debug_cs.create("particle_cull_debug");
    s_cull_debug_vs = RImplementation.Resources->_CreateVS("cull_debug");
    s_cull_debug_ps = RImplementation.Resources->_CreatePS("cull_debug");

    if (!s_object_cull_debug_cs || !s_object_cull_debug_cs->nvrhiShader) {
        Msg("! [GPUCulling] object_cull_debug.cs not found - debug visualization disabled");
        return;
    }
    if (!s_cull_debug_vs || !s_cull_debug_vs->nvrhiShader) {
        Msg("! [GPUCulling] cull_debug.vs not found - debug visualization disabled");
        return;
    }
    if (!s_cull_debug_ps || !s_cull_debug_ps->nvrhiShader) {
        Msg("! [GPUCulling] cull_debug.ps not found - debug visualization disabled");
        return;
    }
    if (!s_particle_cull_debug_cs || !s_particle_cull_debug_cs->nvrhiShader) {
        Msg("* [GPUCulling] particle_cull_debug.cs not found - particle debug disabled");
    }

    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_DebugData";
        desc.byteSize = (m_maxObjects + m_maxParticles) * sizeof(CullDebugData);
        desc.structStride = sizeof(CullDebugData);
        desc.canHaveUAVs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;  // Let NVRHI handle state transitions

        m_debugBuffer = nvDevice->createBuffer(desc);
        if (!m_debugBuffer) {
            Msg("! [GPUCulling] Failed to create debug buffer");
            return;
        }
    }

    // ─────────────────────────────────────────────────────
    //  DEBUG CONSTANT BUFFERS (separate for compute and graphics)
    // ─────────────────────────────────────────────────────
    {
        // Compute shader constant buffer
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_DebugComputeParams";
        desc.byteSize = sizeof(CullDebugParamsCB);
        desc.isConstantBuffer = true;
        desc.isVolatile = true;
        desc.maxVersions = 16;

        m_debugComputeParamsCB = nvDevice->createBuffer(desc);
        if (!m_debugComputeParamsCB) {
            Msg("! [GPUCulling] Failed to create debug compute constant buffer");
            return;
        }
    }
    {
        // Graphics shader constant buffer
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_DebugGraphicsParams";
        desc.byteSize = sizeof(CullDebugVSParamsCB);
        desc.isConstantBuffer = true;
        desc.isVolatile = true;
        desc.maxVersions = 16;

        m_debugGraphicsParamsCB = nvDevice->createBuffer(desc);
        if (!m_debugGraphicsParamsCB) {
            Msg("! [GPUCulling] Failed to create debug graphics constant buffer");
            return;
        }
    }

    // ─────────────────────────────────────────────────────
    //  DEBUG COMPUTE PIPELINE (object_cull_debug.cs)
    // ─────────────────────────────────────────────────────
    {
        // Binding layout for debug compute
        // b5: CullDebugParams
        // t0: g_Objects (structured buffer SRV)
        // t1: g_HiZPyramid (texture SRV)
        // s0: g_PointSampler
        // u0: g_DebugOutput (structured buffer UAV)
        nvrhi::BindingLayoutDesc layoutDesc;
        layoutDesc.visibility = nvrhi::ShaderType::Compute;
        layoutDesc.bindings = {
            nvrhi::BindingLayoutItem::VolatileConstantBuffer(5),  // volatile CB
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(0),
            nvrhi::BindingLayoutItem::Texture_SRV(1),
            nvrhi::BindingLayoutItem::Sampler(0),
            nvrhi::BindingLayoutItem::StructuredBuffer_UAV(0)
        };

        m_debugComputeLayout = nvDevice->createBindingLayout(layoutDesc);
        if (!m_debugComputeLayout) {
            Msg("! [GPUCulling] Failed to create debug compute binding layout");
            return;
        }

        nvrhi::ComputePipelineDesc pipeDesc;
        pipeDesc.CS = s_object_cull_debug_cs->nvrhiShader;
        pipeDesc.bindingLayouts = { m_debugComputeLayout };

        m_debugComputePipeline = nvDevice->createComputePipeline(pipeDesc);
        if (!m_debugComputePipeline) {
            Msg("! [GPUCulling] Failed to create debug compute pipeline");
            return;
        }

        if (s_particle_cull_debug_cs && s_particle_cull_debug_cs->nvrhiShader) {
            nvrhi::ComputePipelineDesc particlePipeDesc;
            particlePipeDesc.CS = s_particle_cull_debug_cs->nvrhiShader;
            particlePipeDesc.bindingLayouts = { m_debugComputeLayout };
            m_particleDebugComputePipeline = nvDevice->createComputePipeline(particlePipeDesc);
        }
    }

    // ─────────────────────────────────────────────────────
    //  DEBUG GRAPHICS PIPELINE (cull_debug.vs + cull_debug.ps)
    // ─────────────────────────────────────────────────────
    {
        // Binding layout for debug graphics
        // b5: CullDebugVSParams (constant buffer)
        // t0: g_DebugData (structured buffer SRV)
        nvrhi::BindingLayoutDesc layoutDesc;
        layoutDesc.visibility = nvrhi::ShaderType::Vertex | nvrhi::ShaderType::Pixel;
        layoutDesc.bindings = {
            nvrhi::BindingLayoutItem::VolatileConstantBuffer(5),  // volatile CB
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(0)
        };

        m_debugGraphicsLayout = nvDevice->createBindingLayout(layoutDesc);
        if (!m_debugGraphicsLayout) {
            Msg("! [GPUCulling] Failed to create debug graphics binding layout");
            return;
        }

        // No input layout needed - VS generates vertices from SV_VertexID/SV_InstanceID
        nvrhi::GraphicsPipelineDesc pipeDesc;
        pipeDesc.VS = s_cull_debug_vs->nvrhiShader;
        pipeDesc.PS = s_cull_debug_ps->nvrhiShader;
        pipeDesc.bindingLayouts = { m_debugGraphicsLayout };
        pipeDesc.primType = nvrhi::PrimitiveType::TriangleStrip;
        pipeDesc.inputLayout = nullptr;  // No vertex input - generated in shader

        // Render state: alpha blending, no depth write, depth test enabled
        pipeDesc.renderState.blendState.targets[0].setBlendEnable(true);
        pipeDesc.renderState.blendState.targets[0].setSrcBlend(nvrhi::BlendFactor::SrcAlpha);
        pipeDesc.renderState.blendState.targets[0].setDestBlend(nvrhi::BlendFactor::InvSrcAlpha);
        pipeDesc.renderState.blendState.targets[0].setBlendOp(nvrhi::BlendOp::Add);
        pipeDesc.renderState.blendState.targets[0].setSrcBlendAlpha(nvrhi::BlendFactor::One);
        pipeDesc.renderState.blendState.targets[0].setDestBlendAlpha(nvrhi::BlendFactor::Zero);
        pipeDesc.renderState.blendState.targets[0].setBlendOpAlpha(nvrhi::BlendOp::Add);

        pipeDesc.renderState.depthStencilState.setDepthTestEnable(false);  // Always render on top
        pipeDesc.renderState.depthStencilState.setDepthWriteEnable(false);  // Don't write depth

        pipeDesc.renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);  // No culling for billboards

        // Must provide framebuffer info for pipeline creation
        nvrhi::FramebufferInfoEx framebufferInfo;
        framebufferInfo.addColorFormat(nvrhi::Format::RGBA16_FLOAT);  // HDR color target
        framebufferInfo.setDepthFormat(nvrhi::Format::D32);           // Depth buffer

        m_debugGraphicsPipeline = nvDevice->createGraphicsPipeline(pipeDesc, framebufferInfo);
        if (!m_debugGraphicsPipeline) {
            Msg("! [GPUCulling] Failed to create debug graphics pipeline");
            return;
        }
    }

    Msg("* [GPUCulling] Debug visualization resources created");
}

void GPUCullingManager::SetupDebugVisualizationPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle hizPyramid,
    framegraph::VirtualResourceHandle colorTarget,
    framegraph::VirtualResourceHandle depthTarget,
    u32 hizWidth,
    u32 hizHeight,
    u32 hizMipLevels,
    const xr_vector<passes::ParticleBatch>* particleBatches)
{
    using namespace framegraph;

    u32 particleCount = particleBatches ? std::min(static_cast<u32>(particleBatches->size()), m_maxParticles) : 0;

    if (!IsDebugEnabled() || (m_objectCount == 0 && particleCount == 0))
        return;

    struct DebugPassData {
        VirtualResourceHandle hizPyramid;
        VirtualResourceHandle colorTarget;
        VirtualResourceHandle depthTarget;

        GPUCullingManager* manager;
        const xr_vector<passes::ParticleBatch>* particleBatches;
        u32 objectCount;
        u32 particleCount;
        u32 hizWidth;
        u32 hizHeight;
        u32 hizMipLevels;
    };

    fg.addCallbackPass<DebugPassData>(
        "GPU Culling Debug",

        [&, hizWidth, hizHeight, hizMipLevels, particleCount, particleBatches](FrameGraph& builder, PassHandle passHandle, DebugPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.manager = this;
            data.particleBatches = particleBatches;
            data.objectCount = m_objectCount;
            data.particleCount = particleCount;
            data.hizWidth = hizWidth;
            data.hizHeight = hizHeight;
            data.hizMipLevels = hizMipLevels;

            data.hizPyramid = passBuilder.read(hizPyramid, ResourceState::ShaderResource);
            data.colorTarget = passBuilder.write(colorTarget, ResourceState::RenderTarget);
            data.depthTarget = passBuilder.read(depthTarget, ResourceState::DepthStencilRead);
        },

        // Execute lambda
        [](const DebugPassData& data,
           const FrameGraph& fg,
           ng::RenderContext* ctx) {

            GPUCullingManager* mgr = data.manager;
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            nvrhi::IDevice* nvDevice = mgr->m_device->GetNVRHIDevice();

            // Get physical resources
            nvrhi::ITexture* hizTexture = fg.GetPhysicalTexture(data.hizPyramid);
            nvrhi::ITexture* colorTexture = fg.GetPhysicalTexture(data.colorTarget);
            nvrhi::ITexture* depthTexture = fg.GetPhysicalTexture(data.depthTarget);

            if (!hizTexture || !colorTexture || !depthTexture) {
                Msg("! [GPUCulling] Debug pass missing textures");
                return;
            }

            float farPlane = g_pGamePersistent ? g_pGamePersistent->Environment().CurrentEnv.far_plane : 300.0f;

            if (data.objectCount > 0) {
                CullDebugParamsCB cb;
                cb.viewProj.transpose(Device.mFullTransform);
                cb.cameraPos = Device.vCameraPosition;
                cb.maxDistanceSq = farPlane * farPlane;
                cb.objectCount = data.objectCount;
                cb.hizWidth = data.hizWidth;
                cb.hizHeight = data.hizHeight;
                cb.hizMipLevels = data.hizMipLevels;
                cb.occluderThreshold = 50.0f;
                cb.debugOffset = 0;

                mgr->ExtractFrustumPlanes(Device.mFullTransform, cb.frustumPlanes);
                cmdList->writeBuffer(mgr->m_debugComputeParamsCB, &cb, sizeof(cb));

                nvrhi::BindingSetDesc bindDesc;
                bindDesc.bindings = {
                    nvrhi::BindingSetItem::ConstantBuffer(5, mgr->m_debugComputeParamsCB),
                    nvrhi::BindingSetItem::StructuredBuffer_SRV(0, mgr->m_objectBuffer),
                    nvrhi::BindingSetItem::Texture_SRV(1, hizTexture),
                    nvrhi::BindingSetItem::Sampler(0, mgr->m_pointSampler),
                    nvrhi::BindingSetItem::StructuredBuffer_UAV(0, mgr->m_debugBuffer)
                };

                nvrhi::BindingSetHandle bindingSet = nvDevice->createBindingSet(bindDesc, mgr->m_debugComputeLayout);

                nvrhi::ComputeState state;
                state.pipeline = mgr->m_debugComputePipeline;
                state.bindings = { bindingSet };
                cmdList->setComputeState(state);

                u32 groupCount = (data.objectCount + CULL_THREAD_GROUP_SIZE - 1) / CULL_THREAD_GROUP_SIZE;
                cmdList->dispatch(groupCount, 1, 1);
            }

            if (data.particleCount > 0 && data.particleBatches && mgr->m_particleDebugComputePipeline) {
                mgr->m_particleData.clear();
                mgr->m_particleData.reserve(data.particleCount);

                for (u32 i = 0; i < data.particleCount; i++) {
                    const auto& batch = (*data.particleBatches)[i];
                    if (!batch.visual) continue;

                    GPUParticleData particle;
                    particle.position = batch.visual->vis.sphere.P;
                    particle.radius = batch.visual->vis.sphere.R;
                    particle.batchIndex = i;
                    particle.flags = 0;
                    particle.pad0 = 0.0f;
                    particle.pad1 = 0.0f;
                    mgr->m_particleData.push_back(particle);
                }

                if (!mgr->m_particleData.empty()) {
                    cmdList->writeBuffer(mgr->m_particleBuffer, mgr->m_particleData.data(),
                                         mgr->m_particleData.size() * sizeof(GPUParticleData));

                    CullDebugParamsCB cb;
                    cb.viewProj.transpose(Device.mFullTransform);
                    cb.cameraPos = Device.vCameraPosition;
                    cb.maxDistanceSq = farPlane * farPlane;
                    cb.objectCount = static_cast<u32>(mgr->m_particleData.size());
                    cb.hizWidth = data.hizWidth;
                    cb.hizHeight = data.hizHeight;
                    cb.hizMipLevels = data.hizMipLevels;
                    cb.occluderThreshold = 50.0f;
                    cb.debugOffset = data.objectCount;

                    mgr->ExtractFrustumPlanes(Device.mFullTransform, cb.frustumPlanes);
                    cmdList->writeBuffer(mgr->m_debugComputeParamsCB, &cb, sizeof(cb));

                    nvrhi::BindingSetDesc bindDesc;
                    bindDesc.bindings = {
                        nvrhi::BindingSetItem::ConstantBuffer(5, mgr->m_debugComputeParamsCB),
                        nvrhi::BindingSetItem::StructuredBuffer_SRV(0, mgr->m_particleBuffer),
                        nvrhi::BindingSetItem::Texture_SRV(1, hizTexture),
                        nvrhi::BindingSetItem::Sampler(0, mgr->m_pointSampler),
                        nvrhi::BindingSetItem::StructuredBuffer_UAV(0, mgr->m_debugBuffer)
                    };

                    nvrhi::BindingSetHandle bindingSet = nvDevice->createBindingSet(bindDesc, mgr->m_debugComputeLayout);

                    nvrhi::ComputeState state;
                    state.pipeline = mgr->m_particleDebugComputePipeline;
                    state.bindings = { bindingSet };
                    cmdList->setComputeState(state);

                    u32 groupCount = (static_cast<u32>(mgr->m_particleData.size()) + CULL_THREAD_GROUP_SIZE - 1) / CULL_THREAD_GROUP_SIZE;
                    cmdList->dispatch(groupCount, 1, 1);
                }
            }

            cmdList->setBufferState(mgr->m_debugBuffer, nvrhi::ResourceStates::ShaderResource);

            u32 totalDebugCount = data.objectCount + data.particleCount;
            if (totalDebugCount > 0) {
                CullDebugVSParamsCB vsCB;
                vsCB.view.transpose(Device.mView);
                vsCB.viewProj.transpose(Device.mFullTransform);
                vsCB.objectCount = totalDebugCount;
                vsCB.wireframeAlpha = 0.7f;

                cmdList->writeBuffer(mgr->m_debugGraphicsParamsCB, &vsCB, sizeof(vsCB));

                nvrhi::BindingSetDesc bindDesc;
                bindDesc.bindings = {
                    nvrhi::BindingSetItem::ConstantBuffer(5, mgr->m_debugGraphicsParamsCB),
                    nvrhi::BindingSetItem::StructuredBuffer_SRV(0, mgr->m_debugBuffer)
                };

                nvrhi::BindingSetHandle bindingSet = nvDevice->createBindingSet(bindDesc, mgr->m_debugGraphicsLayout);

                nvrhi::FramebufferDesc fbDesc;
                fbDesc.addColorAttachment(colorTexture);
                fbDesc.setDepthAttachment(depthTexture);
                nvrhi::FramebufferHandle framebuffer = nvDevice->createFramebuffer(fbDesc);

                nvrhi::GraphicsState gfxState;
                gfxState.pipeline = mgr->m_debugGraphicsPipeline;
                gfxState.bindings = { bindingSet };
                gfxState.framebuffer = framebuffer;

                nvrhi::Viewport viewport;
                viewport.minX = 0;
                viewport.minY = 0;
                viewport.maxX = static_cast<float>(colorTexture->getDesc().width);
                viewport.maxY = static_cast<float>(colorTexture->getDesc().height);
                viewport.minZ = 0.0f;
                viewport.maxZ = 1.0f;
                gfxState.viewport.addViewportAndScissorRect(viewport);

                cmdList->setGraphicsState(gfxState);

                nvrhi::DrawArguments drawArgs;
                drawArgs.vertexCount = 4;
                drawArgs.instanceCount = totalDebugCount;
                drawArgs.startVertexLocation = 0;
                drawArgs.startInstanceLocation = 0;
                cmdList->draw(drawArgs);
            }

            cmdList->setBufferState(mgr->m_debugBuffer, nvrhi::ResourceStates::UnorderedAccess);
        }
    );
}

void GPUCullingManager::CreateParticleResources(ng::RenderDevice* device)
{
    if (!m_computeEnabled)
        return;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();

    s_particle_cull_cs.create("particle_cull");
    if (!s_particle_cull_cs || !s_particle_cull_cs->nvrhiShader) {
        Msg("! [GPUCulling] particle_cull.cs not found - particle culling disabled");
        return;
    }

    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_Particles";
        desc.byteSize = m_maxParticles * sizeof(GPUParticleData);
        desc.structStride = sizeof(GPUParticleData);
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;

        m_particleBuffer = nvDevice->createBuffer(desc);
        if (!m_particleBuffer) {
            Msg("! [GPUCulling] Failed to create particle buffer");
            return;
        }
    }

    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_ParticleDrawArgs";
        desc.byteSize = m_maxParticles * sizeof(IndirectDrawArgs);
        desc.canHaveUAVs = true;
        desc.canHaveRawViews = true;
        desc.isDrawIndirectArgs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;  // Let NVRHI handle state transitions

        m_particleDrawArgsBuffer = nvDevice->createBuffer(desc);
        if (!m_particleDrawArgsBuffer) {
            Msg("! [GPUCulling] Failed to create particle draw args buffer");
            return;
        }
    }

    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_ParticleVisibleCount";
        desc.byteSize = sizeof(u32);
        desc.canHaveUAVs = true;
        desc.canHaveRawViews = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;

        m_particleVisibleCountBuffer = nvDevice->createBuffer(desc);
        if (!m_particleVisibleCountBuffer) {
            Msg("! [GPUCulling] Failed to create particle visible count buffer");
            return;
        }
    }

    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_ParticleParams";
        desc.byteSize = sizeof(CullParamsCB);
        desc.isConstantBuffer = true;
        desc.isVolatile = true;
        desc.maxVersions = 16;

        m_particleCullParamsCB = nvDevice->createBuffer(desc);
        if (!m_particleCullParamsCB) {
            Msg("! [GPUCulling] Failed to create particle constant buffer");
            return;
        }
    }

    {
        nvrhi::BindingLayoutDesc layoutDesc;
        layoutDesc.visibility = nvrhi::ShaderType::Compute;
        layoutDesc.bindings = {
            nvrhi::BindingLayoutItem::VolatileConstantBuffer(5),  // volatile CB
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(0),
            nvrhi::BindingLayoutItem::Texture_SRV(1),
            nvrhi::BindingLayoutItem::Sampler(0),
            nvrhi::BindingLayoutItem::RawBuffer_UAV(0),
            nvrhi::BindingLayoutItem::RawBuffer_UAV(1)
        };

        m_particleCullLayout = nvDevice->createBindingLayout(layoutDesc);
        if (!m_particleCullLayout) {
            Msg("! [GPUCulling] Failed to create particle binding layout");
            return;
        }
    }

    {
        nvrhi::ComputePipelineDesc pipeDesc;
        pipeDesc.CS = s_particle_cull_cs->nvrhiShader;
        pipeDesc.bindingLayouts = { m_particleCullLayout };

        Msg("* [GPUCulling] Creating particle compute pipeline:");
        Msg("    CS shader: %p", pipeDesc.CS.Get());
        Msg("    Binding layouts: %u", pipeDesc.bindingLayouts.size());
        Msg("    Layout[0]: %p", pipeDesc.bindingLayouts[0].Get());

        m_particleCullPipeline = nvDevice->createComputePipeline(pipeDesc);
        if (!m_particleCullPipeline) {
            Msg("! [GPUCulling] Failed to create particle compute pipeline");
            Msg("! [GPUCulling]   Check NVRHI validation layer output above for details");
            return;
        }
    }

    m_particleCullEnabled = true;
    Msg("* [GPUCulling] Particle culling resources created");
}

void GPUCullingManager::UploadParticleBatches(ng::RenderContext* ctx, const xr_vector<passes::ParticleBatch>* batches)
{
    if (!m_particleCullEnabled || !batches)
        return;

    m_particleCount = std::min(static_cast<u32>(batches->size()), m_maxParticles);

    if (m_particleCount == 0)
        return;

    m_particleData.clear();
    m_particleData.reserve(m_particleCount);
    m_particleDrawArgsData.clear();
    m_particleDrawArgsData.reserve(m_particleCount);

    for (u32 i = 0; i < m_particleCount; i++) {
        const auto& batch = (*batches)[i];

        if (!batch.visual)
            continue;

        GPUParticleData particle;
        particle.position = batch.visual->vis.sphere.P;
        particle.radius = batch.visual->vis.sphere.R;
        particle.batchIndex = i;
        particle.flags = 0;
        particle.pad0 = 0.0f;
        particle.pad1 = 0.0f;

        m_particleData.push_back(particle);

        IndirectDrawArgs args;
        args.indexCountPerInstance = batch.particleCount * 6;
        args.instanceCount = 0;
        args.startIndexLocation = 0;
        args.baseVertexLocation = 0;
        args.startInstanceLocation = 0;

        m_particleDrawArgsData.push_back(args);
    }

    nvrhi::ICommandList* cmdList = ctx->GetCommandList();
    cmdList->writeBuffer(m_particleBuffer, m_particleData.data(), m_particleCount * sizeof(GPUParticleData));
    cmdList->writeBuffer(m_particleDrawArgsBuffer, m_particleDrawArgsData.data(), m_particleCount * sizeof(IndirectDrawArgs));
}

GPUParticleCullOutput GPUCullingManager::SetupParticleCullingPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle hizPyramid,
    u32 hizWidth,
    u32 hizHeight,
    u32 hizMipLevels,
    const xr_vector<passes::ParticleBatch>* batches)
{
    using namespace framegraph;

    GPUParticleCullOutput output;
    output.maxParticles = m_maxParticles;

    if (!m_particleCullEnabled) {
        output.drawArgsBuffer = VirtualResourceHandle();
        return output;
    }

    m_particleCount = batches ? std::min(static_cast<u32>(batches->size()), m_maxParticles) : 0;

    if (m_particleCount == 0) {
        output.drawArgsBuffer = VirtualResourceHandle();
        return output;
    }

    struct ParticleCullPassData {
        VirtualResourceHandle hizPyramid;
        VirtualResourceHandle drawArgsBuffer;

        GPUCullingManager* manager;
        const xr_vector<passes::ParticleBatch>* batches;
        u32 particleCount;
        u32 hizWidth;
        u32 hizHeight;
        u32 hizMipLevels;
    };

    ResourceDesc drawArgsDesc;
    drawArgsDesc.type = ResourceDesc::Type::Buffer;
    drawArgsDesc.debugName = "GPUCull_ParticleDrawArgs";
    drawArgsDesc.bufferSize = m_maxParticles * sizeof(IndirectDrawArgs);
    drawArgsDesc.structStride = sizeof(IndirectDrawArgs);
    drawArgsDesc.isUAV = true;
    drawArgsDesc.isTransient = false;

    VirtualResourceHandle drawArgsHandle = fg.ImportBuffer("gpu_cull_particle_drawargs", m_particleDrawArgsBuffer, drawArgsDesc);

    auto& passData = fg.addCallbackPass<ParticleCullPassData>(
        "GPU Particle Culling",

        [&, hizWidth, hizHeight, hizMipLevels, drawArgsHandle, batches](FrameGraph& builder, PassHandle passHandle, ParticleCullPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.manager = this;
            data.batches = batches;
            data.particleCount = m_particleCount;
            data.hizWidth = hizWidth;
            data.hizHeight = hizHeight;
            data.hizMipLevels = hizMipLevels;

            data.hizPyramid = passBuilder.read(hizPyramid, ResourceState::ShaderResource);
            data.drawArgsBuffer = passBuilder.write(drawArgsHandle, ResourceState::UnorderedAccess);
        },

        [](const ParticleCullPassData& data,
           const FrameGraph& fg,
           ng::RenderContext* ctx) {

            GPUCullingManager* mgr = data.manager;
            if (!mgr->m_particleCullEnabled || data.particleCount == 0)
                return;

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            nvrhi::IDevice* nvDevice = mgr->m_device->GetNVRHIDevice();

            mgr->UploadParticleBatches(ctx, data.batches);

            nvrhi::ITexture* hizTexture = fg.GetPhysicalTexture(data.hizPyramid);
            if (!hizTexture) {
                Msg("! [GPUCulling] Hi-Z texture not available for particle culling");
                return;
            }

            u32 zero = 0;
            cmdList->writeBuffer(mgr->m_particleVisibleCountBuffer, &zero, sizeof(u32));

            CullParamsCB cb;
            cb.viewProj.transpose(Device.mFullTransform);
            cb.cameraPos = Device.vCameraPosition;
            float farPlane = g_pGamePersistent ? g_pGamePersistent->Environment().CurrentEnv.far_plane : 300.0f;
            cb.maxDistanceSq = farPlane * farPlane;
            cb.objectCount = data.particleCount;
            cb.hizWidth = data.hizWidth;
            cb.hizHeight = data.hizHeight;
            cb.hizMipLevels = data.hizMipLevels;

            mgr->ExtractFrustumPlanes(Device.mFullTransform, cb.frustumPlanes);

            cmdList->writeBuffer(mgr->m_particleCullParamsCB, &cb, sizeof(cb));

            nvrhi::BindingSetDesc bindDesc;
            bindDesc.bindings = {
                nvrhi::BindingSetItem::ConstantBuffer(5, mgr->m_particleCullParamsCB),
                nvrhi::BindingSetItem::StructuredBuffer_SRV(0, mgr->m_particleBuffer),
                nvrhi::BindingSetItem::Texture_SRV(1, hizTexture),
                nvrhi::BindingSetItem::Sampler(0, mgr->m_pointSampler),
                nvrhi::BindingSetItem::RawBuffer_UAV(0, mgr->m_particleVisibleCountBuffer),
                nvrhi::BindingSetItem::RawBuffer_UAV(1, mgr->m_particleDrawArgsBuffer)
            };

            nvrhi::BindingSetHandle bindingSet = nvDevice->createBindingSet(bindDesc, mgr->m_particleCullLayout);
            if (!bindingSet) {
                Msg("! [GPUCulling] Failed to create particle binding set");
                return;
            }

            nvrhi::ComputeState state;
            state.pipeline = mgr->m_particleCullPipeline;
            state.bindings = { bindingSet };
            cmdList->setComputeState(state);

            u32 groupCount = (data.particleCount + CULL_THREAD_GROUP_SIZE - 1) / CULL_THREAD_GROUP_SIZE;
            cmdList->dispatch(groupCount, 1, 1);
        }
    );

    output.drawArgsBuffer = passData.drawArgsBuffer;
    return output;
}

// ═══════════════════════════════════════════════════════
//  MEGA-BUFFER SYSTEM IMPLEMENTATION
// ═══════════════════════════════════════════════════════

void GPUCullingManager::BeginLevelLoad(u32 estimatedVertices, u32 estimatedIndices)
{
    if (m_levelLoadInProgress) {
        Msg("! [GPUCulling] BeginLevelLoad called while already in progress");
        return;
    }

    m_levelLoadInProgress = true;
    m_megaBuffersReady = false;

    // Clear and pre-allocate mega-buffers
    m_megaVertices.clear();
    m_megaVertices.reserve(estimatedVertices);
    m_megaIndices.clear();
    m_megaIndices.reserve(estimatedIndices);

    // Clear VB/IB pool tracking
    m_vbPools.clear();
    m_ibPools.clear();
    m_vbPoolsAlt.clear();
    m_ibPoolsAlt.clear();

    m_totalVertexCount = 0;
    m_totalIndexCount = 0;

    Msg("* [GPUCulling] BeginLevelLoad - estimated %u vertices, %u indices", estimatedVertices, estimatedIndices);
}

MeshAllocation GPUCullingManager::RegisterMesh(
    const void* vertices,
    u32 vertexCount,
    u32 vertexStride,
    bindless::SourceVertexFormat format,
    const u16* indices,
    u32 indexCount)
{
    MeshAllocation alloc;

    if (!m_levelLoadInProgress) {
        Msg("! [GPUCulling] RegisterMesh called outside of level load");
        return alloc;
    }

    if (!vertices || vertexCount == 0 || !indices || indexCount == 0) {
        return alloc;
    }

    if (format == bindless::SourceVertexFormat::Unknown) {
        Msg("! [GPUCulling] RegisterMesh: Unknown vertex format (stride=%u)", vertexStride);
        return alloc;
    }

    // Record offsets before adding
    alloc.vertexOffset = m_totalVertexCount;
    alloc.indexOffset = m_totalIndexCount;
    alloc.vertexCount = vertexCount;
    alloc.indexCount = indexCount;

    // Convert vertices to unified format
    u32 prevSize = static_cast<u32>(m_megaVertices.size());
    m_megaVertices.resize(prevSize + vertexCount);

    u32 converted = bindless::VertexConverter::ConvertVertices(
        vertices, vertexStride, vertexCount, format,
        &m_megaVertices[prevSize]
    );

    if (converted != vertexCount) {
        Msg("! [GPUCulling] RegisterMesh: Vertex conversion failed (got %u, expected %u)", converted, vertexCount);
        m_megaVertices.resize(prevSize);  // Rollback
        return alloc;
    }

    // Copy indices, converting from 16-bit to 32-bit and adjusting for vertex offset
    u32 prevIndexSize = static_cast<u32>(m_megaIndices.size());
    m_megaIndices.resize(prevIndexSize + indexCount);

    for (u32 i = 0; i < indexCount; i++) {
        // Note: We store raw indices without adding vertexOffset here
        // The offset will be handled via baseVertexLocation in draw args
        m_megaIndices[prevIndexSize + i] = static_cast<u32>(indices[i]);
    }

    m_totalVertexCount += vertexCount;
    m_totalIndexCount += indexCount;
    alloc.valid = true;

    return alloc;
}

void GPUCullingManager::CreateMegaBuffers()
{
    if (!m_device) {
        Msg("! [GPUCulling] CreateMegaBuffers: No device");
        return;
    }

    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();

    // Create vertex buffer
    // NOTE: Vertex buffers cannot use structStride (structured buffer) - D3D11 restriction
    if (m_totalVertexCount > 0) {
        nvrhi::BufferDesc desc;
        desc.debugName = "MegaVertexBuffer";
        desc.byteSize = m_totalVertexCount * sizeof(bindless::UnifiedVertex);
        desc.isVertexBuffer = true;  // Required for D3D11 vertex buffer binding
        desc.initialState = nvrhi::ResourceStates::VertexBuffer;
        desc.keepInitialState = true;

        m_megaVertexBuffer = nvDevice->createBuffer(desc);
        if (!m_megaVertexBuffer) {
            Msg("! [GPUCulling] Failed to create mega vertex buffer (%u vertices, %zu bytes)",
                m_totalVertexCount, desc.byteSize);
            return;
        }

        m_maxMegaVertices = m_totalVertexCount;
    }

    // Create index buffer (32-bit indices)
    if (m_totalIndexCount > 0) {
        nvrhi::BufferDesc desc;
        desc.debugName = "MegaIndexBuffer";
        desc.byteSize = m_totalIndexCount * sizeof(u32);
        desc.initialState = nvrhi::ResourceStates::IndexBuffer;
        desc.keepInitialState = true;
        desc.isIndexBuffer = true;

        m_megaIndexBuffer = nvDevice->createBuffer(desc);
        if (!m_megaIndexBuffer) {
            Msg("! [GPUCulling] Failed to create mega index buffer (%u indices, %zu bytes)",
                m_totalIndexCount, desc.byteSize);
            return;
        }

        m_maxMegaIndices = m_totalIndexCount;
    }

    // Create instance buffer (sized for max objects)
    {
        nvrhi::BufferDesc desc;
        desc.debugName = "InstanceBuffer";
        desc.byteSize = m_maxObjects * sizeof(GPUInstanceData);
        desc.structStride = sizeof(GPUInstanceData);
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;

        m_instanceBuffer = nvDevice->createBuffer(desc);
        if (!m_instanceBuffer) {
            Msg("! [GPUCulling] Failed to create instance buffer");
            return;
        }
    }

    Msg("* [GPUCulling] Mega-buffers created: VB=%u verts (%.1f MB), IB=%u indices (%.1f MB)",
        m_totalVertexCount,
        (m_totalVertexCount * sizeof(bindless::UnifiedVertex)) / (1024.0f * 1024.0f),
        m_totalIndexCount,
        (m_totalIndexCount * sizeof(u32)) / (1024.0f * 1024.0f));
}

void GPUCullingManager::EndLevelLoad()
{
    if (!m_levelLoadInProgress) {
        Msg("! [GPUCulling] EndLevelLoad called without BeginLevelLoad");
        return;
    }

    Msg("* [GPUCulling] EndLevelLoad - collected %u vertices, %u indices",
        m_totalVertexCount, m_totalIndexCount);

    // Create GPU buffers
    CreateMegaBuffers();

    if (!m_megaVertexBuffer || !m_megaIndexBuffer) {
        Msg("! [GPUCulling] EndLevelLoad: Failed to create mega-buffers");
        m_levelLoadInProgress = false;
        return;
    }

    // Upload data to GPU (need a context - will be done on first frame)
    // Mark as ready - upload will happen in UploadSceneObjects on first frame
    m_megaBuffersReady = true;
    m_levelLoadInProgress = false;

    Msg("* [GPUCulling] Mega-buffers ready for GPU upload");
}

void GPUCullingManager::UploadInstanceData(ng::RenderContext* ctx, const GeometryCollector* geometry)
{
    // NOTE: Mega-buffer upload is now handled in UploadSceneObjects()
    // This function is reserved for future per-frame instance data (transforms for instancing)
    if (!m_instanceBuffer || !geometry)
        return;

    const auto& batches = geometry->GetBatches();
    u32 batchCount = static_cast<u32>(batches.size());

    if (batchCount == 0)
        return;

    m_instanceData.clear();
    m_instanceData.reserve(batchCount);

    for (u32 i = 0; i < batchCount; i++) {
        const auto& batch = batches[i];

        // Skip skinned batches - they use a separate per-draw rendering path
        // with bone matrices and cannot use the GPU-driven multi-draw system
        if (batch.isSkinned)
            continue;

        // Skip terrain batches - they use separate terrain buffers/shader
        // CRITICAL: Must match filtering in UploadSceneObjects() or indices won't align
        if (batch.isTerrain)
            continue;

        GPUInstanceData inst;
        // Transpose for HLSL row-major convention
        inst.world.transpose(batch.worldMatrix);
        inst.materialID = batch.bindlessMaterialID;
        inst.flags = 0;
        if (batch.IsOpaque()) inst.flags |= GPU_OBJECT_OPAQUE;
        if (batch.IsAlphaTested()) inst.flags |= GPU_OBJECT_ALPHA_TEST;
        if (batch.IsStrictB2F()) inst.flags |= GPU_OBJECT_TRANSPARENT;
        inst.pad0 = 0.0f;
        inst.pad1 = 0.0f;

        m_instanceData.push_back(inst);
    }

    u32 instanceCount = std::min(static_cast<u32>(m_instanceData.size()), m_maxObjects);
    if (instanceCount == 0)
        return;

    nvrhi::ICommandList* cmdList = ctx->GetCommandList();
    cmdList->writeBuffer(m_instanceBuffer, m_instanceData.data(), instanceCount * sizeof(GPUInstanceData));
}

// ═══════════════════════════════════════════════════════
//  VB/IB POOL REGISTRATION (for level geometry)
// ═══════════════════════════════════════════════════════

bindless::SourceVertexFormat GPUCullingManager::DetectFormatFromDecl(
    const D3DVERTEXELEMENT9* decl,
    u32 stride)
{
    if (!decl)
        return bindless::SourceVertexFormat::Unknown;

    // Analyze declaration to determine format
    // Key differentiators:
    // - r1_decl_lmap: TEXCOORD 0 (SHORT2) + TEXCOORD 1 (SHORT2) for lightmap
    // - r1_decl_vert: COLOR + TEXCOORD 0 (SHORT2), no lightmap
    // - mu_model_decl: TEXCOORD 0 (SHORT4), no COLOR
    // - x_decl_vert: position only (12 bytes)

    bool hasColor = false;
    bool hasTexCoord1 = false;
    bool hasShort4TexCoord = false;
    bool hasFloat2TexCoord = false;
    u32 texcoord0Type = 0;

    for (int i = 0; decl[i].Stream != 0xFF; i++) {
        const D3DVERTEXELEMENT9& elem = decl[i];

        if (elem.Usage == D3DDECLUSAGE_COLOR && elem.UsageIndex == 0) {
            hasColor = true;
        }
        else if (elem.Usage == D3DDECLUSAGE_TEXCOORD) {
            if (elem.UsageIndex == 0) {
                texcoord0Type = elem.Type;
                if (elem.Type == D3DDECLTYPE_SHORT4) hasShort4TexCoord = true;
                if (elem.Type == D3DDECLTYPE_FLOAT2) hasFloat2TexCoord = true;
            }
            else if (elem.UsageIndex == 1) {
                hasTexCoord1 = true;
            }
        }
    }

    // Match against known formats
    if (stride == 12) {
        return bindless::SourceVertexFormat::X_Vert;
    }
    else if (stride == 28) {
        // Unpacked formats
        if (hasColor) return bindless::SourceVertexFormat::R1_Vert_Unpacked;
        return bindless::SourceVertexFormat::MU_Model_Unpacked;
    }
    else if (stride == 32) {
        if (hasFloat2TexCoord && hasTexCoord1) {
            return bindless::SourceVertexFormat::R1_Lmap_Unpacked;
        }
        if (hasTexCoord1) {
            return bindless::SourceVertexFormat::R1_Lmap;  // Lightmapped
        }
        if (hasColor) {
            return bindless::SourceVertexFormat::R1_Vert;  // Vertex-lit
        }
        if (hasShort4TexCoord) {
            return bindless::SourceVertexFormat::MU_Model;  // Trees/models
        }
        // Default to R1_Lmap for 32-byte without clear indicators
        return bindless::SourceVertexFormat::R1_Lmap;
    }

    Msg("! [GPUCulling] Unknown vertex format: stride=%u, hasColor=%d, hasTexCoord1=%d, hasShort4=%d",
        stride, hasColor, hasTexCoord1, hasShort4TexCoord);
    return bindless::SourceVertexFormat::Unknown;
}

u32 GPUCullingManager::RegisterVBPool(
    const void* vertices,
    u32 vertexCount,
    u32 vertexStride,
    const D3DVERTEXELEMENT9* decl,
    bool alternative)
{
    if (!m_levelLoadInProgress) {
        Msg("! [GPUCulling] RegisterVBPool called outside of level load");
        return UINT32_MAX;
    }

    if (!vertices || vertexCount == 0) {
        Msg("! [GPUCulling] RegisterVBPool: Invalid parameters");
        return UINT32_MAX;
    }

    // Detect vertex format from declaration
    bindless::SourceVertexFormat format = DetectFormatFromDecl(decl, vertexStride);
    if (format == bindless::SourceVertexFormat::Unknown) {
        // Still register a placeholder pool to keep indices in sync
        // (FVisual stores the raw VB ID, so pool[i] must correspond to VB[i])
        Msg("! [GPUCulling] RegisterVBPool: Unknown vertex format (stride=%u) - registering placeholder", vertexStride);

        VBPoolInfo placeholder;
        placeholder.megaBufferVertexOffset = 0;
        placeholder.vertexCount = 0;  // Empty placeholder
        placeholder.format = bindless::SourceVertexFormat::Unknown;

        xr_vector<VBPoolInfo>& pools = alternative ? m_vbPoolsAlt : m_vbPools;
        u32 poolID = static_cast<u32>(pools.size());
        pools.push_back(placeholder);

        return poolID;  // Return valid index but pool has 0 vertices
    }

    // Create pool info
    VBPoolInfo poolInfo;
    poolInfo.megaBufferVertexOffset = m_totalVertexCount;
    poolInfo.vertexCount = vertexCount;
    poolInfo.format = format;

    // Convert vertices to unified format and add to mega-buffer
    u32 prevSize = static_cast<u32>(m_megaVertices.size());
    m_megaVertices.resize(prevSize + vertexCount);

    u32 converted = bindless::VertexConverter::ConvertVertices(
        vertices, vertexStride, vertexCount, format,
        &m_megaVertices[prevSize]
    );

    if (converted != vertexCount) {
        Msg("! [GPUCulling] RegisterVBPool: Vertex conversion failed (got %u, expected %u)", converted, vertexCount);
        m_megaVertices.resize(prevSize);  // Rollback
        return UINT32_MAX;
    }

    m_totalVertexCount += vertexCount;

    // Store pool info
    xr_vector<VBPoolInfo>& pools = alternative ? m_vbPoolsAlt : m_vbPools;
    u32 poolID = static_cast<u32>(pools.size());
    pools.push_back(poolInfo);

    Msg("* [GPUCulling] RegisterVBPool[%u]: %u verts (format=%d, offset=%u)%s",
        poolID, vertexCount, static_cast<int>(format), poolInfo.megaBufferVertexOffset,
        alternative ? " [ALT]" : "");

    return poolID;
}

u32 GPUCullingManager::RegisterIBPool(
    const u16* indices,
    u32 indexCount,
    bool alternative)
{
    if (!m_levelLoadInProgress) {
        Msg("! [GPUCulling] RegisterIBPool called outside of level load");
        return UINT32_MAX;
    }

    if (!indices || indexCount == 0) {
        // Register placeholder to keep indices in sync
        Msg("! [GPUCulling] RegisterIBPool: Invalid parameters - registering placeholder");

        IBPoolInfo placeholder;
        placeholder.megaBufferIndexOffset = 0;
        placeholder.indexCount = 0;

        xr_vector<IBPoolInfo>& pools = alternative ? m_ibPoolsAlt : m_ibPools;
        u32 poolID = static_cast<u32>(pools.size());
        pools.push_back(placeholder);

        return poolID;
    }

    // Create pool info
    IBPoolInfo poolInfo;
    poolInfo.megaBufferIndexOffset = m_totalIndexCount;
    poolInfo.indexCount = indexCount;

    // Convert from 16-bit to 32-bit indices
    u32 prevSize = static_cast<u32>(m_megaIndices.size());
    m_megaIndices.resize(prevSize + indexCount);

    for (u32 i = 0; i < indexCount; i++) {
        m_megaIndices[prevSize + i] = static_cast<u32>(indices[i]);
    }

    m_totalIndexCount += indexCount;

    // Store pool info
    xr_vector<IBPoolInfo>& pools = alternative ? m_ibPoolsAlt : m_ibPools;
    u32 poolID = static_cast<u32>(pools.size());
    pools.push_back(poolInfo);

    Msg("* [GPUCulling] RegisterIBPool[%u]: %u indices (offset=%u)%s",
        poolID, indexCount, poolInfo.megaBufferIndexOffset,
        alternative ? " [ALT]" : "");

    return poolID;
}

MeshAllocation GPUCullingManager::GetMeshAllocation(
    u32 vbID, u32 vBase, u32 vCount,
    u32 ibID, u32 iBase, u32 iCount,
    bool alternative) const
{
    MeshAllocation alloc;

    const xr_vector<VBPoolInfo>& vbPools = alternative ? m_vbPoolsAlt : m_vbPools;
    const xr_vector<IBPoolInfo>& ibPools = alternative ? m_ibPoolsAlt : m_ibPools;

    if (vbID >= vbPools.size() || ibID >= ibPools.size()) {
        // Pool not registered - mesh not in mega-buffer
        return alloc;
    }

    const VBPoolInfo& vbPool = vbPools[vbID];
    const IBPoolInfo& ibPool = ibPools[ibID];

    // Check for placeholder pools (unknown format or invalid data)
    if (vbPool.vertexCount == 0 || ibPool.indexCount == 0) {
        // Placeholder pool - mesh uses unsupported vertex format
        return alloc;
    }

    // Validate bounds
    if (vBase + vCount > vbPool.vertexCount) {
        Msg("! [GPUCulling] GetMeshAllocation: VB bounds exceeded (vBase=%u, vCount=%u, poolCount=%u)",
            vBase, vCount, vbPool.vertexCount);
        return alloc;
    }
    if (iBase + iCount > ibPool.indexCount) {
        Msg("! [GPUCulling] GetMeshAllocation: IB bounds exceeded (iBase=%u, iCount=%u, poolCount=%u)",
            iBase, iCount, ibPool.indexCount);
        return alloc;
    }

    // Calculate mega-buffer offsets
    // vBase/iBase are the mesh's starting offsets within its VB/IB pool
    // Indices in X-Ray are relative to the start of the IB pool, and reference
    // vertices relative to vBase=0 of the VB pool
    alloc.vertexOffset = vbPool.megaBufferVertexOffset + vBase;
    alloc.indexOffset = ibPool.megaBufferIndexOffset + iBase;
    alloc.vertexCount = vCount;
    alloc.indexCount = iCount;
    alloc.valid = true;

    return alloc;
}

} // namespace xray::render::RENDER_NAMESPACE
