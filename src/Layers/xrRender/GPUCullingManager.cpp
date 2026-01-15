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
    Fmatrix viewProj;           // Current frame view-projection (for frustum culling)
    Fmatrix prevViewProj;       // Previous frame view-projection (for Hi-Z sampling)
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
static ref_cs s_terrain_apply_visibility_cs;

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
    m_staticObjectData.reserve(MAX_CULLING_OBJECTS);
    m_staticDrawArgsData.reserve(MAX_CULLING_OBJECTS);
    m_staticMaterialIDData.reserve(MAX_CULLING_OBJECTS);
    m_staticInstanceData.reserve(MAX_CULLING_OBJECTS);

    m_dynamicObjectData.reserve(MAX_CULLING_OBJECTS);
    m_dynamicDrawArgsData.reserve(MAX_CULLING_OBJECTS);
    m_dynamicMaterialIDData.reserve(MAX_CULLING_OBJECTS);
    m_dynamicInstanceData.reserve(MAX_CULLING_OBJECTS);

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

    auto createCullSetBuffers = [&](CullSetBuffers& set, bool isStatic) {
        set.maxObjects = m_maxObjects;
        auto pickName = [isStatic](const char* staticName, const char* dynamicName) {
            return isStatic ? staticName : dynamicName;
        };

        // Object buffer: Structured buffer of GPUObjectData
        {
            nvrhi::BufferDesc desc;
            desc.debugName = pickName("GPUCull_Static_Objects", "GPUCull_Dynamic_Objects");
            desc.byteSize = m_maxObjects * sizeof(GPUObjectData);
            desc.structStride = sizeof(GPUObjectData);
            desc.initialState = nvrhi::ResourceStates::ShaderResource;
            desc.keepInitialState = true;

            set.objectBuffer = nvDevice->createBuffer(desc);
            R_ASSERT2(set.objectBuffer, "Failed to create object buffer");
        }

        // Visible index buffer: Structured buffer of u32
        {
            nvrhi::BufferDesc desc;
            desc.debugName = pickName("GPUCull_Static_VisibleIndices", "GPUCull_Dynamic_VisibleIndices");
            desc.byteSize = m_maxObjects * sizeof(u32);
            desc.structStride = sizeof(u32);
            desc.canHaveUAVs = true;
            desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
            desc.keepInitialState = true;

            set.visibleIndexBuffer = nvDevice->createBuffer(desc);
            R_ASSERT2(set.visibleIndexBuffer, "Failed to create visible index buffer");
        }

        // Visible count buffer: Single u32 atomic counter
        {
            nvrhi::BufferDesc desc;
            desc.debugName = pickName("GPUCull_Static_VisibleCount", "GPUCull_Dynamic_VisibleCount");
            desc.byteSize = sizeof(u32);
            desc.canHaveUAVs = true;
            desc.canHaveRawViews = true;  // For InterlockedAdd
            desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
            desc.keepInitialState = true;

            set.visibleCountBuffer = nvDevice->createBuffer(desc);
            R_ASSERT2(set.visibleCountBuffer, "Failed to create visible count buffer");
        }

        // Draw arguments buffer: Indirect draw args for each batch
        // NOTE: Cannot use structured buffer with indirect args on D3D11
        // Must use raw buffer (byte address buffer) instead
        {
            nvrhi::BufferDesc desc;
            desc.debugName = pickName("GPUCull_Static_DrawArgs", "GPUCull_Dynamic_DrawArgs");
            desc.byteSize = m_maxObjects * sizeof(IndirectDrawArgs);
            // No structStride - use as raw buffer
            desc.canHaveUAVs = true;
            desc.canHaveRawViews = true;  // Allow RWByteAddressBuffer access
            desc.isDrawIndirectArgs = true;  // CRITICAL: Allows use with DrawIndexedIndirect
            desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
            desc.keepInitialState = true;  // Let NVRHI handle state transitions

            set.drawArgsBuffer = nvDevice->createBuffer(desc);
            R_ASSERT2(set.drawArgsBuffer, "Failed to create draw args buffer");
        }

        // Visibility buffer (1 uint per object: 0=culled, 1=visible)
        // GPU culling writes here instead of modifying draw args
        // Avoids per-frame CPU upload of draw args
        {
            nvrhi::BufferDesc desc;
            desc.debugName = pickName("GPUCull_Static_Visibility", "GPUCull_Dynamic_Visibility");
            desc.byteSize = m_maxObjects * sizeof(u32);
            desc.structStride = sizeof(u32);
            desc.canHaveUAVs = true;
            desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
            desc.keepInitialState = true;

            set.visibilityBuffer = nvDevice->createBuffer(desc);
            R_ASSERT2(set.visibilityBuffer, "Failed to create visibility buffer");
        }
    };

    createCullSetBuffers(m_staticSet, true);
    createCullSetBuffers(m_dynamicSet, false);

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

    // Terrain visibility buffer (1 uint per object, like regular geometry)
    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_TerrainVisibility";
        desc.byteSize = m_maxTerrainObjects * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.canHaveUAVs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;

        m_terrainVisibilityBuffer = nvDevice->createBuffer(desc);
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
    // u2: g_Visibility (structured buffer UAV - 1 uint per object: 0=culled, 1=visible)
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
            nvrhi::BindingLayoutItem::StructuredBuffer_UAV(2)  // Visibility buffer (replaces draw args write)
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
        auto createCompactionBuffers = [&](CullSetBuffers& set, bool isStatic) {
            auto pickName = [isStatic](const char* staticName, const char* dynamicName) {
                return isStatic ? staticName : dynamicName;
            };
            {
                nvrhi::BufferDesc desc;
                desc.debugName = pickName("GPUCull_Static_CompactDrawArgs", "GPUCull_Dynamic_CompactDrawArgs");
                desc.byteSize = m_maxObjects * sizeof(IndirectDrawArgs);
                // No structStride - raw buffer for D3D11 indirect args compatibility
                desc.canHaveUAVs = true;
                desc.canHaveRawViews = true;  // RWByteAddressBuffer access
                desc.isDrawIndirectArgs = true;
                desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
                desc.keepInitialState = true;  // Let NVRHI handle state transitions

                set.compactDrawArgsBuffer = nvDevice->createBuffer(desc);
                R_ASSERT2(set.compactDrawArgsBuffer, "Failed to create compact draw args buffer");
            }

            {
                nvrhi::BufferDesc desc;
                desc.debugName = pickName("GPUCull_Static_CompactBatchIndices", "GPUCull_Dynamic_CompactBatchIndices");
                desc.byteSize = m_maxObjects * sizeof(u32);
                desc.structStride = sizeof(u32);
                desc.canHaveUAVs = true;
                desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
                desc.keepInitialState = true;  // Let NVRHI handle state transitions

                set.compactBatchIndicesBuffer = nvDevice->createBuffer(desc);
                R_ASSERT2(set.compactBatchIndicesBuffer, "Failed to create compact batch indices buffer");
            }

            // Material ID buffers for bindless rendering
            {
                nvrhi::BufferDesc desc;
                desc.debugName = pickName("GPUCull_Static_MaterialIDs", "GPUCull_Dynamic_MaterialIDs");
                desc.byteSize = m_maxObjects * sizeof(u32);
                desc.structStride = sizeof(u32);
                desc.initialState = nvrhi::ResourceStates::ShaderResource;
                desc.keepInitialState = true;

                set.materialIDBuffer = nvDevice->createBuffer(desc);
                R_ASSERT2(set.materialIDBuffer, "Failed to create material ID buffer");
            }

            {
                nvrhi::BufferDesc desc;
                desc.debugName = pickName("GPUCull_Static_CompactMaterialIDs", "GPUCull_Dynamic_CompactMaterialIDs");
                desc.byteSize = m_maxObjects * sizeof(u32);
                desc.structStride = sizeof(u32);
                desc.canHaveUAVs = true;
                desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
                desc.keepInitialState = true;  // Let NVRHI handle state transitions

                set.compactMaterialIDBuffer = nvDevice->createBuffer(desc);
                R_ASSERT2(set.compactMaterialIDBuffer, "Failed to create compact material ID buffer");
            }

            {
                nvrhi::BufferDesc desc;
                desc.debugName = pickName("GPUCull_Static_CompactCount", "GPUCull_Dynamic_CompactCount");
                desc.byteSize = sizeof(u32);
                desc.canHaveUAVs = true;
                desc.canHaveRawViews = true;
                desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
                desc.keepInitialState = true;

                set.compactCountBuffer = nvDevice->createBuffer(desc);
                R_ASSERT2(set.compactCountBuffer, "Failed to create compact count buffer");
            }
        };

        createCompactionBuffers(m_staticSet, true);
        createCompactionBuffers(m_dynamicSet, false);
    }

    // Terrain compaction buffers (separate from main geometry)
    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_TerrainCompactDrawArgs";
        desc.byteSize = m_maxTerrainObjects * sizeof(IndirectDrawArgs);
        desc.canHaveUAVs = true;
        desc.canHaveRawViews = true;
        desc.isDrawIndirectArgs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;

        m_terrainCompactDrawArgsBuffer = nvDevice->createBuffer(desc);
        R_ASSERT2(m_terrainCompactDrawArgsBuffer, "Failed to create terrain compact draw args buffer");
    }

    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_TerrainCompactBatchIndices";
        desc.byteSize = m_maxTerrainObjects * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.canHaveUAVs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;

        m_terrainCompactBatchIndicesBuffer = nvDevice->createBuffer(desc);
        R_ASSERT2(m_terrainCompactBatchIndicesBuffer, "Failed to create terrain compact batch indices buffer");
    }

    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_TerrainCompactMaterialIDs";
        desc.byteSize = m_maxTerrainObjects * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.canHaveUAVs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;

        m_terrainCompactMaterialIDBuffer = nvDevice->createBuffer(desc);
        R_ASSERT2(m_terrainCompactMaterialIDBuffer, "Failed to create terrain compact material ID buffer");
    }

    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_TerrainCompactCount";
        desc.byteSize = sizeof(u32);
        desc.canHaveUAVs = true;
        desc.canHaveRawViews = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;

        m_terrainCompactCountBuffer = nvDevice->createBuffer(desc);
        R_ASSERT2(m_terrainCompactCountBuffer, "Failed to create terrain compact count buffer");
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
        // t2: g_Visibility (StructuredBuffer<uint> - SRV, from cull pass)
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
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(2),   // Visibility buffer (from cull pass)
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

    // ───────────────────────────────────────────────────────
    //  TERRAIN APPLY VISIBILITY PIPELINE
    // ───────────────────────────────────────────────────────
    // Copies terrain visibility buffer → instanceCount in draw args
    // Simpler than full compaction since terrain doesn't need sorting
    s_terrain_apply_visibility_cs.create("terrain_apply_visibility");
    if (s_terrain_apply_visibility_cs && s_terrain_apply_visibility_cs->nvrhiShader) {
        // Layout: b5 = params, t0 = visibility, u0 = draw args
        nvrhi::BindingLayoutDesc layoutDesc;
        layoutDesc.visibility = nvrhi::ShaderType::Compute;
        layoutDesc.bindings = {
            nvrhi::BindingLayoutItem::VolatileConstantBuffer(5),  // ApplyVisibilityParams
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(0),    // Visibility buffer
            nvrhi::BindingLayoutItem::RawBuffer_UAV(0)            // Draw args (write instanceCount)
        };

        m_terrainApplyVisibilityLayout = nvDevice->createBindingLayout(layoutDesc);
        if (m_terrainApplyVisibilityLayout) {
            nvrhi::ComputePipelineDesc pipeDesc;
            pipeDesc.CS = s_terrain_apply_visibility_cs->nvrhiShader;
            pipeDesc.bindingLayouts = { m_terrainApplyVisibilityLayout };

            m_terrainApplyVisibilityPipeline = nvDevice->createComputePipeline(pipeDesc);
            if (m_terrainApplyVisibilityPipeline) {
                Msg("* [GPUCulling] Terrain apply visibility pipeline created");
            }
        }
    } else {
        Msg("! [GPUCulling] terrain_apply_visibility.cs not found");
    }
}

