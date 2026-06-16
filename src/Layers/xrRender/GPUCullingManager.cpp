// xrRender/GPUCullingManager.cpp
#include "stdafx.h"
#include "GPUCullingManager.h"
#include "xrCore/Profiler/Profiler.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/Geometry/GeometryBatch.h"
#include "Layers/xrRender/xrRender_console.h"
#include "Layers/xrRender/FrameGraphPasses/ParticlePassSetup.h"
#include "Layers/xrRender/FBasicVisual.h"
#include "Layers/xrRender/Bindless/VertexConverter.h"
#include "Layers/xrRender/SkeletonCustom.h"  // For CKinematics bone access
#include "Layers/xrRender/ShaderVariant/ShaderVariantRegistry.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/Bindless/MaterialBuffer.h"
#include "Layers/xrRender/Bindless/TerrainMaterialBuffer.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/RayTracing/RTAccelStructManager.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"

namespace fg
{
    extern xray::render::FrameGraphRenderer RImplementation;
}

namespace xray::render::fg {

// ═══════════════════════════════════════════════════════
//  CONSTANTS
// ═══════════════════════════════════════════════════════

constexpr u32 MAX_CULLING_OBJECTS = 65536;  // Maximum objects per frame
constexpr u32 CULL_THREAD_GROUP_SIZE = 64;  // Must match [numthreads] in shader
constexpr u32 COMPACT_THREAD_GROUP_SIZE = 256;  // Must match batch_compact_* shaders

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
    u32 frameId;                // Frame stamp for visibility
    u32 padding[3];
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

void GPUCullingManager::Initialize(fg::RenderDevice* device)
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
    auto cullResult = GEnv.Render->GetShaderLoader()->LoadComputeShader("object_cull");
    if (cullResult.handle) {
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
    CreateVariantPartitionResources(device);
    CreateDebugResources(device);
    CreateParticleResources(device);

    m_initialized = true;
    Msg("* [GPUCulling] Initialized (max objects: %d, max particles: %d, compact: %s)",
        m_maxObjects, m_maxParticles, m_compactEnabled ? "yes" : "no");
}

void GPUCullingManager::CreateBuffers(fg::RenderDevice* device)
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
        fg::RenderDevice::BufferDesc desc;
        desc.debugName = "GPUCull_Params";
        desc.byteSize = sizeof(CullParamsCB);
        desc.isConstantBuffer = true;
        desc.isVolatile = true;
        desc.maxVersions = 512;

        m_cullParamsCB = m_device->CreateBuffer(desc);
        if (!m_cullParamsCB.IsValid()) {
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

    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_TerrainObjects";
        desc.byteSize = m_maxTerrainObjects * sizeof(GPUObjectData);
        desc.structStride = sizeof(GPUObjectData);
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;

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

    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_TerrainDrawArgs";
        desc.byteSize = m_maxTerrainObjects * sizeof(IndirectDrawArgs);
        desc.canHaveUAVs = true;
        desc.canHaveRawViews = true;
        desc.isDrawIndirectArgs = true;
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;

        m_terrainDrawArgsBuffer = nvDevice->createBuffer(desc);
    }

    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_TerrainMaterialIDs";
        desc.byteSize = m_maxTerrainObjects * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;

        m_terrainMaterialIDBuffer = nvDevice->createBuffer(desc);
    }

    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_TerrainInstanceData";
        desc.byteSize = m_maxTerrainObjects * sizeof(GPUInstanceData);
        desc.structStride = sizeof(GPUInstanceData);
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;

        m_terrainInstanceBuffer = nvDevice->createBuffer(desc);
    }

    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_TerrainBatchIndices";
        desc.byteSize = m_maxTerrainObjects * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;

        m_terrainBatchIndicesBuffer = nvDevice->createBuffer(desc);
    }

    Msg("* [GPUCulling] Terrain buffers created (max: %u objects)", m_maxTerrainObjects);

    // ───────────────────────────────────────────────────────
    //  TRANSPARENT CULLING SET (reuses CullSetBuffers pattern)
    // ───────────────────────────────────────────────────────
    {
        const u32 maxTransparent = 4096;
        m_transparentSet.maxObjects = maxTransparent;

        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_Transparent_Objects";
        desc.byteSize = maxTransparent * sizeof(GPUObjectData);
        desc.structStride = sizeof(GPUObjectData);
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        m_transparentSet.objectBuffer = nvDevice->createBuffer(desc);

        desc = {};
        desc.debugName = "GPUCull_Transparent_VisibleIndices";
        desc.byteSize = maxTransparent * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.canHaveUAVs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        m_transparentSet.visibleIndexBuffer = nvDevice->createBuffer(desc);

        desc = {};
        desc.debugName = "GPUCull_Transparent_VisibleCount";
        desc.byteSize = sizeof(u32);
        desc.canHaveUAVs = true;
        desc.canHaveRawViews = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        m_transparentSet.visibleCountBuffer = nvDevice->createBuffer(desc);

        desc = {};
        desc.debugName = "GPUCull_Transparent_DrawArgs";
        desc.byteSize = maxTransparent * sizeof(IndirectDrawArgs);
        desc.canHaveUAVs = true;
        desc.canHaveRawViews = true;
        desc.isDrawIndirectArgs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        m_transparentSet.drawArgsBuffer = nvDevice->createBuffer(desc);

        desc = {};
        desc.debugName = "GPUCull_Transparent_Visibility";
        desc.byteSize = maxTransparent * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.canHaveUAVs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        m_transparentSet.visibilityBuffer = nvDevice->createBuffer(desc);

        desc = {};
        desc.debugName = "GPUCull_Transparent_MaterialIDs";
        desc.byteSize = maxTransparent * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        m_transparentSet.materialIDBuffer = nvDevice->createBuffer(desc);

        desc = {};
        desc.debugName = "GPUCull_Transparent_InstanceData";
        desc.byteSize = maxTransparent * sizeof(GPUInstanceData);
        desc.structStride = sizeof(GPUInstanceData);
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        m_transparentSet.instanceBuffer = nvDevice->createBuffer(desc);

        Msg("* [GPUCulling] Transparent buffers created (max: %u objects)", maxTransparent);
    }

    // ───────────────────────────────────────────────────────
    //  SKINNED MESH CULLING BUFFERS
    // ───────────────────────────────────────────────────────
    CreateSkinnedCullingBuffers(device);
}

void GPUCullingManager::CreateSkinnedCullingBuffers(fg::RenderDevice* device)
{
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();

    m_maxSkinnedObjects = 1024;  // Typical scene has ~200 skinned meshes

    // Skinned object buffer (bounding spheres)
    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_SkinnedObjects";
        desc.byteSize = m_maxSkinnedObjects * sizeof(GPUObjectData);
        desc.structStride = sizeof(GPUObjectData);
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;

        m_skinnedObjectBuffer = nvDevice->createBuffer(desc);
        if (!m_skinnedObjectBuffer) {
            Msg("! [GPUCulling] Failed to create skinned object buffer");
            return;
        }
    }

    // Skinned visibility buffer (frame stamp per batch)
    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_SkinnedVisibility";
        desc.byteSize = m_maxSkinnedObjects * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.canHaveUAVs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;

        m_skinnedVisibilityBuffer = nvDevice->createBuffer(desc);
        if (!m_skinnedVisibilityBuffer) {
            Msg("! [GPUCulling] Failed to create skinned visibility buffer");
            return;
        }
    }

    // Skinned visibility readback double-buffer (CPU-readable)
    for (u32 i = 0; i < SKINNED_READBACK_FRAMES; ++i) {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_SkinnedVisibilityReadback";
        desc.byteSize = m_maxSkinnedObjects * sizeof(u32);
        desc.cpuAccess = nvrhi::CpuAccessMode::Read;
        desc.initialState = nvrhi::ResourceStates::CopyDest;
        desc.keepInitialState = true;

        m_skinnedReadbackBuffers[i] = nvDevice->createBuffer(desc);
        if (!m_skinnedReadbackBuffers[i]) {
            Msg("! [GPUCulling] Failed to create skinned visibility readback buffer %u", i);
            return;
        }
    }

    m_skinnedReadbackWriteIndex = 0;
    m_skinnedReadbackFrameCount = 0;

    // Reserve CPU-side vectors
    m_skinnedObjectData.reserve(m_maxSkinnedObjects);
    m_skinnedBatchPointers.reserve(m_maxSkinnedObjects);

    // Global bone buffer for GPU-driven skinned rendering
    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GlobalBoneBuffer";
        desc.byteSize = MAX_TOTAL_BONES * BONE_STRIDE;  // 8192 * 64 = 512KB
        desc.structStride = BONE_STRIDE;
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;

        m_globalBoneBuffer = nvDevice->createBuffer(desc);
        if (!m_globalBoneBuffer) {
            Msg("! [GPUCulling] Failed to create global bone buffer");
            return;
        }
    }

    // Pre-allocate staging buffer for max skeleton size
    m_boneStagingBuffer.resize(256);  // Max bones per skeleton (typically 78)
    m_boneBufferInitialized = true;

    m_skinnedCullEnabled = true;
    Msg("* [GPUCulling] Skinned culling buffers created (max: %u objects, %u bones)",
        m_maxSkinnedObjects, MAX_TOTAL_BONES);
}

void GPUCullingManager::EnsureSkinnedBufferCapacity(u32 count)
{
    if (count <= m_maxSkinnedObjects)
        return;

    // Grow by 2x
    u32 newCapacity = std::max(count, m_maxSkinnedObjects * 2);
    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();

    Msg("* [GPUCulling] Growing skinned buffers: %u -> %u", m_maxSkinnedObjects, newCapacity);

    // Recreate object buffer
    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_SkinnedObjects";
        desc.byteSize = newCapacity * sizeof(GPUObjectData);
        desc.structStride = sizeof(GPUObjectData);
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;

        m_skinnedObjectBuffer = nvDevice->createBuffer(desc);
    }

    // Recreate visibility buffer
    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_SkinnedVisibility";
        desc.byteSize = newCapacity * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.canHaveUAVs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;

        m_skinnedVisibilityBuffer = nvDevice->createBuffer(desc);
    }

    // Recreate readback double-buffer
    for (u32 i = 0; i < SKINNED_READBACK_FRAMES; ++i) {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_SkinnedVisibilityReadback";
        desc.byteSize = newCapacity * sizeof(u32);
        desc.cpuAccess = nvrhi::CpuAccessMode::Read;
        desc.initialState = nvrhi::ResourceStates::CopyDest;
        desc.keepInitialState = true;

        m_skinnedReadbackBuffers[i] = nvDevice->createBuffer(desc);
    }
    m_skinnedReadbackWriteIndex = 0;
    m_skinnedReadbackFrameCount = 0;

    for (u32 i = 0; i < SKINNED_READBACK_FRAMES; ++i) {
        m_skinnedReadbackSubmitFrame[i] = 0;
        m_skinnedReadbackCounts[i] = 0;
    }
    m_skinnedVisibilityValues.clear();
    m_skinnedVisibilityFrame = 0;

    m_maxSkinnedObjects = newCapacity;
}