void GPUCullingManager::Shutdown()
{
    m_staticSet.objectBuffer = nullptr;
    m_staticSet.visibleIndexBuffer = nullptr;
    m_staticSet.visibleCountBuffer = nullptr;
    m_staticSet.drawArgsBuffer = nullptr;
    m_staticSet.materialIDBuffer = nullptr;
    m_staticSet.visibilityBuffer = nullptr;
    m_staticSet.compactDrawArgsBuffer = nullptr;
    m_staticSet.compactBatchIndicesBuffer = nullptr;
    m_staticSet.compactMaterialIDBuffer = nullptr;
    m_staticSet.compactCountBuffer = nullptr;
    m_staticSet.instanceBuffer = nullptr;
    m_staticSet.objectCount = 0;
    m_staticSet.maxObjects = 0;
    m_staticSet.drawArgsUploaded = false;
    m_staticSet.objectsUploaded = false;

    m_dynamicSet.objectBuffer = nullptr;
    m_dynamicSet.visibleIndexBuffer = nullptr;
    m_dynamicSet.visibleCountBuffer = nullptr;
    m_dynamicSet.drawArgsBuffer = nullptr;
    m_dynamicSet.materialIDBuffer = nullptr;
    m_dynamicSet.visibilityBuffer = nullptr;
    m_dynamicSet.compactDrawArgsBuffer = nullptr;
    m_dynamicSet.compactBatchIndicesBuffer = nullptr;
    m_dynamicSet.compactMaterialIDBuffer = nullptr;
    m_dynamicSet.compactCountBuffer = nullptr;
    m_dynamicSet.instanceBuffer = nullptr;
    m_dynamicSet.objectCount = 0;
    m_dynamicSet.maxObjects = 0;
    m_dynamicSet.drawArgsUploaded = false;
    m_dynamicSet.objectsUploaded = false;

    m_cullParamsCB = nullptr;
    m_cullPipeline = nullptr;
    m_cullLayout = nullptr;
    m_pointSampler = nullptr;

    m_compactParamsCB = nullptr;
    m_compactPipeline = nullptr;
    m_compactLayout = nullptr;

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
    m_megaVertices.clear();
    m_megaIndices.clear();
    m_staticInstanceData.clear();
    m_dynamicInstanceData.clear();
    m_staticObjectData.clear();
    m_staticDrawArgsData.clear();
    m_staticMaterialIDData.clear();
    m_dynamicObjectData.clear();
    m_dynamicDrawArgsData.clear();
    m_dynamicMaterialIDData.clear();
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
    m_terrainVisibilityBuffer = nullptr;
    m_terrainInstanceBuffer = nullptr;
    m_terrainBatchIndicesBuffer = nullptr;
    m_terrainMaterialIDBuffer = nullptr;
    m_terrainCompactDrawArgsBuffer = nullptr;
    m_terrainCompactBatchIndicesBuffer = nullptr;
    m_terrainCompactCountBuffer = nullptr;
    m_terrainCompactMaterialIDBuffer = nullptr;
    m_terrainApplyVisibilityPipeline = nullptr;
    m_terrainApplyVisibilityLayout = nullptr;
    m_terrainObjectData.clear();
    m_terrainDrawArgsData.clear();
    m_terrainMaterialIDData.clear();
    m_terrainInstanceData.clear();
    m_terrainObjectCount = 0;

    // Visibility buffer
    m_staticTerrainDrawArgsUploaded = false;
    m_staticDataCached = false;

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

    if (totalBatches == 0) {
        m_staticSet.objectCount = 0;
        m_dynamicSet.objectCount = 0;
        m_objectCount = 0;
        return;
    }

    R_ASSERT2(m_staticSet.objectBuffer && m_staticSet.drawArgsBuffer && m_staticSet.visibilityBuffer,
        "Static GPU culling buffers not initialized");
    R_ASSERT2(m_dynamicSet.objectBuffer && m_dynamicSet.drawArgsBuffer && m_dynamicSet.visibilityBuffer,
        "Dynamic GPU culling buffers not initialized");

    // Build object data, draw args, and material ID arrays
    // NOTE: Skip skinned batches - they use separate per-draw rendering with bone matrices
    // NOTE: Terrain batches are tracked separately for terrain shader rendering
    if (!m_staticDataCached) {
        m_staticObjectData.clear();
        m_staticObjectData.reserve(totalBatches);
        m_staticDrawArgsData.clear();
        m_staticDrawArgsData.reserve(totalBatches);
        m_staticMaterialIDData.clear();
        m_staticMaterialIDData.reserve(totalBatches);
        m_staticInstanceData.clear();
        m_staticInstanceData.reserve(totalBatches);
    }

    m_dynamicObjectData.clear();
    m_dynamicObjectData.reserve(totalBatches);
    m_dynamicDrawArgsData.clear();
    m_dynamicDrawArgsData.reserve(totalBatches);
    m_dynamicMaterialIDData.clear();
    m_dynamicMaterialIDData.reserve(totalBatches);
    m_dynamicInstanceData.clear();
    m_dynamicInstanceData.reserve(totalBatches);

    // Terrain-specific arrays
    m_terrainObjectData.clear();
    m_terrainObjectData.reserve(totalBatches / 4);  // Terrain is typically ~25% of batches
    m_terrainDrawArgsData.clear();
    m_terrainDrawArgsData.reserve(totalBatches / 4);
    m_terrainMaterialIDData.clear();
    m_terrainMaterialIDData.reserve(totalBatches / 4);
    m_terrainInstanceData.clear();
    m_terrainInstanceData.reserve(totalBatches / 4);

    auto appendBatch = [&](const GeometryBatch& batch,
                           xr_vector<GPUObjectData>& objectData,
                           xr_vector<IndirectDrawArgs>& drawArgsData,
                           xr_vector<u32>& materialIDData,
                           xr_vector<GPUInstanceData>& instanceData) {
        // ─────────────────────────────────────────────────────
        //  BUILD OBJECT DATA (for culling)
        // ─────────────────────────────────────────────────────
        GPUObjectData obj;
        obj.position = batch.worldBoundsCenter;
        obj.radius = batch.worldBoundsRadius;
        obj.batchIndex = static_cast<u32>(objectData.size());

        obj.flags = 0;
        if (batch.IsOpaque())
            obj.flags |= GPU_OBJECT_OPAQUE;
        if (batch.IsAlphaTested())
            obj.flags |= GPU_OBJECT_ALPHA_TEST;
        if (batch.IsStrictB2F())
            obj.flags |= GPU_OBJECT_TRANSPARENT;

        obj.pad0 = 0.0f;
        obj.pad1 = 0.0f;

        objectData.push_back(obj);

        // ─────────────────────────────────────────────────────
        //  BUILD DRAW ARGS (for indirect draw)
        // ─────────────────────────────────────────────────────
        IndirectDrawArgs args;
        args.indexCountPerInstance = batch.indexCount;
        args.instanceCount = 1;  // Compaction uses visibility buffer
        if (batch.megaBufferAlloc.valid) {
            args.startIndexLocation = batch.megaBufferAlloc.indexOffset;
            args.baseVertexLocation = static_cast<s32>(batch.megaBufferAlloc.vertexOffset);
        } else {
            args.startIndexLocation = batch.startIndex;
            args.baseVertexLocation = batch.baseVertex;
        }
        args.startInstanceLocation = 0;
        drawArgsData.push_back(args);

        // ─────────────────────────────────────────────────────
        //  MATERIAL ID (for bindless rendering)
        // ─────────────────────────────────────────────────────
        materialIDData.push_back(batch.bindlessMaterialID);

        // ─────────────────────────────────────────────────────
        //  INSTANCE DATA (world transforms + material ID)
        // ─────────────────────────────────────────────────────
        GPUInstanceData inst;
        inst.world.transpose(batch.worldMatrix);
        inst.materialID = batch.bindlessMaterialID;
        inst.flags = obj.flags;
        inst.pad0 = 0.0f;
        inst.pad1 = 0.0f;
        instanceData.push_back(inst);
    };

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

        if (batch.isStatic) {
            if (!m_staticDataCached) {
                appendBatch(batch, m_staticObjectData, m_staticDrawArgsData, m_staticMaterialIDData, m_staticInstanceData);
            }
        } else {
            appendBatch(batch, m_dynamicObjectData, m_dynamicDrawArgsData, m_dynamicMaterialIDData, m_dynamicInstanceData);
        }
    }

    // Set object counts with total cap
    u32 staticCount = std::min(static_cast<u32>(m_staticObjectData.size()), m_maxObjects);
    if (m_staticObjectData.size() > staticCount) {
        m_staticObjectData.resize(staticCount);
        m_staticDrawArgsData.resize(staticCount);
        m_staticMaterialIDData.resize(staticCount);
        m_staticInstanceData.resize(staticCount);
    }

    u32 dynamicCapacity = (staticCount < m_maxObjects) ? (m_maxObjects - staticCount) : 0;
    u32 dynamicCount = std::min(static_cast<u32>(m_dynamicObjectData.size()), dynamicCapacity);
    if (m_dynamicObjectData.size() > dynamicCount) {
        m_dynamicObjectData.resize(dynamicCount);
        m_dynamicDrawArgsData.resize(dynamicCount);
        m_dynamicMaterialIDData.resize(dynamicCount);
        m_dynamicInstanceData.resize(dynamicCount);
    }

    m_staticSet.objectCount = staticCount;
    m_dynamicSet.objectCount = dynamicCount;
    m_objectCount = staticCount + dynamicCount;

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

    if (m_staticSet.objectCount > 0 && !m_staticSet.objectsUploaded) {
        R_ASSERT2(m_staticObjectData.size() >= m_staticSet.objectCount, "Static object data smaller than count");
        R_ASSERT2(m_staticDrawArgsData.size() >= m_staticSet.objectCount, "Static draw args data smaller than count");
        R_ASSERT2(m_staticMaterialIDData.size() >= m_staticSet.objectCount, "Static material ID data smaller than count");
        R_ASSERT2(m_staticInstanceData.size() >= m_staticSet.objectCount, "Static instance data smaller than count");

        cmdList->writeBuffer(m_staticSet.objectBuffer,
            m_staticObjectData.data(),
            m_staticSet.objectCount * sizeof(GPUObjectData));

        cmdList->writeBuffer(m_staticSet.drawArgsBuffer,
            m_staticDrawArgsData.data(),
            m_staticSet.objectCount * sizeof(IndirectDrawArgs));

        if (m_compactEnabled && m_staticSet.materialIDBuffer) {
            cmdList->writeBuffer(m_staticSet.materialIDBuffer,
                m_staticMaterialIDData.data(),
                m_staticSet.objectCount * sizeof(u32));
        }

        R_ASSERT2(m_staticSet.instanceBuffer, "Static instance buffer not initialized");
        cmdList->writeBuffer(m_staticSet.instanceBuffer,
            m_staticInstanceData.data(),
            m_staticSet.objectCount * sizeof(GPUInstanceData));

        m_staticSet.objectsUploaded = true;
        m_staticSet.drawArgsUploaded = true;
        m_staticDataCached = true;

        Msg("* [GPUCulling] Static object data uploaded: %u objects", m_staticSet.objectCount);
    }

    if (m_dynamicSet.objectCount > 0) {
        R_ASSERT2(m_dynamicObjectData.size() >= m_dynamicSet.objectCount, "Dynamic object data smaller than count");
        R_ASSERT2(m_dynamicDrawArgsData.size() >= m_dynamicSet.objectCount, "Dynamic draw args data smaller than count");
        R_ASSERT2(m_dynamicMaterialIDData.size() >= m_dynamicSet.objectCount, "Dynamic material ID data smaller than count");
        R_ASSERT2(m_dynamicInstanceData.size() >= m_dynamicSet.objectCount, "Dynamic instance data smaller than count");

        cmdList->writeBuffer(m_dynamicSet.objectBuffer,
            m_dynamicObjectData.data(),
            m_dynamicSet.objectCount * sizeof(GPUObjectData));

        cmdList->writeBuffer(m_dynamicSet.drawArgsBuffer,
            m_dynamicDrawArgsData.data(),
            m_dynamicSet.objectCount * sizeof(IndirectDrawArgs));

        if (m_compactEnabled && m_dynamicSet.materialIDBuffer) {
            cmdList->writeBuffer(m_dynamicSet.materialIDBuffer,
                m_dynamicMaterialIDData.data(),
                m_dynamicSet.objectCount * sizeof(u32));
        }

        R_ASSERT2(m_dynamicSet.instanceBuffer, "Dynamic instance buffer not initialized");
        cmdList->writeBuffer(m_dynamicSet.instanceBuffer,
            m_dynamicInstanceData.data(),
            m_dynamicSet.objectCount * sizeof(GPUInstanceData));
    }

    // ─────────────────────────────────────────────────────
    //  TERRAIN BUFFER UPLOADS
    // ─────────────────────────────────────────────────────
    m_terrainObjectCount = std::min(static_cast<u32>(m_terrainObjectData.size()), m_maxTerrainObjects);

    if (m_terrainObjectCount > 0 && m_terrainObjectBuffer && m_terrainDrawArgsBuffer) {
        R_ASSERT2(m_terrainObjectCount <= m_maxTerrainObjects, "Terrain object count exceeds buffer capacity");
        R_ASSERT2(m_terrainDrawArgsData.size() >= m_terrainObjectCount, "Terrain draw args data smaller than object count");
        R_ASSERT2(m_terrainMaterialIDData.size() >= m_terrainObjectCount, "Terrain material ID data smaller than object count");
        R_ASSERT2(m_terrainInstanceData.size() >= m_terrainObjectCount, "Terrain instance data smaller than object count");

        // Terrain object data (bounding spheres) - still uploaded every frame
        // TODO: Terrain is static, could cache this too with dirty flag
        cmdList->beginTrackingBufferState(m_terrainObjectBuffer, nvrhi::ResourceStates::CopyDest);
        cmdList->writeBuffer(m_terrainObjectBuffer,
            m_terrainObjectData.data(),
            m_terrainObjectCount * sizeof(GPUObjectData));
        cmdList->setBufferState(m_terrainObjectBuffer, nvrhi::ResourceStates::ShaderResource);

        // Terrain draw args, material IDs, instance data - uploaded ONCE (visibility buffer handles culling)
        if (!m_staticTerrainDrawArgsUploaded) {
            // Set instanceCount=1 for all terrain (apply visibility pass will set actual visibility)
            for (auto& args : m_terrainDrawArgsData) {
                args.instanceCount = 1;
            }

            // Upload terrain draw args
            cmdList->beginTrackingBufferState(m_terrainDrawArgsBuffer, nvrhi::ResourceStates::CopyDest);
            cmdList->writeBuffer(m_terrainDrawArgsBuffer,
                m_terrainDrawArgsData.data(),
                m_terrainObjectCount * sizeof(IndirectDrawArgs));
            cmdList->setBufferState(m_terrainDrawArgsBuffer, nvrhi::ResourceStates::IndirectArgument);

            // Upload terrain material IDs
            if (m_terrainMaterialIDBuffer) {
                cmdList->beginTrackingBufferState(m_terrainMaterialIDBuffer, nvrhi::ResourceStates::CopyDest);
                cmdList->writeBuffer(m_terrainMaterialIDBuffer,
                    m_terrainMaterialIDData.data(),
                    m_terrainObjectCount * sizeof(u32));
                cmdList->setBufferState(m_terrainMaterialIDBuffer, nvrhi::ResourceStates::ShaderResource);

                // Debug: Log terrain material IDs once
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
            }

            // Upload terrain instance data (world transforms)
            if (m_terrainInstanceBuffer && !m_terrainInstanceData.empty()) {
                cmdList->beginTrackingBufferState(m_terrainInstanceBuffer, nvrhi::ResourceStates::CopyDest);
                cmdList->writeBuffer(m_terrainInstanceBuffer,
                    m_terrainInstanceData.data(),
                    m_terrainObjectCount * sizeof(GPUInstanceData));
                cmdList->setBufferState(m_terrainInstanceBuffer, nvrhi::ResourceStates::ShaderResource);
            }

            // Upload terrain batch indices (identity mapping: 0,1,2,3...)
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

            m_staticTerrainDrawArgsUploaded = true;
            Msg("* [GPUCulling] Static terrain draw args uploaded: %u objects (visibility buffer mode)", m_terrainObjectCount);
        }
    }

    // Upload instance data (world matrices) handled above for static/dynamic sets
}

void GPUCullingManager::InvalidateStaticCullingData()
{
    m_staticDataCached = false;
    m_staticSet.objectsUploaded = false;
    m_staticSet.drawArgsUploaded = false;
    m_staticSet.objectCount = 0;

    m_staticObjectData.clear();
    m_staticDrawArgsData.clear();
    m_staticMaterialIDData.clear();
    m_staticInstanceData.clear();

    Msg("* [GPUCulling] Static culling data invalidated");
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
    const GeometryCollector* geometry,
    const Fmatrix& prevViewProj)
{
    using namespace framegraph;

    GPUCullOutput output;
    output.maxObjects = m_maxObjects;
    output.visibleIndices = VirtualResourceHandle();
    output.visibleCount = VirtualResourceHandle();
    output.drawArgsBuffer = VirtualResourceHandle();
    output.staticDrawArgsBuffer = VirtualResourceHandle();
    output.dynamicDrawArgsBuffer = VirtualResourceHandle();
    output.staticCompactDrawArgs = VirtualResourceHandle();
    output.staticCompactBatchIndices = VirtualResourceHandle();
    output.dynamicCompactDrawArgs = VirtualResourceHandle();
    output.dynamicCompactBatchIndices = VirtualResourceHandle();
    output.staticObjectCount = 0;
    output.dynamicObjectCount = 0;
    output.terrainDrawArgsBuffer = VirtualResourceHandle();
    output.terrainCompactDrawArgs = VirtualResourceHandle();
    output.terrainCompactBatchIndices = VirtualResourceHandle();
    output.terrainCompactMaterialIDs = VirtualResourceHandle();
    output.terrainCompactCount = VirtualResourceHandle();
    output.terrainObjectCount = 0;

    // Early out if not enabled
    if (!m_computeEnabled) {
        return output;
    }

    // Pre-check geometry size to avoid unnecessary pass setup
    if (!geometry || geometry->GetBatches().empty()) {
        return output;
    }

    struct GPUCullPassData {
        VirtualResourceHandle hizPyramid;
        VirtualResourceHandle staticDrawArgsBuffer;   // For framegraph tracking
        VirtualResourceHandle dynamicDrawArgsBuffer;  // For framegraph tracking

        GPUCullingManager* manager;
        const GeometryCollector* geometry;  // For uploading during execute
        Fmatrix prevViewProj;  // Previous frame's viewProj for temporal Hi-Z
        u32 hizWidth;
        u32 hizHeight;
        u32 hizMipLevels;
    };

    // Import draw args buffers into framegraph for proper state tracking
    // This allows forward pass to properly transition the buffers to IndirectArgument state
    ResourceDesc drawArgsDesc;
    drawArgsDesc.type = ResourceDesc::Type::Buffer;
    drawArgsDesc.debugName = "GPUCull_DrawArgs";
    drawArgsDesc.bufferSize = m_maxObjects * sizeof(IndirectDrawArgs);
    drawArgsDesc.structStride = sizeof(IndirectDrawArgs);
    drawArgsDesc.isUAV = true;
    drawArgsDesc.isTransient = false;  // Persistent - forward pass needs it

    VirtualResourceHandle staticDrawArgsHandle = fg.ImportBuffer("gpu_cull_static_drawargs", m_staticSet.drawArgsBuffer, drawArgsDesc);
    VirtualResourceHandle dynamicDrawArgsHandle = fg.ImportBuffer("gpu_cull_dynamic_drawargs", m_dynamicSet.drawArgsBuffer, drawArgsDesc);

    auto& passData = fg.addCallbackPass<GPUCullPassData>(
        "GPU Culling",

        // Setup lambda
        [&, hizWidth, hizHeight, hizMipLevels, staticDrawArgsHandle, dynamicDrawArgsHandle, geometry, prevViewProj](FrameGraph& builder, PassHandle passHandle, GPUCullPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.manager = this;
            data.geometry = geometry;  // Capture for upload during execute
            data.prevViewProj = prevViewProj;  // Previous frame's viewProj for temporal Hi-Z
            data.hizWidth = hizWidth;
            data.hizHeight = hizHeight;
            data.hizMipLevels = hizMipLevels;

            // Read Hi-Z pyramid
            data.hizPyramid = passBuilder.read(hizPyramid, ResourceState::ShaderResource);

            // Write draw args buffers (for dependency tracking)
            data.staticDrawArgsBuffer = passBuilder.write(staticDrawArgsHandle, ResourceState::UnorderedAccess);
            data.dynamicDrawArgsBuffer = passBuilder.write(dynamicDrawArgsHandle, ResourceState::UnorderedAccess);
        },

        // Execute lambda
        [](const GPUCullPassData& data,
           const FrameGraph& fg,
           ng::RenderContext* ctx) {

            GPUCullingManager* mgr = data.manager;
            if (!mgr->m_computeEnabled)
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

            auto dispatchCullSet = [&](CullSetBuffers& set) {
                if (set.objectCount == 0)
                    return;

                R_ASSERT2(set.objectBuffer && set.visibleIndexBuffer && set.visibleCountBuffer && set.visibilityBuffer,
                    "Cull set buffers not initialized");

                // Clear visible count to 0
                u32 zero = 0;
                cmdList->writeBuffer(set.visibleCountBuffer, &zero, sizeof(u32));

                // Clear visibility buffer to 0 (cull pass will set 1 for visible objects)
                // This is fast GPU-side clear, avoids CPU re-uploading draw args every frame
                cmdList->clearBufferUInt(set.visibilityBuffer, 0);

                // Fill constant buffer
                CullParamsCB cb;
                cb.viewProj.transpose(Device.mFullTransform);
                cb.prevViewProj.transpose(data.prevViewProj);  // Previous frame's viewProj for temporal Hi-Z
                cb.cameraPos = Device.vCameraPosition;
                float farPlane = g_pGamePersistent ? g_pGamePersistent->Environment().CurrentEnv.far_plane : 300.0f;
                cb.maxDistanceSq = farPlane * farPlane;
                cb.objectCount = set.objectCount;
                cb.hizWidth = data.hizWidth;
                cb.hizHeight = data.hizHeight;
                cb.hizMipLevels = data.hizMipLevels;

                mgr->ExtractFrustumPlanes(Device.mFullTransform, cb.frustumPlanes);

                cmdList->writeBuffer(mgr->m_cullParamsCB, &cb, sizeof(cb));

                // Create binding set (u2 = visibility buffer instead of draw args)
                nvrhi::BindingSetDesc bindDesc;
                bindDesc.bindings = {
                    nvrhi::BindingSetItem::ConstantBuffer(5, mgr->m_cullParamsCB),
                    nvrhi::BindingSetItem::StructuredBuffer_SRV(0, set.objectBuffer),
                    nvrhi::BindingSetItem::Texture_SRV(1, hizTexture),
                    nvrhi::BindingSetItem::Sampler(0, mgr->m_pointSampler),
                    nvrhi::BindingSetItem::StructuredBuffer_UAV(0, set.visibleIndexBuffer),
                    nvrhi::BindingSetItem::RawBuffer_UAV(1, set.visibleCountBuffer),
                    nvrhi::BindingSetItem::StructuredBuffer_UAV(2, set.visibilityBuffer)
                };

                nvrhi::BindingSetHandle bindingSet = nvDevice->createBindingSet(bindDesc, mgr->m_cullLayout);
                R_ASSERT2(bindingSet, "Failed to create culling binding set");

                // Set compute state and dispatch culling
                nvrhi::ComputeState state;
                state.pipeline = mgr->m_cullPipeline;
                state.bindings = { bindingSet };
                cmdList->setComputeState(state);

                u32 groupCount = (set.objectCount + CULL_THREAD_GROUP_SIZE - 1) / CULL_THREAD_GROUP_SIZE;
                cmdList->dispatch(groupCount, 1, 1);

                // ─────────────────────────────────────────────────────
                //  BATCH COMPACTION PASS
                // ─────────────────────────────────────────────────────
                if (mgr->m_compactEnabled) {
                    R_ASSERT2(set.drawArgsBuffer && set.materialIDBuffer &&
                                  set.compactDrawArgsBuffer && set.compactBatchIndicesBuffer &&
                                  set.compactMaterialIDBuffer && set.compactCountBuffer,
                        "Compaction buffers not initialized");

                    cmdList->setBufferState(set.drawArgsBuffer, nvrhi::ResourceStates::ShaderResource);

                    u32 zeroCount = 0;
                    cmdList->writeBuffer(set.compactCountBuffer, &zeroCount, sizeof(u32));
                    cmdList->clearBufferUInt(set.compactDrawArgsBuffer, 0);

                    struct CompactParamsCB {
                        u32 batchCount;
                        u32 padding[3];
                    };
                    CompactParamsCB compactCB;
                    compactCB.batchCount = set.objectCount;
                    compactCB.padding[0] = compactCB.padding[1] = compactCB.padding[2] = 0;
                    cmdList->writeBuffer(mgr->m_compactParamsCB, &compactCB, sizeof(compactCB));

                    nvrhi::BindingSetDesc compactBindDesc;
                    compactBindDesc.bindings = {
                        nvrhi::BindingSetItem::ConstantBuffer(5, mgr->m_compactParamsCB),
                        nvrhi::BindingSetItem::RawBuffer_SRV(0, set.drawArgsBuffer),
                        nvrhi::BindingSetItem::StructuredBuffer_SRV(1, set.materialIDBuffer),
                        nvrhi::BindingSetItem::StructuredBuffer_SRV(2, set.visibilityBuffer),
                        nvrhi::BindingSetItem::RawBuffer_UAV(0, set.compactDrawArgsBuffer),
                        nvrhi::BindingSetItem::StructuredBuffer_UAV(1, set.compactBatchIndicesBuffer),
                        nvrhi::BindingSetItem::RawBuffer_UAV(2, set.compactCountBuffer),
                        nvrhi::BindingSetItem::StructuredBuffer_UAV(3, set.compactMaterialIDBuffer)
                    };

                    nvrhi::BindingSetHandle compactBindingSet = nvDevice->createBindingSet(compactBindDesc, mgr->m_compactLayout);
                    R_ASSERT2(compactBindingSet, "Failed to create compaction binding set");

                    nvrhi::ComputeState compactState;
                    compactState.pipeline = mgr->m_compactPipeline;
                    compactState.bindings = { compactBindingSet };
                    cmdList->setComputeState(compactState);
                    cmdList->dispatch(groupCount, 1, 1);

                    cmdList->setBufferState(set.compactDrawArgsBuffer, nvrhi::ResourceStates::IndirectArgument);
                    cmdList->setBufferState(set.compactCountBuffer, nvrhi::ResourceStates::IndirectArgument);
                }
            };

            dispatchCullSet(mgr->m_staticSet);
            dispatchCullSet(mgr->m_dynamicSet);

            // ─────────────────────────────────────────────────────
            //  TERRAIN CULLING PASS (uses same shader, different data)
            // ─────────────────────────────────────────────────────
            if (mgr->m_terrainObjectCount > 0) {
                R_ASSERT2(mgr->m_terrainObjectBuffer && mgr->m_terrainDrawArgsBuffer,
                    "Terrain buffers not initialized for culling");
            }

            if (mgr->m_terrainObjectCount > 0 && mgr->m_terrainObjectBuffer && mgr->m_terrainDrawArgsBuffer) {
                // Clear terrain visible count and visibility buffer
                u32 zeroTerrain = 0;
                cmdList->writeBuffer(mgr->m_terrainVisibleCountBuffer, &zeroTerrain, sizeof(u32));
                cmdList->clearBufferUInt(mgr->m_terrainVisibilityBuffer, 0);

                // Update constant buffer for terrain (reuse same CB, different object count)
                CullParamsCB terrainCB;
                terrainCB.viewProj.transpose(Device.mFullTransform);
                terrainCB.prevViewProj.transpose(data.prevViewProj);  // Previous frame for temporal Hi-Z
                terrainCB.cameraPos = Device.vCameraPosition;
                float farPlane = g_pGamePersistent ? g_pGamePersistent->Environment().CurrentEnv.far_plane : 300.0f;
                terrainCB.maxDistanceSq = farPlane * farPlane;
                terrainCB.objectCount = mgr->m_terrainObjectCount;
                terrainCB.hizWidth = data.hizWidth;
                terrainCB.hizHeight = data.hizHeight;
                terrainCB.hizMipLevels = data.hizMipLevels;
                mgr->ExtractFrustumPlanes(Device.mFullTransform, terrainCB.frustumPlanes);
                cmdList->writeBuffer(mgr->m_cullParamsCB, &terrainCB, sizeof(terrainCB));

                // Create terrain binding set (same layout as regular geometry, uses visibility buffer)
                nvrhi::BindingSetDesc terrainBindDesc;
                terrainBindDesc.bindings = {
                    nvrhi::BindingSetItem::ConstantBuffer(5, mgr->m_cullParamsCB),
                    nvrhi::BindingSetItem::StructuredBuffer_SRV(0, mgr->m_terrainObjectBuffer),
                    nvrhi::BindingSetItem::Texture_SRV(1, hizTexture),
                    nvrhi::BindingSetItem::Sampler(0, mgr->m_pointSampler),
                    nvrhi::BindingSetItem::StructuredBuffer_UAV(0, mgr->m_terrainVisibleIndexBuffer),
                    nvrhi::BindingSetItem::RawBuffer_UAV(1, mgr->m_terrainVisibleCountBuffer),
                    nvrhi::BindingSetItem::StructuredBuffer_UAV(2, mgr->m_terrainVisibilityBuffer)  // Visibility buffer (like regular geometry)
                };

                nvrhi::BindingSetHandle terrainBindingSet = nvDevice->createBindingSet(terrainBindDesc, mgr->m_cullLayout);
                R_ASSERT2(terrainBindingSet, "Terrain culling binding set creation failed");

                nvrhi::ComputeState terrainState;
                terrainState.pipeline = mgr->m_cullPipeline;
                terrainState.bindings = { terrainBindingSet };
                cmdList->setComputeState(terrainState);

                u32 terrainGroupCount = (mgr->m_terrainObjectCount + CULL_THREAD_GROUP_SIZE - 1) / CULL_THREAD_GROUP_SIZE;
                cmdList->dispatch(terrainGroupCount, 1, 1);

                // ─────────────────────────────────────────────────────
                //  TERRAIN COMPACTION PASS
                // ─────────────────────────────────────────────────────
                R_ASSERT2(mgr->m_compactEnabled, "Terrain compaction requires batch compaction to be enabled");
                R_ASSERT2(mgr->m_compactPipeline && mgr->m_compactLayout, "Terrain compaction pipeline not initialized");
                R_ASSERT2(mgr->m_terrainCompactDrawArgsBuffer && mgr->m_terrainCompactBatchIndicesBuffer &&
                              mgr->m_terrainCompactMaterialIDBuffer && mgr->m_terrainCompactCountBuffer,
                    "Terrain compaction buffers not initialized");
                R_ASSERT2(mgr->m_terrainMaterialIDBuffer, "Terrain material ID buffer missing");

                // Transition terrain draw args to SRV for compaction input
                cmdList->beginTrackingBufferState(mgr->m_terrainDrawArgsBuffer, nvrhi::ResourceStates::IndirectArgument);
                cmdList->setBufferState(mgr->m_terrainDrawArgsBuffer, nvrhi::ResourceStates::ShaderResource);
                cmdList->beginTrackingBufferState(mgr->m_terrainMaterialIDBuffer, nvrhi::ResourceStates::ShaderResource);

                // Clear compact count and draw args
                u32 zeroTerrainCount = 0;
                cmdList->writeBuffer(mgr->m_terrainCompactCountBuffer, &zeroTerrainCount, sizeof(u32));
                cmdList->clearBufferUInt(mgr->m_terrainCompactDrawArgsBuffer, 0);

                // Update compact params (reuse same CB)
                struct CompactParamsCB {
                    u32 batchCount;
                    u32 padding[3];
                };
                CompactParamsCB terrainCompactCB;
                terrainCompactCB.batchCount = mgr->m_terrainObjectCount;
                terrainCompactCB.padding[0] = terrainCompactCB.padding[1] = terrainCompactCB.padding[2] = 0;
                cmdList->writeBuffer(mgr->m_compactParamsCB, &terrainCompactCB, sizeof(terrainCompactCB));

                // Create binding set for terrain compaction
                nvrhi::BindingSetDesc terrainCompactBindDesc;
                terrainCompactBindDesc.bindings = {
                    nvrhi::BindingSetItem::ConstantBuffer(5, mgr->m_compactParamsCB),
                    nvrhi::BindingSetItem::RawBuffer_SRV(0, mgr->m_terrainDrawArgsBuffer),
                    nvrhi::BindingSetItem::StructuredBuffer_SRV(1, mgr->m_terrainMaterialIDBuffer),
                    nvrhi::BindingSetItem::StructuredBuffer_SRV(2, mgr->m_terrainVisibilityBuffer),
                    nvrhi::BindingSetItem::RawBuffer_UAV(0, mgr->m_terrainCompactDrawArgsBuffer),
                    nvrhi::BindingSetItem::StructuredBuffer_UAV(1, mgr->m_terrainCompactBatchIndicesBuffer),
                    nvrhi::BindingSetItem::RawBuffer_UAV(2, mgr->m_terrainCompactCountBuffer),
                    nvrhi::BindingSetItem::StructuredBuffer_UAV(3, mgr->m_terrainCompactMaterialIDBuffer)
                };

                nvrhi::BindingSetHandle terrainCompactBindingSet = nvDevice->createBindingSet(terrainCompactBindDesc, mgr->m_compactLayout);
                R_ASSERT2(terrainCompactBindingSet, "Terrain compaction binding set creation failed");

                nvrhi::ComputeState terrainCompactState;
                terrainCompactState.pipeline = mgr->m_compactPipeline;
                terrainCompactState.bindings = { terrainCompactBindingSet };
                cmdList->setComputeState(terrainCompactState);
                cmdList->dispatch(terrainGroupCount, 1, 1);

                // Transition compact buffers to IndirectArgument for DrawIndexedIndirectCount
                cmdList->setBufferState(mgr->m_terrainCompactDrawArgsBuffer, nvrhi::ResourceStates::IndirectArgument);
                cmdList->setBufferState(mgr->m_terrainCompactCountBuffer, nvrhi::ResourceStates::IndirectArgument);
            }
        }
    );

    output.visibleIndices = VirtualResourceHandle();  // Not using framegraph for these
    output.visibleCount = VirtualResourceHandle();
    output.drawArgsBuffer = passData.staticDrawArgsBuffer;
    output.staticDrawArgsBuffer = passData.staticDrawArgsBuffer;
    output.dynamicDrawArgsBuffer = passData.dynamicDrawArgsBuffer;
    output.staticObjectCount = m_staticSet.objectCount;
    output.dynamicObjectCount = m_dynamicSet.objectCount;

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

        output.staticCompactDrawArgs = fg.ImportBuffer("gpu_cull_static_compact_drawargs", m_staticSet.compactDrawArgsBuffer, compactArgsDesc);
        output.staticCompactBatchIndices = fg.ImportBuffer("gpu_cull_static_compact_batchindices", m_staticSet.compactBatchIndicesBuffer, compactIndicesDesc);

        output.dynamicCompactDrawArgs = fg.ImportBuffer("gpu_cull_dynamic_compact_drawargs", m_dynamicSet.compactDrawArgsBuffer, compactArgsDesc);
        output.dynamicCompactBatchIndices = fg.ImportBuffer("gpu_cull_dynamic_compact_batchindices", m_dynamicSet.compactBatchIndicesBuffer, compactIndicesDesc);
    } else {
        output.staticCompactDrawArgs = VirtualResourceHandle();
        output.staticCompactBatchIndices = VirtualResourceHandle();
        output.dynamicCompactDrawArgs = VirtualResourceHandle();
        output.dynamicCompactBatchIndices = VirtualResourceHandle();
    }

    // ───────────────────────────────────────────────────────
    //  TERRAIN OUTPUT HANDLES
    // ───────────────────────────────────────────────────────
    output.terrainObjectCount = m_terrainObjectCount;

    if (m_terrainObjectCount > 0) {
        R_ASSERT2(m_terrainDrawArgsBuffer, "Terrain draw args buffer not initialized");
        R_ASSERT2(m_compactEnabled, "Terrain compaction requires batch compaction to be enabled");
        R_ASSERT2(m_terrainCompactDrawArgsBuffer && m_terrainCompactBatchIndicesBuffer &&
                      m_terrainCompactMaterialIDBuffer && m_terrainCompactCountBuffer,
            "Terrain compaction buffers not initialized");

        // Import terrain draw args into framegraph
        ResourceDesc terrainArgsDesc;
        terrainArgsDesc.type = ResourceDesc::Type::Buffer;
        terrainArgsDesc.debugName = "GPUCull_TerrainDrawArgs";
        terrainArgsDesc.bufferSize = m_maxTerrainObjects * sizeof(IndirectDrawArgs);
        terrainArgsDesc.isUAV = true;
        terrainArgsDesc.isTransient = false;

        output.terrainDrawArgsBuffer = fg.ImportBuffer("gpu_cull_terrain_drawargs", m_terrainDrawArgsBuffer, terrainArgsDesc);

        // Import terrain compact buffers for GPU-driven draw count
        ResourceDesc terrainCompactArgsDesc;
        terrainCompactArgsDesc.type = ResourceDesc::Type::Buffer;
        terrainCompactArgsDesc.debugName = "GPUCull_TerrainCompactDrawArgs";
        terrainCompactArgsDesc.bufferSize = m_maxTerrainObjects * sizeof(IndirectDrawArgs);
        terrainCompactArgsDesc.isUAV = true;
        terrainCompactArgsDesc.isTransient = false;

        ResourceDesc terrainCompactIndicesDesc;
        terrainCompactIndicesDesc.type = ResourceDesc::Type::Buffer;
        terrainCompactIndicesDesc.debugName = "GPUCull_TerrainCompactBatchIndices";
        terrainCompactIndicesDesc.bufferSize = m_maxTerrainObjects * sizeof(u32);
        terrainCompactIndicesDesc.structStride = sizeof(u32);
        terrainCompactIndicesDesc.isUAV = true;
        terrainCompactIndicesDesc.isTransient = false;

        ResourceDesc terrainCompactMaterialDesc;
        terrainCompactMaterialDesc.type = ResourceDesc::Type::Buffer;
        terrainCompactMaterialDesc.debugName = "GPUCull_TerrainCompactMaterialIDs";
        terrainCompactMaterialDesc.bufferSize = m_maxTerrainObjects * sizeof(u32);
        terrainCompactMaterialDesc.structStride = sizeof(u32);
        terrainCompactMaterialDesc.isUAV = true;
        terrainCompactMaterialDesc.isTransient = false;

        ResourceDesc terrainCompactCountDesc;
        terrainCompactCountDesc.type = ResourceDesc::Type::Buffer;
        terrainCompactCountDesc.debugName = "GPUCull_TerrainCompactCount";
        terrainCompactCountDesc.bufferSize = sizeof(u32);
        terrainCompactCountDesc.isUAV = true;
        terrainCompactCountDesc.isTransient = false;

        output.terrainCompactDrawArgs = fg.ImportBuffer("gpu_cull_terrain_compact_drawargs", m_terrainCompactDrawArgsBuffer, terrainCompactArgsDesc);
        output.terrainCompactBatchIndices = fg.ImportBuffer("gpu_cull_terrain_compact_batchindices", m_terrainCompactBatchIndicesBuffer, terrainCompactIndicesDesc);
        output.terrainCompactMaterialIDs = fg.ImportBuffer("gpu_cull_terrain_compact_materialids", m_terrainCompactMaterialIDBuffer, terrainCompactMaterialDesc);
        output.terrainCompactCount = fg.ImportBuffer("gpu_cull_terrain_compact_count", m_terrainCompactCountBuffer, terrainCompactCountDesc);
    } else {
        output.terrainDrawArgsBuffer = VirtualResourceHandle();
        output.terrainCompactDrawArgs = VirtualResourceHandle();
        output.terrainCompactBatchIndices = VirtualResourceHandle();
        output.terrainCompactMaterialIDs = VirtualResourceHandle();
        output.terrainCompactCount = VirtualResourceHandle();
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

    u32 totalObjectCount = m_staticSet.objectCount + m_dynamicSet.objectCount;
    if (!IsDebugEnabled() || (totalObjectCount == 0 && particleCount == 0))
        return;

    struct DebugPassData {
        VirtualResourceHandle hizPyramid;
        VirtualResourceHandle colorTarget;
        VirtualResourceHandle depthTarget;

        GPUCullingManager* manager;
        const xr_vector<passes::ParticleBatch>* particleBatches;
        u32 objectCount;
        u32 staticCount;
        u32 dynamicCount;
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
            data.objectCount = totalObjectCount;
            data.staticCount = m_staticSet.objectCount;
            data.dynamicCount = m_dynamicSet.objectCount;
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

            auto dispatchDebugObjects = [&](nvrhi::IBuffer* objectBuffer, u32 objectCount, u32 debugOffset) {
                if (!objectBuffer || objectCount == 0)
                    return;

                CullDebugParamsCB cb;
                cb.viewProj.transpose(Device.mFullTransform);
                cb.cameraPos = Device.vCameraPosition;
                cb.maxDistanceSq = farPlane * farPlane;
                cb.objectCount = objectCount;
                cb.hizWidth = data.hizWidth;
                cb.hizHeight = data.hizHeight;
                cb.hizMipLevels = data.hizMipLevels;
                cb.occluderThreshold = 50.0f;
                cb.debugOffset = debugOffset;

                mgr->ExtractFrustumPlanes(Device.mFullTransform, cb.frustumPlanes);
                cmdList->writeBuffer(mgr->m_debugComputeParamsCB, &cb, sizeof(cb));

                nvrhi::BindingSetDesc bindDesc;
                bindDesc.bindings = {
                    nvrhi::BindingSetItem::ConstantBuffer(5, mgr->m_debugComputeParamsCB),
                    nvrhi::BindingSetItem::StructuredBuffer_SRV(0, objectBuffer),
                    nvrhi::BindingSetItem::Texture_SRV(1, hizTexture),
                    nvrhi::BindingSetItem::Sampler(0, mgr->m_pointSampler),
                    nvrhi::BindingSetItem::StructuredBuffer_UAV(0, mgr->m_debugBuffer)
                };

                nvrhi::BindingSetHandle bindingSet = nvDevice->createBindingSet(bindDesc, mgr->m_debugComputeLayout);
                R_ASSERT2(bindingSet, "Debug binding set creation failed");

                nvrhi::ComputeState state;
                state.pipeline = mgr->m_debugComputePipeline;
                state.bindings = { bindingSet };
                cmdList->setComputeState(state);

                u32 groupCount = (objectCount + CULL_THREAD_GROUP_SIZE - 1) / CULL_THREAD_GROUP_SIZE;
                cmdList->dispatch(groupCount, 1, 1);
            };

            dispatchDebugObjects(mgr->m_staticSet.objectBuffer, data.staticCount, 0);
            dispatchDebugObjects(mgr->m_dynamicSet.objectBuffer, data.dynamicCount, data.staticCount);

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

    InvalidateStaticCullingData();

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

    // Create instance buffers (sized for max objects)
    {
        nvrhi::BufferDesc desc;
        desc.byteSize = m_maxObjects * sizeof(GPUInstanceData);
        desc.structStride = sizeof(GPUInstanceData);
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;

        desc.debugName = "GPUCull_StaticInstanceData";
        m_staticSet.instanceBuffer = nvDevice->createBuffer(desc);
        R_ASSERT2(m_staticSet.instanceBuffer, "Failed to create static instance buffer");

        desc.debugName = "GPUCull_DynamicInstanceData";
        m_dynamicSet.instanceBuffer = nvDevice->createBuffer(desc);
        R_ASSERT2(m_dynamicSet.instanceBuffer, "Failed to create dynamic instance buffer");
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
    (void)ctx;
    (void)geometry;
    // Instance data uploads are handled in UploadSceneObjects() for static/dynamic sets.
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