void GPUCullingManager::CreateComputePipeline(fg::RenderDevice* device)
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
        auto& cache = framegraph::GetPassResourceCache();
        auto* objectCullRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("object_cull", ".cs");
        m_cullLayout = cache.GetOrCreateBindingLayoutFromReflection("GPUCull_ObjectCull", *objectCullRefl, nvDevice);
        if (!m_cullLayout) {
            Msg("! [GPUCulling] Failed to create binding layout");
            m_computeEnabled = false;
            return;
        }
    }

    // Create compute pipeline for main culling
    {
        nvrhi::ComputePipelineDesc pipeDesc;
        auto cullShader = GEnv.Render->GetShaderLoader()->LoadComputeShader("object_cull");
        pipeDesc.CS = cullShader.handle;
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

void GPUCullingManager::CreateCompactionResources(fg::RenderDevice* device)
{
    if (!m_computeEnabled)
        return;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();

    auto compactCountResult = GEnv.Render->GetShaderLoader()->LoadComputeShader("batch_compact_count");
    R_ASSERT2(compactCountResult.handle,
        "batch_compact_count.cs not found - compaction requires this shader");
    auto compactScanResult = GEnv.Render->GetShaderLoader()->LoadComputeShader("batch_compact_scan");
    R_ASSERT2(compactScanResult.handle,
        "batch_compact_scan.cs not found - compaction requires this shader");
    auto compactScatterResult = GEnv.Render->GetShaderLoader()->LoadComputeShader("batch_compact");
    R_ASSERT2(compactScatterResult.handle,
        "batch_compact.cs not found - compaction requires this shader");

    {
        u32 maxCompactGroups = (m_maxObjects + COMPACT_THREAD_GROUP_SIZE - 1) / COMPACT_THREAD_GROUP_SIZE;
        R_ASSERT2(maxCompactGroups > 0, "Compaction group count must be non-zero");
        R_ASSERT2(maxCompactGroups <= COMPACT_THREAD_GROUP_SIZE,
            "Compaction group count exceeds scan group size");
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
                desc.isDrawIndirectArgs = true;
                desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
                desc.keepInitialState = true;

                set.compactCountBuffer = nvDevice->createBuffer(desc);
                R_ASSERT2(set.compactCountBuffer, "Failed to create compact count buffer");
            }

            {
                nvrhi::BufferDesc desc;
                desc.debugName = pickName("GPUCull_Static_CompactDispatchArgs", "GPUCull_Dynamic_CompactDispatchArgs");
                desc.byteSize = 3 * sizeof(u32);
                desc.canHaveUAVs = true;
                desc.canHaveRawViews = true;
                desc.isDrawIndirectArgs = true;
                desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
                desc.keepInitialState = true;

                set.compactDispatchArgsBuffer = nvDevice->createBuffer(desc);
                R_ASSERT2(set.compactDispatchArgsBuffer, "Failed to create compact dispatch args buffer");
            }

            {
                nvrhi::BufferDesc desc;
                desc.debugName = pickName("GPUCull_Static_CompactLocalPrefix", "GPUCull_Dynamic_CompactLocalPrefix");
                desc.byteSize = m_maxObjects * sizeof(u32);
                desc.structStride = sizeof(u32);
                desc.canHaveUAVs = true;
                desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
                desc.keepInitialState = true;

                set.compactLocalPrefixBuffer = nvDevice->createBuffer(desc);
                R_ASSERT2(set.compactLocalPrefixBuffer, "Failed to create compact local prefix buffer");
            }

            {
                nvrhi::BufferDesc desc;
                desc.debugName = pickName("GPUCull_Static_CompactGroupCounts", "GPUCull_Dynamic_CompactGroupCounts");
                desc.byteSize = maxCompactGroups * sizeof(u32);
                desc.structStride = sizeof(u32);
                desc.canHaveUAVs = true;
                desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
                desc.keepInitialState = true;

                set.compactGroupCountsBuffer = nvDevice->createBuffer(desc);
                R_ASSERT2(set.compactGroupCountsBuffer, "Failed to create compact group counts buffer");
            }

            {
                nvrhi::BufferDesc desc;
                desc.debugName = pickName("GPUCull_Static_CompactGroupOffsets", "GPUCull_Dynamic_CompactGroupOffsets");
                desc.byteSize = maxCompactGroups * sizeof(u32);
                desc.structStride = sizeof(u32);
                desc.canHaveUAVs = true;
                desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
                desc.keepInitialState = true;

                set.compactGroupOffsetsBuffer = nvDevice->createBuffer(desc);
                R_ASSERT2(set.compactGroupOffsetsBuffer, "Failed to create compact group offsets buffer");
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
        desc.isDrawIndirectArgs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;

        m_terrainCompactCountBuffer = nvDevice->createBuffer(desc);
        R_ASSERT2(m_terrainCompactCountBuffer, "Failed to create terrain compact count buffer");
    }

    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_TerrainCompactDispatchArgs";
        desc.byteSize = 3 * sizeof(u32);
        desc.canHaveUAVs = true;
        desc.canHaveRawViews = true;
        desc.isDrawIndirectArgs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;

        m_terrainCompactDispatchArgsBuffer = nvDevice->createBuffer(desc);
        R_ASSERT2(m_terrainCompactDispatchArgsBuffer, "Failed to create terrain compact dispatch args buffer");
    }

    {
        u32 maxTerrainGroups = (m_maxTerrainObjects + COMPACT_THREAD_GROUP_SIZE - 1) / COMPACT_THREAD_GROUP_SIZE;
        R_ASSERT2(maxTerrainGroups > 0, "Terrain compaction group count must be non-zero");
        R_ASSERT2(maxTerrainGroups <= COMPACT_THREAD_GROUP_SIZE,
            "Terrain compaction group count exceeds scan group size");

        {
            nvrhi::BufferDesc desc;
            desc.debugName = "GPUCull_TerrainCompactLocalPrefix";
            desc.byteSize = m_maxTerrainObjects * sizeof(u32);
            desc.structStride = sizeof(u32);
            desc.canHaveUAVs = true;
            desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
            desc.keepInitialState = true;

            m_terrainCompactLocalPrefixBuffer = nvDevice->createBuffer(desc);
            R_ASSERT2(m_terrainCompactLocalPrefixBuffer, "Failed to create terrain compact local prefix buffer");
        }

        {
            nvrhi::BufferDesc desc;
            desc.debugName = "GPUCull_TerrainCompactGroupCounts";
            desc.byteSize = maxTerrainGroups * sizeof(u32);
            desc.structStride = sizeof(u32);
            desc.canHaveUAVs = true;
            desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
            desc.keepInitialState = true;

            m_terrainCompactGroupCountsBuffer = nvDevice->createBuffer(desc);
            R_ASSERT2(m_terrainCompactGroupCountsBuffer, "Failed to create terrain compact group counts buffer");
        }

        {
            nvrhi::BufferDesc desc;
            desc.debugName = "GPUCull_TerrainCompactGroupOffsets";
            desc.byteSize = maxTerrainGroups * sizeof(u32);
            desc.structStride = sizeof(u32);
            desc.canHaveUAVs = true;
            desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
            desc.keepInitialState = true;

            m_terrainCompactGroupOffsetsBuffer = nvDevice->createBuffer(desc);
            R_ASSERT2(m_terrainCompactGroupOffsetsBuffer, "Failed to create terrain compact group offsets buffer");
        }
    }

    // Transparent compaction buffers (same pattern as static/dynamic)
    {
        const u32 maxTrans = m_transparentSet.maxObjects;
        u32 maxTransGroups = (maxTrans + COMPACT_THREAD_GROUP_SIZE - 1) / COMPACT_THREAD_GROUP_SIZE;
        R_ASSERT2(maxTransGroups > 0, "Transparent compaction group count must be non-zero");
        R_ASSERT2(maxTransGroups <= COMPACT_THREAD_GROUP_SIZE, "Transparent compaction group count exceeds scan group size");

        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_Trans_CompactDrawArgs";
        desc.byteSize = maxTrans * sizeof(IndirectDrawArgs);
        desc.canHaveUAVs = true;
        desc.canHaveRawViews = true;
        desc.isDrawIndirectArgs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        m_transparentSet.compactDrawArgsBuffer = nvDevice->createBuffer(desc);

        desc = {};
        desc.debugName = "GPUCull_Trans_CompactBatchIndices";
        desc.byteSize = maxTrans * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.canHaveUAVs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        m_transparentSet.compactBatchIndicesBuffer = nvDevice->createBuffer(desc);

        desc = {};
        desc.debugName = "GPUCull_Trans_CompactMaterialIDs";
        desc.byteSize = maxTrans * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.canHaveUAVs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        m_transparentSet.compactMaterialIDBuffer = nvDevice->createBuffer(desc);

        desc = {};
        desc.debugName = "GPUCull_Trans_CompactCount";
        desc.byteSize = sizeof(u32);
        desc.canHaveUAVs = true;
        desc.canHaveRawViews = true;
        desc.isDrawIndirectArgs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        m_transparentSet.compactCountBuffer = nvDevice->createBuffer(desc);

        desc = {};
        desc.debugName = "GPUCull_Trans_CompactDispatchArgs";
        desc.byteSize = 3 * sizeof(u32);
        desc.canHaveUAVs = true;
        desc.canHaveRawViews = true;
        desc.isDrawIndirectArgs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        m_transparentSet.compactDispatchArgsBuffer = nvDevice->createBuffer(desc);

        desc = {};
        desc.debugName = "GPUCull_Trans_CompactLocalPrefix";
        desc.byteSize = maxTrans * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.canHaveUAVs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        m_transparentSet.compactLocalPrefixBuffer = nvDevice->createBuffer(desc);

        desc = {};
        desc.debugName = "GPUCull_Trans_CompactGroupCounts";
        desc.byteSize = maxTransGroups * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.canHaveUAVs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        m_transparentSet.compactGroupCountsBuffer = nvDevice->createBuffer(desc);

        desc = {};
        desc.debugName = "GPUCull_Trans_CompactGroupOffsets";
        desc.byteSize = maxTransGroups * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.canHaveUAVs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        m_transparentSet.compactGroupOffsetsBuffer = nvDevice->createBuffer(desc);
    }

    {
        fg::RenderDevice::BufferDesc desc;
        desc.debugName = "GPUCull_CompactParams";
        desc.byteSize = 16;
        desc.isConstantBuffer = true;
        desc.isVolatile = true;
        desc.maxVersions = 512;

        m_compactParamsCB = m_device->CreateBuffer(desc);
        R_ASSERT2(m_compactParamsCB.IsValid(), "Failed to create compact params CB");
    }

    {
        auto& cache = framegraph::GetPassResourceCache();
        auto* compactCountRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("batch_compact_count", ".cs");
        m_compactCountLayout = cache.GetOrCreateBindingLayoutFromReflection("GPUCull_CompactCount", *compactCountRefl, nvDevice);
        R_ASSERT2(m_compactCountLayout, "Failed to create compact count binding layout");
    }

    {
        auto& cache = framegraph::GetPassResourceCache();
        auto* compactScanRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("batch_compact_scan", ".cs");
        m_compactScanLayout = cache.GetOrCreateBindingLayoutFromReflection("GPUCull_CompactScan", *compactScanRefl, nvDevice);
        R_ASSERT2(m_compactScanLayout, "Failed to create compact scan binding layout");
    }

    {
        auto& cache = framegraph::GetPassResourceCache();
        auto* compactScatterRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("batch_compact", ".cs");
        m_compactScatterLayout = cache.GetOrCreateBindingLayoutFromReflection("GPUCull_CompactScatter", *compactScatterRefl, nvDevice);
        R_ASSERT2(m_compactScatterLayout, "Failed to create compact scatter binding layout");
    }

    {
        nvrhi::ComputePipelineDesc pipeDesc;
        pipeDesc.CS = GEnv.Render->GetShaderLoader()->LoadComputeShader("batch_compact_count").handle;
        pipeDesc.bindingLayouts = { m_compactCountLayout };

        m_compactCountPipeline = nvDevice->createComputePipeline(pipeDesc);
        R_ASSERT2(m_compactCountPipeline, "Failed to create compact count pipeline");
    }

    {
        nvrhi::ComputePipelineDesc pipeDesc;
        pipeDesc.CS = GEnv.Render->GetShaderLoader()->LoadComputeShader("batch_compact_scan").handle;
        pipeDesc.bindingLayouts = { m_compactScanLayout };

        m_compactScanPipeline = nvDevice->createComputePipeline(pipeDesc);
        R_ASSERT2(m_compactScanPipeline, "Failed to create compact scan pipeline");
    }

    {
        nvrhi::ComputePipelineDesc pipeDesc;
        pipeDesc.CS = GEnv.Render->GetShaderLoader()->LoadComputeShader("batch_compact").handle;
        pipeDesc.bindingLayouts = { m_compactScatterLayout };

        m_compactScatterPipeline = nvDevice->createComputePipeline(pipeDesc);
        R_ASSERT2(m_compactScatterPipeline, "Failed to create compact scatter pipeline");
    }

    m_compactEnabled = true;
    Msg("* [GPUCulling] Compaction resources created");

    // ───────────────────────────────────────────────────────
    //  TERRAIN APPLY VISIBILITY PIPELINE
    // ───────────────────────────────────────────────────────
    // Copies terrain visibility buffer → instanceCount in draw args
    // Simpler than full compaction since terrain doesn't need sorting
    auto terrainVisResult = GEnv.Render->GetShaderLoader()->LoadComputeShader("terrain_apply_visibility");
    if (terrainVisResult.handle) {
        auto& cache = framegraph::GetPassResourceCache();
        m_terrainApplyVisibilityLayout = cache.GetOrCreateBindingLayoutFromReflection("GPUCull_TerrainVisibility", *terrainVisResult.reflection, nvDevice);
        if (m_terrainApplyVisibilityLayout) {
            nvrhi::ComputePipelineDesc pipeDesc;
            pipeDesc.CS = terrainVisResult.handle;
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

void GPUCullingManager::CreateVariantPartitionResources(fg::RenderDevice* device)
{
    if (!m_compactEnabled)
        return;

    auto& registry = ShaderVariantRegistry::Instance();
    u32 variantCount = registry.GetVariantCount();
    if (variantCount <= 1) {
        Msg("* [GPUCulling] No shader variants loaded, partition disabled");
        return;
    }

    if (variantCount > MAX_SHADER_VARIANTS) {
        Msg("! [GPUCulling] Variant count %d exceeds MAX_SHADER_VARIANTS (%d), clamping", variantCount, MAX_SHADER_VARIANTS);
        variantCount = MAX_SHADER_VARIANTS;
    }

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();

    auto variantPartResult = GEnv.Render->GetShaderLoader()->LoadComputeShader("variant_partition");
    if (!variantPartResult.handle) {
        Msg("! [GPUCulling] variant_partition.cs not found - variant partition disabled");
        return;
    }

    {
        auto& cache = framegraph::GetPassResourceCache();
        m_variantPartitionLayout = cache.GetOrCreateBindingLayoutFromReflection("GPUCull_VariantPartition", *variantPartResult.reflection, nvDevice);
        R_ASSERT2(m_variantPartitionLayout, "Failed to create variant partition layout");
    }

    {
        nvrhi::ComputePipelineDesc pipeDesc;
        pipeDesc.CS = variantPartResult.handle;
        pipeDesc.bindingLayouts = { m_variantPartitionLayout };
        m_variantPartitionPipeline = nvDevice->createComputePipeline(pipeDesc);
        R_ASSERT2(m_variantPartitionPipeline, "Failed to create variant partition pipeline");
    }

    {
        fg::RenderDevice::BufferDesc desc;
        desc.debugName = "VariantPartition_Params";
        desc.byteSize = 16;
        desc.isConstantBuffer = true;
        desc.isVolatile = true;
        desc.maxVersions = fg::RenderDevice::BufferDesc::VOLATILE_CB_MAX_VERSIONS;
        m_variantPartitionParamsCB = m_device->CreateBuffer(desc);
        R_ASSERT2(m_variantPartitionParamsCB.IsValid(), "Failed to create variant partition params CB");
    }

    InitPartitionBuffers(nvDevice, m_staticPartition, "Static", variantCount, m_maxObjects);
    InitPartitionBuffers(nvDevice, m_transparentPartition, "Transparent", variantCount, m_maxObjects);

    m_variantPartitionEnabled = true;
    Msg("* [GPUCulling] Variant partition enabled (%d variants)", variantCount);
}

void GPUCullingManager::InitPartitionBuffers(
    nvrhi::IDevice* nvDevice, VariantPartitionBuffers& part,
    const char* prefix, u32 variantCount, u32 maxObjects)
{
    part.variantCount = variantCount;
    part.binCapacity = maxObjects;
    u32 totalSlots = variantCount * maxObjects;

    auto makeBuffer = [&](const char* suffix, nvrhi::BufferDesc desc) -> nvrhi::BufferHandle {
        desc.debugName = xr_string(prefix) + suffix;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        auto buf = nvDevice->createBuffer(desc);
        R_ASSERT2(buf, desc.debugName);
        return buf;
    };

    {
        nvrhi::BufferDesc desc;
        desc.byteSize = variantCount * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.canHaveUAVs = true;
        part.variantCountBuffer = makeBuffer("_VariantCounts", desc);
    }
    {
        nvrhi::BufferDesc desc;
        desc.byteSize = totalSlots * sizeof(IndirectDrawArgs);
        desc.canHaveUAVs = true;
        desc.canHaveRawViews = true;
        desc.isDrawIndirectArgs = true;
        part.reorderedDrawArgsBuffer = makeBuffer("_ReorderedDrawArgs", desc);
    }
    {
        nvrhi::BufferDesc desc;
        desc.byteSize = totalSlots * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.canHaveUAVs = true;
        part.reorderedBatchIndicesBuffer = makeBuffer("_ReorderedBatchIndices", desc);
    }
    {
        nvrhi::BufferDesc desc;
        desc.byteSize = totalSlots * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.canHaveUAVs = true;
        part.reorderedMaterialIDsBuffer = makeBuffer("_ReorderedMaterialIDs", desc);
    }
    {
        xr_vector<u32> drawIndices(totalSlots);
        for (u32 i = 0; i < totalSlots; i++)
            drawIndices[i] = i;

        nvrhi::BufferDesc desc;
        desc.byteSize = totalSlots * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.isVertexBuffer = true;
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        desc.debugName = xr_string(prefix) + "_PartitionDrawIndex";
        part.drawIndexBuffer = nvDevice->createBuffer(desc);
        R_ASSERT2(part.drawIndexBuffer, desc.debugName);

        if (GEnv.Backend)
            GEnv.Backend->UploadBufferData(part.drawIndexBuffer, drawIndices.data(), totalSlots * sizeof(u32));
    }

    Msg("* [GPUCulling] Variant partition buffers: %s (%d variants x %d cap = %d slots, %.1f MB)",
        prefix, variantCount, maxObjects, totalSlots,
        static_cast<float>(totalSlots * (sizeof(IndirectDrawArgs) + sizeof(u32) * 3)) / (1024.f * 1024.f));
}

void GPUCullingManager::DispatchVariantPartition(
    nvrhi::ICommandList* cmdList,
    nvrhi::IDevice* nvDevice,
    const CullSetBuffers& set,
    VariantPartitionBuffers& partition)
{
    auto& matBuffer = bindless::MaterialBuffer::Instance();
    nvrhi::IBuffer* materialBuffer = matBuffer.GetBuffer();
    if (!materialBuffer)
        return;

    u32 zeroData[MAX_SHADER_VARIANTS] = {};
    cmdList->writeBuffer(partition.variantCountBuffer, zeroData, partition.variantCount * sizeof(u32));

    cmdList->setBufferState(set.compactDrawArgsBuffer, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(set.compactBatchIndicesBuffer, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(set.compactMaterialIDBuffer, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(set.compactDrawArgsBuffer, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(set.compactBatchIndicesBuffer, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(set.compactMaterialIDBuffer, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(set.compactCountBuffer, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(set.compactDispatchArgsBuffer, nvrhi::ResourceStates::IndirectArgument);
    cmdList->setBufferState(materialBuffer, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(partition.variantCountBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(partition.reorderedDrawArgsBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(partition.reorderedBatchIndicesBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(partition.reorderedMaterialIDsBuffer, nvrhi::ResourceStates::UnorderedAccess);

    struct PartitionParamsCB {
        u32 binCapacity;
        u32 variantCount;
        u32 padding0;
        u32 padding1;
    };
    PartitionParamsCB params;
    params.binCapacity = partition.binCapacity;
    params.variantCount = partition.variantCount;
    params.padding0 = 0;
    params.padding1 = 0;
    cmdList->writeBuffer(m_device->GetNativeBuffer(m_variantPartitionParamsCB), &params, sizeof(params));

    auto* variantPartRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("variant_partition", ".cs");
    framegraph::BindingSetBuilder bsb(*variantPartRefl, nvDevice, "GPUCull.VariantPart");
    bsb.ConstantBuffer("PartitionParams", m_device->GetNativeBuffer(m_variantPartitionParamsCB))
       .BufferSRV("g_CompactCount", set.compactCountBuffer)
       .BufferSRV("g_CompactDrawArgs", set.compactDrawArgsBuffer)
       .BufferSRV("g_CompactBatchIndices", set.compactBatchIndicesBuffer)
       .BufferSRV("g_CompactMaterialIDs", set.compactMaterialIDBuffer)
       .BufferSRV("g_Materials", materialBuffer)
       .BufferUAV("g_VariantCounts", partition.variantCountBuffer)
       .BufferUAV("g_ReorderedDrawArgs", partition.reorderedDrawArgsBuffer)
       .BufferUAV("g_ReorderedBatchIndices", partition.reorderedBatchIndicesBuffer)
       .BufferUAV("g_ReorderedMaterialIDs", partition.reorderedMaterialIDsBuffer);
    nvrhi::BindingSetHandle bindingSet = framegraph::GetPassResourceCache().GetOrCreateBindingSet(bsb.Build(), m_variantPartitionLayout, nvDevice);
    R_ASSERT2(bindingSet, "Failed to create variant partition binding set");

    nvrhi::ComputeState state;
    state.pipeline = m_variantPartitionPipeline;
    state.bindings = { bindingSet };
    state.indirectParams = set.compactDispatchArgsBuffer;
    cmdList->setComputeState(state);
    cmdList->dispatchIndirect(0);

    cmdList->setBufferState(partition.variantCountBuffer, nvrhi::ResourceStates::IndirectArgument);
    cmdList->setBufferState(partition.reorderedDrawArgsBuffer, nvrhi::ResourceStates::IndirectArgument);
    cmdList->setBufferState(partition.reorderedBatchIndicesBuffer, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(partition.reorderedMaterialIDsBuffer, nvrhi::ResourceStates::ShaderResource);
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
    m_staticSet.compactDispatchArgsBuffer = nullptr;
    m_staticSet.compactLocalPrefixBuffer = nullptr;
    m_staticSet.compactGroupCountsBuffer = nullptr;
    m_staticSet.compactGroupOffsetsBuffer = nullptr;
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
    m_dynamicSet.compactDispatchArgsBuffer = nullptr;
    m_dynamicSet.compactLocalPrefixBuffer = nullptr;
    m_dynamicSet.compactGroupCountsBuffer = nullptr;
    m_dynamicSet.compactGroupOffsetsBuffer = nullptr;
    m_dynamicSet.instanceBuffer = nullptr;
    m_dynamicSet.objectCount = 0;
    m_dynamicSet.maxObjects = 0;
    m_dynamicSet.drawArgsUploaded = false;
    m_dynamicSet.objectsUploaded = false;

    m_transparentSet = {};

    m_cullParamsCB = fg::BufferHandle();
    m_cullPipeline = nullptr;
    m_cullLayout = nullptr;
    m_pointSampler = nullptr;

    m_compactParamsCB = fg::BufferHandle();
    m_compactCountPipeline = nullptr;
    m_compactScanPipeline = nullptr;
    m_compactScatterPipeline = nullptr;
    m_compactCountLayout = nullptr;
    m_compactScanLayout = nullptr;
    m_compactScatterLayout = nullptr;

    m_variantPartitionPipeline = nullptr;
    m_variantPartitionLayout = nullptr;
    m_variantPartitionParamsCB = fg::BufferHandle();
    m_staticPartition = {};
    m_transparentPartition = {};
    m_variantPartitionEnabled = false;

    m_debugBuffer = nullptr;
    m_debugComputeParamsCB = fg::BufferHandle();
    m_debugGraphicsParamsCB = fg::BufferHandle();
    m_debugComputePipeline = nullptr;
    m_particleDebugComputePipeline = nullptr;
    m_debugComputeLayout = nullptr;
    m_debugGraphicsPipeline = nullptr;
    m_debugGraphicsLayout = nullptr;
    m_debugInputLayout = nullptr;

    m_particleBuffer = nullptr;
    m_particleDrawArgsBuffer = nullptr;
    m_particleVisibleCountBuffer = nullptr;
    m_particleCullParamsCB = fg::BufferHandle();
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
    m_staticBatchVertexCounts.clear();
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
    m_terrainCompactDispatchArgsBuffer = nullptr;
    m_terrainCompactMaterialIDBuffer = nullptr;
    m_terrainCompactLocalPrefixBuffer = nullptr;
    m_terrainCompactGroupCountsBuffer = nullptr;
    m_terrainCompactGroupOffsetsBuffer = nullptr;
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

    // Stats readback
    for (u32 i = 0; i < STATS_READBACK_SLOTS; ++i)
        m_statsReadbackBuffers[i] = nullptr;
    m_statsWriteSlot = 0;
    m_statsScheduled = 0;
    m_cullingStats = CullingStats();
}

// ═══════════════════════════════════════════════════════
//  STATS READBACK (for profiling)
// ═══════════════════════════════════════════════════════

void GPUCullingManager::ScheduleStatsReadback(nvrhi::ICommandList* cmdList)
{
    if (!m_compactEnabled || !m_device)
        return;

    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();
    if (!nvDevice)
        return;

    // Create readback buffer on first use (3 u32s: static, dynamic, terrain)
    nvrhi::BufferHandle& slot = m_statsReadbackBuffers[m_statsWriteSlot];
    if (!slot)
    {
        nvrhi::BufferDesc desc;
        desc.byteSize = sizeof(u32) * 3;
        desc.debugName = "CullingStatsReadback";
        desc.cpuAccess = nvrhi::CpuAccessMode::Read;
        desc.initialState = nvrhi::ResourceStates::CopyDest;
        desc.keepInitialState = true;
        slot = nvDevice->createBuffer(desc);

        if (!slot)
            return;
    }

    // Copy compact count values to readback buffer
    // Static count at offset 0
    if (m_staticSet.compactCountBuffer)
    {
        cmdList->copyBuffer(
            slot, 0,
            m_staticSet.compactCountBuffer, 0,
            sizeof(u32)
        );
    }

    // Dynamic count at offset 4
    if (m_dynamicSet.compactCountBuffer)
    {
        cmdList->copyBuffer(
            slot, sizeof(u32),
            m_dynamicSet.compactCountBuffer, 0,
            sizeof(u32)
        );
    }

    // Terrain count at offset 8
    if (m_terrainCompactCountBuffer)
    {
        cmdList->copyBuffer(
            slot, sizeof(u32) * 2,
            m_terrainCompactCountBuffer, 0,
            sizeof(u32)
        );
    }

    m_statsWriteSlot = (m_statsWriteSlot + 1) % STATS_READBACK_SLOTS;
    if (m_statsScheduled < STATS_READBACK_SLOTS)
        ++m_statsScheduled;
}

void GPUCullingManager::ProcessStatsReadback()
{
    if (m_statsScheduled < STATS_READBACK_SLOTS || !m_device)
        return;

    // Only read back at the same interval as CPU profiler for consistency
    static u32 frameCounter = 0;
    frameCounter++;
    const u32 throttleInterval = xray::profiler::GetCPUProfiler().GetThrottleInterval();
    if ((frameCounter % throttleInterval) != 0)
        return;

    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();
    if (!nvDevice)
        return;

    // Map the readback buffer and read the values
    nvrhi::IBuffer* oldest = m_statsReadbackBuffers[m_statsWriteSlot];
    void* mappedData = nvDevice->mapBuffer(oldest, nvrhi::CpuAccessMode::Read);
    if (mappedData)
    {
        const u32* counts = static_cast<const u32*>(mappedData);
        m_cullingStats.staticVisible = counts[0];
        m_cullingStats.dynamicVisible = counts[1];
        m_cullingStats.terrainVisible = counts[2];
        nvDevice->unmapBuffer(oldest);
    }
}

// ═══════════════════════════════════════════════════════
//  UPLOAD SCENE OBJECTS
// ═══════════════════════════════════════════════════════

void GPUCullingManager::UploadSceneObjects(fg::RenderContext* ctx, const GeometryCollector* geometry)
{
    ZoneScopedN("GPUCull::UploadSceneObjects");

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
        m_staticBatchVertexCounts.clear();
        m_staticBatchVertexCounts.reserve(totalBatches);
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
    m_terrainObjectData.reserve(totalBatches / 4);
    m_terrainDrawArgsData.clear();
    m_terrainDrawArgsData.reserve(totalBatches / 4);
    m_terrainMaterialIDData.clear();
    m_terrainMaterialIDData.reserve(totalBatches / 4);
    m_terrainInstanceData.clear();
    m_terrainInstanceData.reserve(totalBatches / 4);

    // Transparent-specific arrays
    m_transparentObjectData.clear();
    m_transparentDrawArgsData.clear();
    m_transparentMaterialIDData.clear();
    m_transparentInstanceData.clear();

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
        inst.world = batch.worldMatrix;
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
            inst.world = batch.worldMatrix;
            inst.materialID = batch.terrainMaterialID;  // Terrain material ID
            inst.flags = GPU_OBJECT_OPAQUE;  // Terrain is always opaque
            inst.pad0 = 0.0f;
            inst.pad1 = 0.0f;
            m_terrainInstanceData.push_back(inst);
            continue;
        }

        if (batch.IsStrictB2F()) {
            appendBatch(batch, m_transparentObjectData, m_transparentDrawArgsData, m_transparentMaterialIDData, m_transparentInstanceData);
            continue;
        }

        if (batch.isStatic) {
            if (!m_staticDataCached) {
                appendBatch(batch, m_staticObjectData, m_staticDrawArgsData, m_staticMaterialIDData, m_staticInstanceData);
                m_staticBatchVertexCounts.push_back(batch.megaBufferAlloc.valid ? batch.megaBufferAlloc.vertexCount : 0);
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
    if (!m_megaDataUploaded && m_megaBuffersReady &&
        !m_megaVertices.empty() && !m_megaIndices.empty() &&
        m_megaVertexBuffer && m_megaIndexBuffer) {

        cmdList->writeBuffer(m_megaVertexBuffer,
            m_megaVertices.data(),
            m_megaVertices.size() * sizeof(bindless::UnifiedVertex));

        cmdList->writeBuffer(m_megaIndexBuffer,
            m_megaIndices.data(),
            m_megaIndices.size() * sizeof(u32));

        m_megaDataUploaded = true;

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
            cmdList->writeBuffer(m_terrainDrawArgsBuffer,
                m_terrainDrawArgsData.data(),
                m_terrainObjectCount * sizeof(IndirectDrawArgs));
            cmdList->setBufferState(m_terrainDrawArgsBuffer, nvrhi::ResourceStates::IndirectArgument);

            // Upload terrain material IDs
            if (m_terrainMaterialIDBuffer) {
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
                cmdList->writeBuffer(m_terrainBatchIndicesBuffer,
                    identityIndices.data(),
                    m_terrainObjectCount * sizeof(u32));
                cmdList->setBufferState(m_terrainBatchIndicesBuffer, nvrhi::ResourceStates::ShaderResource);
            }

            m_staticTerrainDrawArgsUploaded = true;
            Msg("* [GPUCulling] Static terrain draw args uploaded: %u objects (visibility buffer mode)", m_terrainObjectCount);
        }
    }

    // ─────────────────────────────────────────────────────
    //  TRANSPARENT BUFFER UPLOADS (every frame)
    // ─────────────────────────────────────────────────────
    //  TRANSPARENT GPU UPLOAD (per-frame, like dynamic)
    // ─────────────────────────────────────────────────────
    m_transparentSet.objectCount = std::min(static_cast<u32>(m_transparentObjectData.size()), m_transparentSet.maxObjects);

    if (m_transparentSet.objectCount > 0 && m_transparentSet.objectBuffer && m_transparentSet.drawArgsBuffer) {
        for (auto& args : m_transparentDrawArgsData)
            args.instanceCount = 1;

        cmdList->writeBuffer(m_transparentSet.objectBuffer,
            m_transparentObjectData.data(),
            m_transparentSet.objectCount * sizeof(GPUObjectData));
        cmdList->setBufferState(m_transparentSet.objectBuffer, nvrhi::ResourceStates::ShaderResource);

        cmdList->writeBuffer(m_transparentSet.drawArgsBuffer,
            m_transparentDrawArgsData.data(),
            m_transparentSet.objectCount * sizeof(IndirectDrawArgs));
        cmdList->setBufferState(m_transparentSet.drawArgsBuffer, nvrhi::ResourceStates::ShaderResource);

        cmdList->writeBuffer(m_transparentSet.materialIDBuffer,
            m_transparentMaterialIDData.data(),
            m_transparentSet.objectCount * sizeof(u32));
        cmdList->setBufferState(m_transparentSet.materialIDBuffer, nvrhi::ResourceStates::ShaderResource);

        cmdList->writeBuffer(m_transparentSet.instanceBuffer,
            m_transparentInstanceData.data(),
            m_transparentSet.objectCount * sizeof(GPUInstanceData));
        cmdList->setBufferState(m_transparentSet.instanceBuffer, nvrhi::ResourceStates::ShaderResource);
    }
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
    m_staticBatchVertexCounts.clear();

    Msg("* [GPUCulling] Static culling data invalidated");
}

void GPUCullingManager::InvalidateShadersAndPipelines()
{
    m_cullPipeline = nullptr;
    m_clearArgsPipeline = nullptr;
    m_compactCountPipeline = nullptr;
    m_compactScanPipeline = nullptr;
    m_compactScatterPipeline = nullptr;
    m_cullLayout = nullptr;
    m_clearArgsLayout = nullptr;
    m_compactCountLayout = nullptr;
    m_compactScanLayout = nullptr;
    m_compactScatterLayout = nullptr;

    m_variantPartitionPipeline = nullptr;
    m_variantPartitionLayout = nullptr;

    m_terrainApplyVisibilityPipeline = nullptr;
    m_terrainApplyVisibilityLayout = nullptr;

    m_debugComputePipeline = nullptr;
    m_particleDebugComputePipeline = nullptr;
    m_debugComputeLayout = nullptr;
    m_debugGraphicsPipeline = nullptr;
    m_debugGraphicsLayout = nullptr;
    m_debugInputLayout = nullptr;

    m_particleCullPipeline = nullptr;
    m_particleCullLayout = nullptr;

    m_pointSampler = nullptr;

    m_initialized = false;
    m_computeEnabled = false;
    m_compactEnabled = false;
    m_variantPartitionEnabled = false;
    m_particleCullEnabled = false;
    m_skinnedCullEnabled = false;

    m_staticDataCached = false;
    m_staticTerrainDrawArgsUploaded = false;
    m_staticSet.objectsUploaded = false;
    m_staticSet.drawArgsUploaded = false;
    m_dynamicSet.objectsUploaded = false;
    m_dynamicSet.drawArgsUploaded = false;

    Msg("* [GPUCulling] Shaders and pipelines invalidated for hot-reload");
}

// ═══════════════════════════════════════════════════════
//  UPLOAD SKINNED OBJECTS
// ═══════════════════════════════════════════════════════

void GPUCullingManager::UploadSkinnedObjects(fg::RenderContext* ctx, const GeometryCollector* geometry)
{
    ZoneScopedN("GPUCull::UploadSkinnedObjects");

    if (!m_skinnedCullEnabled || !geometry) {
        m_skinnedObjectCount = 0;
        return;
    }

    const auto& batches = geometry->GetBatches();

    // Clear and rebuild skinned data
    m_skinnedObjectData.clear();
    m_skinnedBatchPointers.clear();

    // Collect skinned batches
    for (const auto& batch : batches) {
        if (!batch.isSkinned)
            continue;

        u32 batchIdx = static_cast<u32>(m_skinnedObjectData.size());

        // Object data for culling (bounding sphere)
        GPUObjectData obj;
        obj.position = batch.worldBoundsCenter;
        obj.radius = batch.worldBoundsRadius;
        obj.batchIndex = batchIdx;
        obj.flags = 0;  // Skinned meshes use their own alpha handling
        obj.pad0 = 0.0f;
        obj.pad1 = 0.0f;
        m_skinnedObjectData.push_back(obj);
        m_skinnedBatchPointers.push_back(&batch);
    }

    m_skinnedObjectCount = static_cast<u32>(m_skinnedObjectData.size());

    if (m_skinnedObjectCount == 0)
        return;

    // Ensure buffer capacity
    EnsureSkinnedBufferCapacity(m_skinnedObjectCount);

    // Upload to GPU
    auto cmdList = ctx->GetCommandList();

    cmdList->writeBuffer(m_skinnedObjectBuffer,
        m_skinnedObjectData.data(),
        m_skinnedObjectCount * sizeof(GPUObjectData));
    cmdList->setBufferState(m_skinnedObjectBuffer, nvrhi::ResourceStates::ShaderResource);
}

void GPUCullingManager::ScheduleSkinnedVisibilityReadback(nvrhi::ICommandList* cmdList)
{
    if (!m_skinnedCullEnabled || m_skinnedObjectCount == 0)
        return;

    u32 writeSlot = m_skinnedReadbackWriteIndex;

    // Copy visibility buffer to current write slot
    cmdList->copyBuffer(
        m_skinnedReadbackBuffers[writeSlot], 0,
        m_skinnedVisibilityBuffer, 0,
        m_skinnedObjectCount * sizeof(u32));

    ++m_skinnedSubmitFrameId;
    m_skinnedReadbackSubmitFrame[writeSlot] = m_skinnedSubmitFrameId;
    m_skinnedReadbackCounts[writeSlot] = m_skinnedObjectCount;
    for (u32 i = 0; i < m_skinnedBatchPointers.size(); ++i) {
        const GeometryBatch* batch = m_skinnedBatchPointers[i];
        if (batch && batch->visual) {
            batch->visual->skinned_cull_index = i;
            batch->visual->skinned_cull_frame = m_skinnedSubmitFrameId;
        }
    }

    m_skinnedReadbackWriteIndex = (m_skinnedReadbackWriteIndex + 1) % SKINNED_READBACK_FRAMES;

    // Track frame count (caps at SKINNED_READBACK_FRAMES)
    if (m_skinnedReadbackFrameCount < SKINNED_READBACK_FRAMES) {
        m_skinnedReadbackFrameCount++;
    }
}

void GPUCullingManager::ProcessSkinnedVisibilityReadback()
{
    // Need 2 frames of data before reading (ensures GPU finished writing)
    // Frame 0: write to buffer[0]
    // Frame 1: write to buffer[1], can now read buffer[0] (frame 0's data)
    if (m_skinnedReadbackFrameCount < SKINNED_READBACK_FRAMES || m_skinnedObjectCount == 0) {
        return;
    }

    // Read from the buffer that was written 2 frames ago
    // Current write index points to where we'll write THIS frame
    // So readSlot = writeIndex means we read from where we're ABOUT to write
    // That buffer was written 2 frames ago (Frame N-2 if we're at Frame N)
    u32 readSlot = m_skinnedReadbackWriteIndex;

    void* mappedData = m_device->GetNVRHIDevice()->mapBuffer(
        m_skinnedReadbackBuffers[readSlot],
        nvrhi::CpuAccessMode::Read);

    if (mappedData) {
        const u32* visibilityData = static_cast<const u32*>(mappedData);

        const u32 count = std::min(m_skinnedReadbackCounts[readSlot], m_maxSkinnedObjects);
        m_skinnedVisibilityValues.assign(visibilityData, visibilityData + count);
        m_skinnedVisibilityFrame = m_skinnedReadbackSubmitFrame[readSlot];

        m_device->GetNVRHIDevice()->unmapBuffer(m_skinnedReadbackBuffers[readSlot]);
    }
}

void GPUCullingManager::UpdateSkinnedCullingStats(u32 rendered, u32 culled)
{
    m_skinnedCullingStats.submitted = rendered + culled;
    m_skinnedCullingStats.visible = rendered;
    m_skinnedCullingStats.culled = culled;
}

u32 GPUCullingManager::GetSkinnedVisibilityByVisual(const dxRender_Visual* visual) const
{
    if (visual && m_skinnedVisibilityFrame != 0
        && visual->skinned_cull_frame >= m_skinnedVisibilityFrame
        && visual->skinned_cull_index < m_skinnedVisibilityValues.size()) {
        return m_skinnedVisibilityValues[visual->skinned_cull_index];
    }
    return 0;  // Not found = culled (conservative)
}

void GPUCullingManager::ClearSkinnedVisibilityData()
{
    for (u32 i = 0; i < SKINNED_READBACK_FRAMES; ++i) {
        m_skinnedReadbackSubmitFrame[i] = 0;
        m_skinnedReadbackCounts[i] = 0;
    }
    m_skinnedVisibilityValues.clear();
    m_skinnedVisibilityFrame = 0;
    m_skinnedBatchPointers.clear();
    m_skinnedReadbackFrameCount = 0;  // Reset to avoid reading stale data
}

// ═══════════════════════════════════════════════════════
//  SKELETON BONE BUFFER
// ═══════════════════════════════════════════════════════

void GPUCullingManager::BeginSkinnedFrame()
{
    // Reset bone buffer allocations for new frame
    ++m_boneUploadFrameId;
    m_currentBoneOffset = 0;
}

u32 GPUCullingManager::GetOrUploadSkeleton(nvrhi::ICommandList* cmdList, CKinematics* skeleton)
{
    if (!m_boneBufferInitialized || !skeleton || !cmdList)
        return 0;

    // Check if already uploaded this frame
    if (skeleton->fg_bone_upload_frame == m_boneUploadFrameId) {
        return skeleton->fg_bone_upload_offset;
    }

    // Allocate space for this skeleton
    u32 boneCount = skeleton->LL_BoneCount();
    if (boneCount == 0)
        return 0;

    // Check capacity
    if (m_currentBoneOffset + boneCount > MAX_TOTAL_BONES) {
        Msg("! [GPUCulling] Bone buffer full: need %u bones, have %u remaining",
            boneCount, MAX_TOTAL_BONES - m_currentBoneOffset);
        return 0;  // Return 0 offset, will render with identity bones
    }

    // Record the offset for this skeleton
    u32 boneOffset = m_currentBoneOffset;
    skeleton->fg_bone_upload_frame = m_boneUploadFrameId;
    skeleton->fg_bone_upload_offset = boneOffset;

    // Upload bones
    UploadSkeletonBones(cmdList, skeleton, boneOffset);

    // Advance allocation pointer
    m_currentBoneOffset += boneCount;

    return boneOffset;
}

void GPUCullingManager::UploadSkeletonBones(nvrhi::ICommandList* cmdList, CKinematics* skeleton, u32 boneOffset)
{
    u32 boneCount = skeleton->LL_BoneCount();

    // Ensure staging buffer is large enough
    if (m_boneStagingBuffer.size() < boneCount) {
        m_boneStagingBuffer.resize(boneCount);
    }

    // Slang uses column_major — raw row-major Fmatrix bytes are naturally transposed.
    // No explicit transpose needed.
    for (u32 i = 0; i < boneCount; i++) {
        m_boneStagingBuffer[i] = skeleton->LL_GetTransform_R(u16(i));
    }

    // Upload to GPU at the correct offset
    u64 byteOffset = static_cast<u64>(boneOffset) * BONE_STRIDE;
    u64 byteSize = static_cast<u64>(boneCount) * BONE_STRIDE;

    cmdList->writeBuffer(m_globalBoneBuffer, m_boneStagingBuffer.data(), byteSize, byteOffset);
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
    output.transparentCompactDrawArgs = VirtualResourceHandle();
    output.transparentCompactBatchIndices = VirtualResourceHandle();
    output.transparentCompactMaterialIDs = VirtualResourceHandle();
    output.transparentCompactCount = VirtualResourceHandle();
    output.transparentObjectCount = 0;

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
            passBuilder.asyncCompute();

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
           fg::RenderContext* ctx) {

            GPUCullingManager* mgr = data.manager;
            if (!mgr->m_computeEnabled)
                return;

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();

            nvrhi::IDevice* nvDevice = mgr->m_device->GetNVRHIDevice();
            u32 frameId = Device.dwFrame + 1u;
            if (frameId == 0)
                frameId = 1;

            // ─────────────────────────────────────────────────────
            //  UPLOAD SCENE OBJECTS (must happen during execute, not setup)
            // ─────────────────────────────────────────────────────
            // This ensures we use the correct command list
            mgr->UploadSceneObjects(ctx, data.geometry);

            bindless::MaterialBuffer::Instance().Upload(ctx);

            if (mgr->m_rtAccelMgr) {
                mgr->m_rtAccelMgr->BuildIfNeeded(cmdList, mgr);
                if (mgr->m_rtAccelMgr->IsReady()) {
                    if (!mgr->m_rtAccelMgr->GetMaterialBuffer())
                        mgr->m_rtAccelMgr->SetMaterialBuffer(bindless::MaterialBuffer::Instance().GetBuffer());
                    if (!mgr->m_rtAccelMgr->GetTerrainMaterialBuffer())
                        mgr->m_rtAccelMgr->SetTerrainMaterialBuffer(bindless::TerrainMaterialBuffer::Instance().GetBuffer());
                }
            }

            // Get Hi-Z texture
            nvrhi::ITexture* hizTexture = fg.GetPhysicalTexture(data.hizPyramid);
            if (!hizTexture) {
                Msg("! [GPUCulling] Hi-Z texture not available");
                return;
            }

            auto dispatchCullSet = [&](CullSetBuffers& set, VariantPartitionBuffers* partition = nullptr) {
                if (set.objectCount == 0)
                    return;

                R_ASSERT2(set.objectBuffer && set.visibleIndexBuffer && set.visibleCountBuffer && set.visibilityBuffer,
                    "Cull set buffers not initialized");

                // Clear visible count to 0
                u32 zero = 0;
                cmdList->writeBuffer(set.visibleCountBuffer, &zero, sizeof(u32));

                // Fill constant buffer
                CullParamsCB cb;
                cb.viewProj = Device.mFullTransform;
                cb.prevViewProj = data.prevViewProj;  // Previous frame's viewProj for temporal Hi-Z
                cb.cameraPos = Device.vCameraPosition;
                float farPlane = g_pGamePersistent ? g_pGamePersistent->Environment().CurrentEnv.far_plane : 300.0f;
                cb.maxDistanceSq = farPlane * farPlane;
                cb.objectCount = set.objectCount;
                cb.hizWidth = data.hizWidth;
                cb.hizHeight = data.hizHeight;
                cb.hizMipLevels = data.hizMipLevels;
                cb.frameId = frameId;
                cb.padding[0] = cb.padding[1] = cb.padding[2] = 0;

                mgr->ExtractFrustumPlanes(Device.mFullTransform, cb.frustumPlanes);

                cmdList->writeBuffer(mgr->m_device->GetNativeBuffer(mgr->m_cullParamsCB), &cb, sizeof(cb));

                auto* objectCullRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("object_cull", ".cs");
                framegraph::BindingSetBuilder bsb(*objectCullRefl, nvDevice, "GPUCull.ObjectCull");
                bsb.ConstantBuffer("CullParams", mgr->m_device->GetNativeBuffer(mgr->m_cullParamsCB))
                   .BufferSRV("g_Objects", set.objectBuffer)
                   .Texture("g_HiZPyramid", hizTexture)
                   .BufferUAV("g_VisibleIndices", set.visibleIndexBuffer)
                   .BufferUAV("g_VisibleCount", set.visibleCountBuffer)
                   .BufferUAV("g_Visibility", set.visibilityBuffer);

                nvrhi::BindingSetHandle bindingSet = nvDevice->createBindingSet(bsb.Build(), mgr->m_cullLayout);
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
                                  set.compactMaterialIDBuffer && set.compactCountBuffer &&
                                  set.compactLocalPrefixBuffer && set.compactGroupCountsBuffer &&
                                  set.compactGroupOffsetsBuffer,
                        "Compaction buffers not initialized");
                    R_ASSERT2(mgr->m_compactCountPipeline && mgr->m_compactScanPipeline && mgr->m_compactScatterPipeline,
                        "Compaction pipelines not initialized");

                    u32 compactGroupCount = (set.objectCount + COMPACT_THREAD_GROUP_SIZE - 1) / COMPACT_THREAD_GROUP_SIZE;
                    if (compactGroupCount > 0) {
                        R_ASSERT2(compactGroupCount <= COMPACT_THREAD_GROUP_SIZE,
                            "Compaction group count exceeds scan group size");
                    }

                    struct CompactParamsCB {
                        u32 batchCount;
                        u32 frameId;
                        u32 padding[2];
                    };
                    CompactParamsCB compactCB;
                    compactCB.batchCount = set.objectCount;
                    compactCB.frameId = frameId;
                    compactCB.padding[0] = compactCB.padding[1] = 0;
                    cmdList->writeBuffer(mgr->m_device->GetNativeBuffer(mgr->m_compactParamsCB), &compactCB, sizeof(compactCB));

                    if (compactGroupCount == 0) {
                        u32 zeroCount = 0;
                        cmdList->writeBuffer(set.compactCountBuffer, &zeroCount, sizeof(u32));
                        u32 zeroDispatch[3] = { 0, 1, 1 };
                        cmdList->writeBuffer(set.compactDispatchArgsBuffer, zeroDispatch, sizeof(zeroDispatch));
                    } else {
                        cmdList->setBufferState(set.visibilityBuffer, nvrhi::ResourceStates::ShaderResource);
                        cmdList->setBufferState(set.compactLocalPrefixBuffer, nvrhi::ResourceStates::UnorderedAccess);
                        cmdList->setBufferState(set.compactGroupCountsBuffer, nvrhi::ResourceStates::UnorderedAccess);

                        auto* compactCountRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("batch_compact_count", ".cs");
                        framegraph::BindingSetBuilder countBsb(*compactCountRefl, nvDevice);
                        countBsb.ConstantBuffer("CompactParams", mgr->m_device->GetNativeBuffer(mgr->m_compactParamsCB))
                               .BufferSRV("g_Visibility", set.visibilityBuffer)
                               .BufferUAV("g_LocalPrefix", set.compactLocalPrefixBuffer)
                               .BufferUAV("g_GroupCounts", set.compactGroupCountsBuffer);

                        nvrhi::BindingSetHandle countBindingSet = framegraph::GetPassResourceCache().GetOrCreateBindingSet(countBsb.Build(), mgr->m_compactCountLayout, nvDevice);
                        R_ASSERT2(countBindingSet, "Failed to create compaction count binding set");

                        nvrhi::ComputeState countState;
                        countState.pipeline = mgr->m_compactCountPipeline;
                        countState.bindings = { countBindingSet };
                        cmdList->setComputeState(countState);
                        cmdList->dispatch(compactGroupCount, 1, 1);

                        cmdList->setBufferState(set.compactGroupCountsBuffer, nvrhi::ResourceStates::ShaderResource);
                        cmdList->setBufferState(set.compactGroupOffsetsBuffer, nvrhi::ResourceStates::UnorderedAccess);
                        cmdList->setBufferState(set.compactCountBuffer, nvrhi::ResourceStates::UnorderedAccess);
                        cmdList->setBufferState(set.compactDispatchArgsBuffer, nvrhi::ResourceStates::UnorderedAccess);

                        auto* compactScanRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("batch_compact_scan", ".cs");
                        framegraph::BindingSetBuilder scanBsb(*compactScanRefl, nvDevice);
                        scanBsb.ConstantBuffer("CompactParams", mgr->m_device->GetNativeBuffer(mgr->m_compactParamsCB))
                               .BufferSRV("g_GroupCounts", set.compactGroupCountsBuffer)
                               .BufferUAV("g_GroupOffsets", set.compactGroupOffsetsBuffer)
                               .BufferUAV("g_VisibleCount", set.compactCountBuffer)
                               .BufferUAV("g_DispatchArgs", set.compactDispatchArgsBuffer);

                        nvrhi::BindingSetHandle scanBindingSet = framegraph::GetPassResourceCache().GetOrCreateBindingSet(scanBsb.Build(), mgr->m_compactScanLayout, nvDevice);
                        R_ASSERT2(scanBindingSet, "Failed to create compaction scan binding set");

                        nvrhi::ComputeState scanState;
                        scanState.pipeline = mgr->m_compactScanPipeline;
                        scanState.bindings = { scanBindingSet };
                        cmdList->setComputeState(scanState);
                        cmdList->dispatch(1, 1, 1);

                        cmdList->setBufferState(set.compactLocalPrefixBuffer, nvrhi::ResourceStates::ShaderResource);
                        cmdList->setBufferState(set.compactGroupOffsetsBuffer, nvrhi::ResourceStates::ShaderResource);
                        cmdList->setBufferState(set.drawArgsBuffer, nvrhi::ResourceStates::ShaderResource);
                        cmdList->setBufferState(set.materialIDBuffer, nvrhi::ResourceStates::ShaderResource);
                        cmdList->setBufferState(set.compactDrawArgsBuffer, nvrhi::ResourceStates::UnorderedAccess);
                        cmdList->setBufferState(set.compactBatchIndicesBuffer, nvrhi::ResourceStates::UnorderedAccess);
                        cmdList->setBufferState(set.compactMaterialIDBuffer, nvrhi::ResourceStates::UnorderedAccess);

                        auto* compactScatterRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("batch_compact", ".cs");
                        framegraph::BindingSetBuilder scatterBsb(*compactScatterRefl, nvDevice);
                        scatterBsb.ConstantBuffer("CompactParams", mgr->m_device->GetNativeBuffer(mgr->m_compactParamsCB))
                                  .BufferSRV("g_InputDrawArgs", set.drawArgsBuffer)
                                  .BufferSRV("g_InputMaterialIDs", set.materialIDBuffer)
                                  .BufferSRV("g_Visibility", set.visibilityBuffer)
                                  .BufferSRV("g_LocalPrefix", set.compactLocalPrefixBuffer)
                                  .BufferSRV("g_GroupOffsets", set.compactGroupOffsetsBuffer)
                                  .BufferUAV("g_OutputDrawArgs", set.compactDrawArgsBuffer)
                                  .BufferUAV("g_VisibleBatchIndices", set.compactBatchIndicesBuffer)
                                  .BufferUAV("g_OutputMaterialIDs", set.compactMaterialIDBuffer);

                        nvrhi::BindingSetHandle scatterBindingSet = framegraph::GetPassResourceCache().GetOrCreateBindingSet(scatterBsb.Build(), mgr->m_compactScatterLayout, nvDevice);
                        R_ASSERT2(scatterBindingSet, "Failed to create compaction scatter binding set");

                        nvrhi::ComputeState scatterState;
                        scatterState.pipeline = mgr->m_compactScatterPipeline;
                        scatterState.bindings = { scatterBindingSet };
                        cmdList->setComputeState(scatterState);
                        cmdList->dispatch(compactGroupCount, 1, 1);
                    }

                    if (partition && mgr->m_variantPartitionEnabled) {
                        mgr->DispatchVariantPartition(cmdList, nvDevice, set, *partition);
                    }

                    cmdList->setBufferState(set.compactDrawArgsBuffer, nvrhi::ResourceStates::IndirectArgument);
                    cmdList->setBufferState(set.compactCountBuffer, nvrhi::ResourceStates::IndirectArgument);
                }
            };

            dispatchCullSet(mgr->m_staticSet,
                mgr->m_variantPartitionEnabled ? &mgr->m_staticPartition : nullptr);
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
                // Update constant buffer for terrain (reuse same CB, different object count)
                CullParamsCB terrainCB;
                terrainCB.viewProj = Device.mFullTransform;
                terrainCB.prevViewProj = data.prevViewProj;  // Previous frame for temporal Hi-Z
                terrainCB.cameraPos = Device.vCameraPosition;
                float farPlane = g_pGamePersistent ? g_pGamePersistent->Environment().CurrentEnv.far_plane : 300.0f;
                terrainCB.maxDistanceSq = farPlane * farPlane;
                terrainCB.objectCount = mgr->m_terrainObjectCount;
                terrainCB.hizWidth = data.hizWidth;
                terrainCB.hizHeight = data.hizHeight;
                terrainCB.hizMipLevels = data.hizMipLevels;
                terrainCB.frameId = frameId;
                terrainCB.padding[0] = terrainCB.padding[1] = terrainCB.padding[2] = 0;
                mgr->ExtractFrustumPlanes(Device.mFullTransform, terrainCB.frustumPlanes);
                cmdList->writeBuffer(mgr->m_device->GetNativeBuffer(mgr->m_cullParamsCB), &terrainCB, sizeof(terrainCB));

                auto* objectCullRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("object_cull", ".cs");
                framegraph::BindingSetBuilder terrainBsb(*objectCullRefl, nvDevice, "GPUCull.TerrainCull");
                terrainBsb.ConstantBuffer("CullParams", mgr->m_device->GetNativeBuffer(mgr->m_cullParamsCB))
                          .BufferSRV("g_Objects", mgr->m_terrainObjectBuffer)
                          .Texture("g_HiZPyramid", hizTexture)
                          .BufferUAV("g_VisibleIndices", mgr->m_terrainVisibleIndexBuffer)
                          .BufferUAV("g_VisibleCount", mgr->m_terrainVisibleCountBuffer)
                          .BufferUAV("g_Visibility", mgr->m_terrainVisibilityBuffer);

                nvrhi::BindingSetHandle terrainBindingSet = nvDevice->createBindingSet(terrainBsb.Build(), mgr->m_cullLayout);
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
                R_ASSERT2(mgr->m_compactCountPipeline && mgr->m_compactScanPipeline && mgr->m_compactScatterPipeline,
                    "Terrain compaction pipelines not initialized");
                R_ASSERT2(mgr->m_terrainCompactDrawArgsBuffer && mgr->m_terrainCompactBatchIndicesBuffer &&
                              mgr->m_terrainCompactMaterialIDBuffer && mgr->m_terrainCompactCountBuffer &&
                              mgr->m_terrainCompactLocalPrefixBuffer && mgr->m_terrainCompactGroupCountsBuffer &&
                              mgr->m_terrainCompactGroupOffsetsBuffer,
                    "Terrain compaction buffers not initialized");
                R_ASSERT2(mgr->m_terrainMaterialIDBuffer, "Terrain material ID buffer missing");

                // Update compact params (reuse same CB)
                struct CompactParamsCB {
                    u32 batchCount;
                    u32 frameId;
                    u32 padding[2];
                };
                CompactParamsCB terrainCompactCB;
                terrainCompactCB.batchCount = mgr->m_terrainObjectCount;
                terrainCompactCB.frameId = frameId;
                terrainCompactCB.padding[0] = terrainCompactCB.padding[1] = 0;
                cmdList->writeBuffer(mgr->m_device->GetNativeBuffer(mgr->m_compactParamsCB), &terrainCompactCB, sizeof(terrainCompactCB));

                u32 terrainCompactGroupCount =
                    (mgr->m_terrainObjectCount + COMPACT_THREAD_GROUP_SIZE - 1) / COMPACT_THREAD_GROUP_SIZE;
                if (terrainCompactGroupCount > 0) {
                    R_ASSERT2(terrainCompactGroupCount <= COMPACT_THREAD_GROUP_SIZE,
                        "Terrain compaction group count exceeds scan group size");
                }

                if (terrainCompactGroupCount == 0) {
                    u32 zeroTerrainCount = 0;
                    cmdList->writeBuffer(mgr->m_terrainCompactCountBuffer, &zeroTerrainCount, sizeof(u32));
                } else {
                    cmdList->setBufferState(mgr->m_terrainVisibilityBuffer, nvrhi::ResourceStates::ShaderResource);
                    cmdList->setBufferState(mgr->m_terrainCompactLocalPrefixBuffer, nvrhi::ResourceStates::UnorderedAccess);
                    cmdList->setBufferState(mgr->m_terrainCompactGroupCountsBuffer, nvrhi::ResourceStates::UnorderedAccess);

                    auto* compactCountRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("batch_compact_count", ".cs");
                    framegraph::BindingSetBuilder terrainCountBsb(*compactCountRefl, nvDevice);
                    terrainCountBsb.ConstantBuffer("CompactParams", mgr->m_device->GetNativeBuffer(mgr->m_compactParamsCB))
                                   .BufferSRV("g_Visibility", mgr->m_terrainVisibilityBuffer)
                                   .BufferUAV("g_LocalPrefix", mgr->m_terrainCompactLocalPrefixBuffer)
                                   .BufferUAV("g_GroupCounts", mgr->m_terrainCompactGroupCountsBuffer);

                    nvrhi::BindingSetHandle terrainCountBindingSet =
                        framegraph::GetPassResourceCache().GetOrCreateBindingSet(terrainCountBsb.Build(), mgr->m_compactCountLayout, nvDevice);
                    R_ASSERT2(terrainCountBindingSet, "Terrain compaction count binding set creation failed");

                    nvrhi::ComputeState terrainCountState;
                    terrainCountState.pipeline = mgr->m_compactCountPipeline;
                    terrainCountState.bindings = { terrainCountBindingSet };
                    cmdList->setComputeState(terrainCountState);
                    cmdList->dispatch(terrainCompactGroupCount, 1, 1);

                    cmdList->setBufferState(mgr->m_terrainCompactGroupCountsBuffer, nvrhi::ResourceStates::ShaderResource);
                    cmdList->setBufferState(mgr->m_terrainCompactGroupOffsetsBuffer, nvrhi::ResourceStates::UnorderedAccess);
                    cmdList->setBufferState(mgr->m_terrainCompactCountBuffer, nvrhi::ResourceStates::UnorderedAccess);
                    cmdList->setBufferState(mgr->m_terrainCompactDispatchArgsBuffer, nvrhi::ResourceStates::UnorderedAccess);

                    auto* compactScanRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("batch_compact_scan", ".cs");
                    framegraph::BindingSetBuilder terrainScanBsb(*compactScanRefl, nvDevice);
                    terrainScanBsb.ConstantBuffer("CompactParams", mgr->m_device->GetNativeBuffer(mgr->m_compactParamsCB))
                                  .BufferSRV("g_GroupCounts", mgr->m_terrainCompactGroupCountsBuffer)
                                  .BufferUAV("g_GroupOffsets", mgr->m_terrainCompactGroupOffsetsBuffer)
                                  .BufferUAV("g_VisibleCount", mgr->m_terrainCompactCountBuffer)
                                  .BufferUAV("g_DispatchArgs", mgr->m_terrainCompactDispatchArgsBuffer);

                    nvrhi::BindingSetHandle terrainScanBindingSet =
                        framegraph::GetPassResourceCache().GetOrCreateBindingSet(terrainScanBsb.Build(), mgr->m_compactScanLayout, nvDevice);
                    R_ASSERT2(terrainScanBindingSet, "Terrain compaction scan binding set creation failed");

                    nvrhi::ComputeState terrainScanState;
                    terrainScanState.pipeline = mgr->m_compactScanPipeline;
                    terrainScanState.bindings = { terrainScanBindingSet };
                    cmdList->setComputeState(terrainScanState);
                    cmdList->dispatch(1, 1, 1);

                    cmdList->setBufferState(mgr->m_terrainCompactLocalPrefixBuffer, nvrhi::ResourceStates::ShaderResource);
                    cmdList->setBufferState(mgr->m_terrainCompactGroupOffsetsBuffer, nvrhi::ResourceStates::ShaderResource);
                    cmdList->setBufferState(mgr->m_terrainDrawArgsBuffer, nvrhi::ResourceStates::ShaderResource);
                    cmdList->setBufferState(mgr->m_terrainMaterialIDBuffer, nvrhi::ResourceStates::ShaderResource);
                    cmdList->setBufferState(mgr->m_terrainCompactDrawArgsBuffer, nvrhi::ResourceStates::UnorderedAccess);
                    cmdList->setBufferState(mgr->m_terrainCompactBatchIndicesBuffer, nvrhi::ResourceStates::UnorderedAccess);
                    cmdList->setBufferState(mgr->m_terrainCompactMaterialIDBuffer, nvrhi::ResourceStates::UnorderedAccess);

                    auto* compactScatterRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("batch_compact", ".cs");
                    framegraph::BindingSetBuilder terrainScatterBsb(*compactScatterRefl, nvDevice);
                    terrainScatterBsb.ConstantBuffer("CompactParams", mgr->m_device->GetNativeBuffer(mgr->m_compactParamsCB))
                                     .BufferSRV("g_InputDrawArgs", mgr->m_terrainDrawArgsBuffer)
                                     .BufferSRV("g_InputMaterialIDs", mgr->m_terrainMaterialIDBuffer)
                                     .BufferSRV("g_Visibility", mgr->m_terrainVisibilityBuffer)
                                     .BufferSRV("g_LocalPrefix", mgr->m_terrainCompactLocalPrefixBuffer)
                                     .BufferSRV("g_GroupOffsets", mgr->m_terrainCompactGroupOffsetsBuffer)
                                     .BufferUAV("g_OutputDrawArgs", mgr->m_terrainCompactDrawArgsBuffer)
                                     .BufferUAV("g_VisibleBatchIndices", mgr->m_terrainCompactBatchIndicesBuffer)
                                     .BufferUAV("g_OutputMaterialIDs", mgr->m_terrainCompactMaterialIDBuffer);

                    nvrhi::BindingSetHandle terrainScatterBindingSet =
                        framegraph::GetPassResourceCache().GetOrCreateBindingSet(terrainScatterBsb.Build(), mgr->m_compactScatterLayout, nvDevice);
                    R_ASSERT2(terrainScatterBindingSet, "Terrain compaction scatter binding set creation failed");

                    nvrhi::ComputeState terrainScatterState;
                    terrainScatterState.pipeline = mgr->m_compactScatterPipeline;
                    terrainScatterState.bindings = { terrainScatterBindingSet };
                    cmdList->setComputeState(terrainScatterState);
                    cmdList->dispatch(terrainCompactGroupCount, 1, 1);
                }

                // Transition compact buffers to IndirectArgument for DrawIndexedIndirectCount
                cmdList->setBufferState(mgr->m_terrainCompactDrawArgsBuffer, nvrhi::ResourceStates::IndirectArgument);
                cmdList->setBufferState(mgr->m_terrainCompactCountBuffer, nvrhi::ResourceStates::IndirectArgument);
            }

            // ─────────────────────────────────────────────────────
            //  TRANSPARENT CULLING (uses CullSetBuffers + dispatchCullSet)
            // ─────────────────────────────────────────────────────
            dispatchCullSet(mgr->m_transparentSet,
                mgr->m_variantPartitionEnabled ? &mgr->m_transparentPartition : nullptr);

            // Note: partition buffers are left in IndirectArgument/ShaderResource state
            // for the render passes to consume. keepInitialState handles reset for next frame.
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

    // ───────────────────────────────────────────────────────
    //  TRANSPARENT OUTPUT HANDLES
    // ───────────────────────────────────────────────────────
    output.transparentObjectCount = m_transparentSet.objectCount;

    if (m_transparentSet.objectCount > 0 && m_compactEnabled) {
        ResourceDesc transCompactArgsDesc;
        transCompactArgsDesc.type = ResourceDesc::Type::Buffer;
        transCompactArgsDesc.debugName = "GPUCull_TransCompactDrawArgs";
        transCompactArgsDesc.bufferSize = m_transparentSet.maxObjects * sizeof(IndirectDrawArgs);
        transCompactArgsDesc.isUAV = true;
        transCompactArgsDesc.isTransient = false;

        ResourceDesc transCompactIndicesDesc;
        transCompactIndicesDesc.type = ResourceDesc::Type::Buffer;
        transCompactIndicesDesc.debugName = "GPUCull_TransCompactBatchIndices";
        transCompactIndicesDesc.bufferSize = m_transparentSet.maxObjects * sizeof(u32);
        transCompactIndicesDesc.structStride = sizeof(u32);
        transCompactIndicesDesc.isUAV = true;
        transCompactIndicesDesc.isTransient = false;

        ResourceDesc transCompactMaterialDesc;
        transCompactMaterialDesc.type = ResourceDesc::Type::Buffer;
        transCompactMaterialDesc.debugName = "GPUCull_TransCompactMaterialIDs";
        transCompactMaterialDesc.bufferSize = m_transparentSet.maxObjects * sizeof(u32);
        transCompactMaterialDesc.structStride = sizeof(u32);
        transCompactMaterialDesc.isUAV = true;
        transCompactMaterialDesc.isTransient = false;

        ResourceDesc transCompactCountDesc;
        transCompactCountDesc.type = ResourceDesc::Type::Buffer;
        transCompactCountDesc.debugName = "GPUCull_TransCompactCount";
        transCompactCountDesc.bufferSize = sizeof(u32);
        transCompactCountDesc.isUAV = true;
        transCompactCountDesc.isTransient = false;

        output.transparentCompactDrawArgs = fg.ImportBuffer("gpu_cull_trans_compact_drawargs", m_transparentSet.compactDrawArgsBuffer, transCompactArgsDesc);
        output.transparentCompactBatchIndices = fg.ImportBuffer("gpu_cull_trans_compact_batchindices", m_transparentSet.compactBatchIndicesBuffer, transCompactIndicesDesc);
        output.transparentCompactMaterialIDs = fg.ImportBuffer("gpu_cull_trans_compact_materialids", m_transparentSet.compactMaterialIDBuffer, transCompactMaterialDesc);
        output.transparentCompactCount = fg.ImportBuffer("gpu_cull_trans_compact_count", m_transparentSet.compactCountBuffer, transCompactCountDesc);
    } else {
        output.transparentCompactDrawArgs = VirtualResourceHandle();
        output.transparentCompactBatchIndices = VirtualResourceHandle();
        output.transparentCompactMaterialIDs = VirtualResourceHandle();
        output.transparentCompactCount = VirtualResourceHandle();
    }

    return output;
}

// ═══════════════════════════════════════════════════════
//  SETUP SKINNED CULLING PASS
// ═══════════════════════════════════════════════════════

void GPUCullingManager::SetupSkinnedCullingPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle hizPyramid,
    u32 hizWidth,
    u32 hizHeight,
    u32 hizMipLevels,
    const GeometryCollector* geometry,
    const Fmatrix& prevViewProj)
{
    using namespace framegraph;

    // Early out if not enabled
    if (!m_skinnedCullEnabled) {
        return;
    }

    struct SkinnedCullPassData {
        VirtualResourceHandle hizPyramid;
        VirtualResourceHandle visibilityBuffer;  // For framegraph dependency tracking
        GPUCullingManager* manager;
        const GeometryCollector* geometry;
        Fmatrix prevViewProj;
        u32 hizWidth;
        u32 hizHeight;
        u32 hizMipLevels;
    };

    // Import visibility buffer into framegraph for proper dependency tracking
    ResourceDesc visBufferDesc;
    visBufferDesc.type = ResourceDesc::Type::Buffer;
    visBufferDesc.debugName = "GPUCull_SkinnedVisibility";
    visBufferDesc.bufferSize = m_maxSkinnedObjects * sizeof(u32);
    visBufferDesc.structStride = sizeof(u32);
    visBufferDesc.isUAV = true;
    visBufferDesc.isTransient = false;

    VirtualResourceHandle visBufferHandle = fg.ImportBuffer(
        "skinned_visibility", m_skinnedVisibilityBuffer, visBufferDesc);

    auto& passData = fg.addCallbackPass<SkinnedCullPassData>(
        "Skinned GPU Culling",

        // Setup lambda
        [&, hizWidth, hizHeight, hizMipLevels, geometry, prevViewProj, visBufferHandle](FrameGraph& builder, PassHandle passHandle, SkinnedCullPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);
            passBuilder.asyncCompute();

            data.manager = this;
            data.geometry = geometry;
            data.prevViewProj = prevViewProj;
            data.hizWidth = hizWidth;
            data.hizHeight = hizHeight;
            data.hizMipLevels = hizMipLevels;

            // Read Hi-Z pyramid
            data.hizPyramid = passBuilder.read(hizPyramid, ResourceState::ShaderResource);

            // Write visibility buffer (ensures pass isn't culled by framegraph)
            data.visibilityBuffer = passBuilder.write(visBufferHandle, ResourceState::UnorderedAccess);
        },

        // Execute lambda
        [](const SkinnedCullPassData& data,
           const FrameGraph& fg,
           fg::RenderContext* ctx) {

            GPUCullingManager* mgr = data.manager;
            if (!mgr->m_skinnedCullEnabled)
                return;

            // Upload skinned objects (must happen during execute with correct command list)
            mgr->UploadSkinnedObjects(ctx, data.geometry);

            // Early out if no skinned objects after upload
            if (mgr->m_skinnedObjectCount == 0)
                return;

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            nvrhi::IDevice* nvDevice = mgr->m_device->GetNVRHIDevice();

            // Frame ID for visibility stamping
            u32 frameId = Device.dwFrame + 1u;
            if (frameId == 0)
                frameId = 1;
            mgr->m_skinnedFrameId = frameId;

            // Get Hi-Z texture
            nvrhi::ITexture* hizTexture = fg.GetPhysicalTexture(data.hizPyramid);
            if (!hizTexture) {
                Msg("! [GPUCulling] Hi-Z texture not available for skinned culling");
                return;
            }

            // Clear visibility buffer to 0 (all culled initially)
            cmdList->clearBufferUInt(mgr->m_skinnedVisibilityBuffer, 0);

            // Fill constant buffer (reuse m_cullParamsCB)
            CullParamsCB cb;
            cb.viewProj = Device.mFullTransform;
            cb.prevViewProj = data.prevViewProj;
            cb.cameraPos = Device.vCameraPosition;
            float farPlane = g_pGamePersistent ? g_pGamePersistent->Environment().CurrentEnv.far_plane : 300.0f;
            cb.maxDistanceSq = farPlane * farPlane;
            cb.objectCount = mgr->m_skinnedObjectCount;
            cb.hizWidth = data.hizWidth;
            cb.hizHeight = data.hizHeight;
            cb.hizMipLevels = data.hizMipLevels;
            cb.frameId = frameId;
            cb.padding[0] = cb.padding[1] = cb.padding[2] = 0;

            mgr->ExtractFrustumPlanes(Device.mFullTransform, cb.frustumPlanes);

            cmdList->writeBuffer(mgr->m_device->GetNativeBuffer(mgr->m_cullParamsCB), &cb, sizeof(cb));

            auto* objectCullRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("object_cull", ".cs");
                framegraph::BindingSetBuilder bsb(*objectCullRefl, nvDevice, "GPUCull.DynamicCull");
            bsb.ConstantBuffer("CullParams", mgr->m_device->GetNativeBuffer(mgr->m_cullParamsCB))
               .BufferSRV("g_Objects", mgr->m_skinnedObjectBuffer)
               .Texture("g_HiZPyramid", hizTexture)
               .BufferUAV("g_VisibleIndices", mgr->m_staticSet.visibleIndexBuffer)  // Dummy - not used
               .BufferUAV("g_VisibleCount", mgr->m_staticSet.visibleCountBuffer)    // Dummy - not used
               .BufferUAV("g_Visibility", mgr->m_skinnedVisibilityBuffer);

            nvrhi::BindingSetHandle bindingSet = nvDevice->createBindingSet(bsb.Build(), mgr->m_cullLayout);
            R_ASSERT2(bindingSet, "Failed to create skinned culling binding set");

            // Set compute state and dispatch culling
            nvrhi::ComputeState state;
            state.pipeline = mgr->m_cullPipeline;
            state.bindings = { bindingSet };
            cmdList->setComputeState(state);

            u32 groupCount = (mgr->m_skinnedObjectCount + CULL_THREAD_GROUP_SIZE - 1) / CULL_THREAD_GROUP_SIZE;
            cmdList->dispatch(groupCount, 1, 1);

            // Schedule visibility readback for CPU access
            mgr->ScheduleSkinnedVisibilityReadback(cmdList);
        }
    );
}

// ═══════════════════════════════════════════════════════
//  DEBUG VISUALIZATION
// ═══════════════════════════════════════════════════════

bool GPUCullingManager::IsDebugEnabled() const
{
    return ps_r4_debug_gpu_culling != 0 && m_computeEnabled && m_debugComputePipeline && m_debugGraphicsPipeline;
}

void GPUCullingManager::CreateDebugResources(fg::RenderDevice* device)
{
    if (!m_computeEnabled)
        return;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();

    auto debugCsResult = GEnv.Render->GetShaderLoader()->LoadComputeShader("object_cull_debug");
    auto particleDebugCsResult = GEnv.Render->GetShaderLoader()->LoadComputeShader("particle_cull_debug");
    auto debugVsResult = GEnv.Render->GetShaderLoader()->LoadVertexShader("cull_debug");
    auto debugPsResult = GEnv.Render->GetShaderLoader()->LoadPixelShader("cull_debug");

    if (!debugCsResult.handle) {
        Msg("! [GPUCulling] object_cull_debug.cs not found - debug visualization disabled");
        return;
    }
    if (!debugVsResult.handle) {
        Msg("! [GPUCulling] cull_debug.vs not found - debug visualization disabled");
        return;
    }
    if (!debugPsResult.handle) {
        Msg("! [GPUCulling] cull_debug.ps not found - debug visualization disabled");
        return;
    }
    if (!particleDebugCsResult.handle) {
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
        fg::RenderDevice::BufferDesc desc;
        desc.debugName = "GPUCull_DebugComputeParams";
        desc.byteSize = sizeof(CullDebugParamsCB);
        desc.isConstantBuffer = true;
        desc.isVolatile = true;
        desc.maxVersions = fg::RenderDevice::BufferDesc::VOLATILE_CB_MAX_VERSIONS;

        m_debugComputeParamsCB = m_device->CreateBuffer(desc);
        if (!m_debugComputeParamsCB.IsValid()) {
            Msg("! [GPUCulling] Failed to create debug compute constant buffer");
            return;
        }
    }
    {
        // Graphics shader constant buffer
        fg::RenderDevice::BufferDesc desc;
        desc.debugName = "GPUCull_DebugGraphicsParams";
        desc.byteSize = sizeof(CullDebugVSParamsCB);
        desc.isConstantBuffer = true;
        desc.isVolatile = true;
        desc.maxVersions = fg::RenderDevice::BufferDesc::VOLATILE_CB_MAX_VERSIONS;

        m_debugGraphicsParamsCB = m_device->CreateBuffer(desc);
        if (!m_debugGraphicsParamsCB.IsValid()) {
            Msg("! [GPUCulling] Failed to create debug graphics constant buffer");
            return;
        }
    }

    // ─────────────────────────────────────────────────────
    //  DEBUG COMPUTE PIPELINE (object_cull_debug.cs)
    // ─────────────────────────────────────────────────────
    {
        auto& cache = framegraph::GetPassResourceCache();
        auto* debugCsRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("object_cull_debug", ".cs");
        m_debugComputeLayout = cache.GetOrCreateBindingLayoutFromReflection("GPUCull_DebugCompute", *debugCsRefl, nvDevice);
        if (!m_debugComputeLayout) {
            Msg("! [GPUCulling] Failed to create debug compute binding layout");
            return;
        }

        nvrhi::ComputePipelineDesc pipeDesc;
        pipeDesc.CS = GEnv.Render->GetShaderLoader()->LoadComputeShader("object_cull_debug").handle;
        pipeDesc.bindingLayouts = { m_debugComputeLayout };

        m_debugComputePipeline = nvDevice->createComputePipeline(pipeDesc);
        if (!m_debugComputePipeline) {
            Msg("! [GPUCulling] Failed to create debug compute pipeline");
            return;
        }

        auto particleDebugHandle = GEnv.Render->GetShaderLoader()->LoadComputeShader("particle_cull_debug");
        if (particleDebugHandle.handle) {
            nvrhi::ComputePipelineDesc particlePipeDesc;
            particlePipeDesc.CS = particleDebugHandle.handle;
            particlePipeDesc.bindingLayouts = { m_debugComputeLayout };
            m_particleDebugComputePipeline = nvDevice->createComputePipeline(particlePipeDesc);
        }
    }

    // ─────────────────────────────────────────────────────
    //  DEBUG GRAPHICS PIPELINE (cull_debug.vs + cull_debug.ps)
    // ─────────────────────────────────────────────────────
    {
        auto& cache = framegraph::GetPassResourceCache();
        auto* debugVsRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("cull_debug", ".vs");
        auto* debugPsRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("cull_debug", ".ps");
        m_debugGraphicsLayout = cache.GetOrCreateBindingLayoutFromReflection("GPUCull_DebugGraphics", *debugVsRefl, *debugPsRefl, nvDevice);
        if (!m_debugGraphicsLayout) {
            Msg("! [GPUCulling] Failed to create debug graphics binding layout");
            return;
        }

        // No input layout needed - VS generates vertices from SV_VertexID/SV_InstanceID
        nvrhi::GraphicsPipelineDesc pipeDesc;
        pipeDesc.VS = GEnv.Render->GetShaderLoader()->LoadVertexShader("cull_debug").handle;
        pipeDesc.PS = GEnv.Render->GetShaderLoader()->LoadPixelShader("cull_debug").handle;
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
           fg::RenderContext* ctx) {

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
                cb.viewProj = Device.mFullTransform;
                cb.cameraPos = Device.vCameraPosition;
                cb.maxDistanceSq = farPlane * farPlane;
                cb.objectCount = objectCount;
                cb.hizWidth = data.hizWidth;
                cb.hizHeight = data.hizHeight;
                cb.hizMipLevels = data.hizMipLevels;
                cb.occluderThreshold = 50.0f;
                cb.debugOffset = debugOffset;

                mgr->ExtractFrustumPlanes(Device.mFullTransform, cb.frustumPlanes);
                cmdList->writeBuffer(mgr->m_device->GetNativeBuffer(mgr->m_debugComputeParamsCB), &cb, sizeof(cb));

                auto* debugCsRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("object_cull_debug", ".cs");
                framegraph::BindingSetBuilder bsb(*debugCsRefl, nvDevice, "GPUCull.Debug");
                bsb.ConstantBuffer("CullDebugParams", mgr->m_device->GetNativeBuffer(mgr->m_debugComputeParamsCB))
                   .BufferSRV("g_Objects", objectBuffer)
                   .Texture("g_HiZPyramid", hizTexture)
                   .BufferUAV("g_DebugOutput", mgr->m_debugBuffer);

                nvrhi::BindingSetHandle bindingSet = nvDevice->createBindingSet(bsb.Build(), mgr->m_debugComputeLayout);
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
                    cb.viewProj = Device.mFullTransform;
                    cb.cameraPos = Device.vCameraPosition;
                    cb.maxDistanceSq = farPlane * farPlane;
                    cb.objectCount = static_cast<u32>(mgr->m_particleData.size());
                    cb.hizWidth = data.hizWidth;
                    cb.hizHeight = data.hizHeight;
                    cb.hizMipLevels = data.hizMipLevels;
                    cb.occluderThreshold = 50.0f;
                    cb.debugOffset = data.objectCount;

                    mgr->ExtractFrustumPlanes(Device.mFullTransform, cb.frustumPlanes);
                    cmdList->writeBuffer(mgr->m_device->GetNativeBuffer(mgr->m_debugComputeParamsCB), &cb, sizeof(cb));

                    auto* particleDebugRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("particle_cull_debug", ".cs");
                    framegraph::BindingSetBuilder bsb(*particleDebugRefl, nvDevice, "GPUCull.ParticleDebug");
                    bsb.ConstantBuffer("CullDebugParams", mgr->m_device->GetNativeBuffer(mgr->m_debugComputeParamsCB))
                       .BufferSRV("g_Particles", mgr->m_particleBuffer)
                       .Texture("g_HiZPyramid", hizTexture)
                       .BufferUAV("g_DebugOutput", mgr->m_debugBuffer);

                    nvrhi::BindingSetHandle bindingSet = nvDevice->createBindingSet(bsb.Build(), mgr->m_debugComputeLayout);

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
                vsCB.view = Device.mView;
                vsCB.viewProj = Device.mFullTransform;
                vsCB.objectCount = totalDebugCount;
                vsCB.wireframeAlpha = 0.7f;

                cmdList->writeBuffer(mgr->m_device->GetNativeBuffer(mgr->m_debugGraphicsParamsCB), &vsCB, sizeof(vsCB));

                auto* debugVsRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("cull_debug", ".vs");
                auto* debugPsRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("cull_debug", ".ps");
                framegraph::BindingSetBuilder bsb(*debugVsRefl, *debugPsRefl, nvDevice, "GPUCull.DebugDraw");
                bsb.ConstantBuffer("CullDebugVSParams", mgr->m_device->GetNativeBuffer(mgr->m_debugGraphicsParamsCB))
                   .BufferSRV("g_DebugData", mgr->m_debugBuffer);

                nvrhi::BindingSetHandle bindingSet = nvDevice->createBindingSet(bsb.Build(), mgr->m_debugGraphicsLayout);

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

void GPUCullingManager::CreateParticleResources(fg::RenderDevice* device)
{
    if (!m_computeEnabled)
        return;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();

    auto particleCullResult = GEnv.Render->GetShaderLoader()->LoadComputeShader("particle_cull");
    if (!particleCullResult.handle) {
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
        desc.structStride = sizeof(u32);
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
        fg::RenderDevice::BufferDesc desc;
        desc.debugName = "GPUCull_ParticleParams";
        desc.byteSize = sizeof(CullParamsCB);
        desc.isConstantBuffer = true;
        desc.isVolatile = true;
        desc.maxVersions = fg::RenderDevice::BufferDesc::VOLATILE_CB_MAX_VERSIONS;

        m_particleCullParamsCB = m_device->CreateBuffer(desc);
        if (!m_particleCullParamsCB.IsValid()) {
            Msg("! [GPUCulling] Failed to create particle constant buffer");
            return;
        }
    }

    {
        auto& cache = framegraph::GetPassResourceCache();
        auto* particleCullRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("particle_cull", ".cs");
        m_particleCullLayout = cache.GetOrCreateBindingLayoutFromReflection("GPUCull_ParticleCull", *particleCullRefl, nvDevice);
        if (!m_particleCullLayout) {
            Msg("! [GPUCulling] Failed to create particle binding layout");
            return;
        }
    }

    {
        nvrhi::ComputePipelineDesc pipeDesc;
        pipeDesc.CS = GEnv.Render->GetShaderLoader()->LoadComputeShader("particle_cull").handle;
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

void GPUCullingManager::UploadParticleBatches(fg::RenderContext* ctx, const xr_vector<passes::ParticleBatch>* batches)
{
    ZoneScopedN("GPUCull::UploadParticles");

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
            passBuilder.asyncCompute();

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
           fg::RenderContext* ctx) {

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

            u32 frameId = Device.dwFrame + 1u;
            if (frameId == 0)
                frameId = 1;

            CullParamsCB cb;
            cb.viewProj = Device.mFullTransform;
            cb.cameraPos = Device.vCameraPosition;
            float farPlane = g_pGamePersistent ? g_pGamePersistent->Environment().CurrentEnv.far_plane : 300.0f;
            cb.maxDistanceSq = farPlane * farPlane;
            cb.objectCount = data.particleCount;
            cb.hizWidth = data.hizWidth;
            cb.hizHeight = data.hizHeight;
            cb.hizMipLevels = data.hizMipLevels;
            cb.frameId = frameId;
            cb.padding[0] = cb.padding[1] = cb.padding[2] = 0;

            mgr->ExtractFrustumPlanes(Device.mFullTransform, cb.frustumPlanes);

            cmdList->writeBuffer(mgr->m_device->GetNativeBuffer(mgr->m_particleCullParamsCB), &cb, sizeof(cb));

            auto* particleCullRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("particle_cull", ".cs");
            framegraph::BindingSetBuilder bsb(*particleCullRefl, nvDevice, "GPUCull.ParticleCull");
            bsb.ConstantBuffer("ParticleCullParams", mgr->m_device->GetNativeBuffer(mgr->m_particleCullParamsCB))
               .BufferSRV("g_ParticleData", mgr->m_particleBuffer)
               .Texture("g_HiZPyramid", hizTexture)
               .BufferUAV("g_VisibleIndices", mgr->m_particleVisibleCountBuffer)
               .BufferUAV("g_VisibleCount", mgr->m_particleDrawArgsBuffer);

            nvrhi::BindingSetHandle bindingSet = nvDevice->createBindingSet(bsb.Build(), mgr->m_particleCullLayout);
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
    m_megaDataUploaded = false;

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
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        desc.canHaveRawViews = true;
        desc.isAccelStructBuildInput = nvDevice->queryFeatureSupport(nvrhi::Feature::RayTracingAccelStruct);

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
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        desc.isIndexBuffer = true;
        desc.canHaveRawViews = true;
        desc.isAccelStructBuildInput = nvDevice->queryFeatureSupport(nvrhi::Feature::RayTracingAccelStruct);

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

void GPUCullingManager::UploadInstanceData(fg::RenderContext* ctx, const GeometryCollector* geometry)
{
    (void)ctx;
    (void)geometry;
    // Instance data uploads are handled in UploadSceneObjects() for static/dynamic sets.
}

// ═══════════════════════════════════════════════════════
//  VB/IB POOL REGISTRATION (for level geometry)
// ═══════════════════════════════════════════════════════

bindless::SourceVertexFormat GPUCullingManager::DetectFormatFromDecl(
    const VertexElement* decl,
    u32 stride)
{
    if (!decl)
        return bindless::SourceVertexFormat::Unknown;

    bool hasColor = false;
    bool hasTexCoord1 = false;
    bool hasShort4TexCoord = false;
    bool hasFloat2TexCoord = false;
    u32 texcoord0Type = 0;

    for (int i = 0; decl[i].Stream != 0xFF; i++) {
        const VertexElement& elem = decl[i];

        if (elem.Usage == VS_COLOR && elem.UsageIndex == 0) {
            hasColor = true;
        }
        else if (elem.Usage == VS_TEXCOORD) {
            if (elem.UsageIndex == 0) {
                texcoord0Type = elem.Type;
                if (elem.Type == VF_SHORT4) hasShort4TexCoord = true;
                if (elem.Type == VF_FLOAT2) hasFloat2TexCoord = true;
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
    const VertexElement* decl,
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

} // namespace xray::render::fg
