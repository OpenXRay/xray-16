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
#include "Layers/xrRender/SkeletonCustom.h"
#include "Layers/xrRender/ShaderVariant/ShaderVariantRegistry.h"
#include "Layers/xrRender/Bindless/MaterialBuffer.h"
#include "Layers/xrRender/Bindless/TerrainMaterialBuffer.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/RayTracing/RTAccelStructManager.h"

namespace RENDER_NAMESPACE
{
    extern CRender RImplementation;
}

namespace xray::render::RENDER_NAMESPACE {

constexpr u32 MAX_CULLING_OBJECTS = 65536;
constexpr u32 CULL_THREAD_GROUP_SIZE = 64;
constexpr u32 COMPACT_THREAD_GROUP_SIZE = 256;

struct CullParamsCB {
    Fmatrix viewProj;
    Fmatrix prevViewProj;
    Fvector cameraPos;
    float maxDistanceSq;
    Fvector4 frustumPlanes[6];
    u32 objectCount;
    u32 hizWidth;
    u32 hizHeight;
    u32 hizMipLevels;
    u32 frameId;
    u32 padding[3];
};

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
    Fmatrix view;
    Fmatrix viewProj;
    u32 objectCount;
    float wireframeAlpha;
    float padding[2];
};

static ref_cs s_object_cull_cs;
static ref_cs s_object_cull_debug_cs;
static ref_vs s_cull_debug_vs;
static ref_ps s_cull_debug_ps;
static ref_cs s_particle_cull_cs;
static ref_cs s_particle_cull_debug_cs;
static ref_cs s_batch_compact_count_cs;
static ref_cs s_batch_compact_scan_cs;
static ref_cs s_batch_compact_scatter_cs;
static ref_cs s_terrain_apply_visibility_cs;
static ref_cs s_variant_partition_cs;

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
    CreateVariantPartitionResources(device);
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

        {
            nvrhi::BufferDesc desc;
            desc.debugName = pickName("GPUCull_Static_VisibleCount", "GPUCull_Dynamic_VisibleCount");
            desc.byteSize = sizeof(u32);
            desc.canHaveUAVs = true;
            desc.canHaveRawViews = true;
            desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
            desc.keepInitialState = true;

            set.visibleCountBuffer = nvDevice->createBuffer(desc);
            R_ASSERT2(set.visibleCountBuffer, "Failed to create visible count buffer");
        }

        {
            nvrhi::BufferDesc desc;
            desc.debugName = pickName("GPUCull_Static_DrawArgs", "GPUCull_Dynamic_DrawArgs");
            desc.byteSize = m_maxObjects * sizeof(IndirectDrawArgs);
            desc.canHaveUAVs = true;
            desc.canHaveRawViews = true;
            desc.isDrawIndirectArgs = true;
            desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
            desc.keepInitialState = true;

            set.drawArgsBuffer = nvDevice->createBuffer(desc);
            R_ASSERT2(set.drawArgsBuffer, "Failed to create draw args buffer");
        }

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

    {
        nvrhi::SamplerDesc desc;
        desc.minFilter = false;
        desc.magFilter = false;
        desc.mipFilter = false;
        desc.addressU = nvrhi::SamplerAddressMode::Clamp;
        desc.addressV = nvrhi::SamplerAddressMode::Clamp;
        desc.addressW = nvrhi::SamplerAddressMode::Clamp;

        m_pointSampler = nvDevice->createSampler(desc);
    }

    m_maxTerrainObjects = 4096;

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

    CreateSkinnedCullingBuffers(device);
}

void GPUCullingManager::CreateSkinnedCullingBuffers(ng::RenderDevice* device)
{
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();

    m_maxSkinnedObjects = 1024;

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

    m_skinnedObjectData.reserve(m_maxSkinnedObjects);
    m_skinnedBatchPointers.reserve(m_maxSkinnedObjects);

    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GlobalBoneBuffer";
        desc.byteSize = MAX_TOTAL_BONES * BONE_STRIDE;
        desc.structStride = BONE_STRIDE;
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;

        m_globalBoneBuffer = nvDevice->createBuffer(desc);
        if (!m_globalBoneBuffer) {
            Msg("! [GPUCulling] Failed to create global bone buffer");
            return;
        }
    }

    m_boneStagingBuffer.resize(256);
    m_boneBufferInitialized = true;

    m_skinnedCullEnabled = true;
    Msg("* [GPUCulling] Skinned culling buffers created (max: %u objects, %u bones)",
        m_maxSkinnedObjects, MAX_TOTAL_BONES);
}

void GPUCullingManager::EnsureSkinnedBufferCapacity(u32 count)
{
    if (count <= m_maxSkinnedObjects)
        return;

    u32 newCapacity = std::max(count, m_maxSkinnedObjects * 2);
    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();

    Msg("* [GPUCulling] Growing skinned buffers: %u -> %u", m_maxSkinnedObjects, newCapacity);

    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_SkinnedObjects";
        desc.byteSize = newCapacity * sizeof(GPUObjectData);
        desc.structStride = sizeof(GPUObjectData);
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;

        m_skinnedObjectBuffer = nvDevice->createBuffer(desc);
    }

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
        m_skinnedVisualToIndex[i].clear();
    }
    m_skinnedVisibilityByVisual.clear();

    m_maxSkinnedObjects = newCapacity;
}

void GPUCullingManager::CreateComputePipeline(ng::RenderDevice* device)
{
    if (!m_computeEnabled)
        return;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();

    {
        nvrhi::BindingLayoutDesc layoutDesc;
        layoutDesc.visibility = nvrhi::ShaderType::Compute;
        layoutDesc.bindings = {
            nvrhi::BindingLayoutItem::VolatileConstantBuffer(5),
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(0),
            nvrhi::BindingLayoutItem::Texture_SRV(1),
            nvrhi::BindingLayoutItem::Sampler(0),
            nvrhi::BindingLayoutItem::StructuredBuffer_UAV(0),
            nvrhi::BindingLayoutItem::RawBuffer_UAV(1),
            nvrhi::BindingLayoutItem::StructuredBuffer_UAV(2)
        };

        m_cullLayout = nvDevice->createBindingLayout(layoutDesc);
        if (!m_cullLayout) {
            Msg("! [GPUCulling] Failed to create binding layout");
            m_computeEnabled = false;
            return;
        }
    }

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

    s_batch_compact_count_cs.create("batch_compact_count");
    R_ASSERT2(s_batch_compact_count_cs && s_batch_compact_count_cs->nvrhiShader,
        "batch_compact_count.cs not found - compaction requires this shader");
    s_batch_compact_scan_cs.create("batch_compact_scan");
    R_ASSERT2(s_batch_compact_scan_cs && s_batch_compact_scan_cs->nvrhiShader,
        "batch_compact_scan.cs not found - compaction requires this shader");
    s_batch_compact_scatter_cs.create("batch_compact");
    R_ASSERT2(s_batch_compact_scatter_cs && s_batch_compact_scatter_cs->nvrhiShader,
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
                desc.canHaveUAVs = true;
                desc.canHaveRawViews = true;
                desc.isDrawIndirectArgs = true;
                desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
                desc.keepInitialState = true;

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
                desc.keepInitialState = true;

                set.compactBatchIndicesBuffer = nvDevice->createBuffer(desc);
                R_ASSERT2(set.compactBatchIndicesBuffer, "Failed to create compact batch indices buffer");
            }

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
                desc.keepInitialState = true;

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
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_CompactParams";
        desc.byteSize = 16;
        desc.isConstantBuffer = true;
        desc.isVolatile = true;
        desc.maxVersions = 16;

        m_compactParamsCB = nvDevice->createBuffer(desc);
        R_ASSERT2(m_compactParamsCB, "Failed to create compact params CB");
    }

    {
        nvrhi::BindingLayoutDesc layoutDesc;
        layoutDesc.visibility = nvrhi::ShaderType::Compute;
        layoutDesc.bindings = {
            nvrhi::BindingLayoutItem::VolatileConstantBuffer(5),
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(0),
            nvrhi::BindingLayoutItem::StructuredBuffer_UAV(0),
            nvrhi::BindingLayoutItem::StructuredBuffer_UAV(1)
        };

        m_compactCountLayout = nvDevice->createBindingLayout(layoutDesc);
        R_ASSERT2(m_compactCountLayout, "Failed to create compact count binding layout");
    }

    {
        nvrhi::BindingLayoutDesc layoutDesc;
        layoutDesc.visibility = nvrhi::ShaderType::Compute;
        layoutDesc.bindings = {
            nvrhi::BindingLayoutItem::VolatileConstantBuffer(5),
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(0),
            nvrhi::BindingLayoutItem::StructuredBuffer_UAV(0),
            nvrhi::BindingLayoutItem::RawBuffer_UAV(1),
            nvrhi::BindingLayoutItem::RawBuffer_UAV(2)
        };

        m_compactScanLayout = nvDevice->createBindingLayout(layoutDesc);
        R_ASSERT2(m_compactScanLayout, "Failed to create compact scan binding layout");
    }

    {
        nvrhi::BindingLayoutDesc layoutDesc;
        layoutDesc.visibility = nvrhi::ShaderType::Compute;
        layoutDesc.bindings = {
            nvrhi::BindingLayoutItem::VolatileConstantBuffer(5),
            nvrhi::BindingLayoutItem::RawBuffer_SRV(0),
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(1),
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(2),
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(3),
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(4),
            nvrhi::BindingLayoutItem::RawBuffer_UAV(0),
            nvrhi::BindingLayoutItem::StructuredBuffer_UAV(1),
            nvrhi::BindingLayoutItem::StructuredBuffer_UAV(2)
        };

        m_compactScatterLayout = nvDevice->createBindingLayout(layoutDesc);
        R_ASSERT2(m_compactScatterLayout, "Failed to create compact scatter binding layout");
    }

    {
        nvrhi::ComputePipelineDesc pipeDesc;
        pipeDesc.CS = s_batch_compact_count_cs->nvrhiShader;
        pipeDesc.bindingLayouts = { m_compactCountLayout };

        m_compactCountPipeline = nvDevice->createComputePipeline(pipeDesc);
        R_ASSERT2(m_compactCountPipeline, "Failed to create compact count pipeline");
    }

    {
        nvrhi::ComputePipelineDesc pipeDesc;
        pipeDesc.CS = s_batch_compact_scan_cs->nvrhiShader;
        pipeDesc.bindingLayouts = { m_compactScanLayout };

        m_compactScanPipeline = nvDevice->createComputePipeline(pipeDesc);
        R_ASSERT2(m_compactScanPipeline, "Failed to create compact scan pipeline");
    }

    {
        nvrhi::ComputePipelineDesc pipeDesc;
        pipeDesc.CS = s_batch_compact_scatter_cs->nvrhiShader;
        pipeDesc.bindingLayouts = { m_compactScatterLayout };

        m_compactScatterPipeline = nvDevice->createComputePipeline(pipeDesc);
        R_ASSERT2(m_compactScatterPipeline, "Failed to create compact scatter pipeline");
    }

    m_compactEnabled = true;
    Msg("* [GPUCulling] Compaction resources created");

    s_terrain_apply_visibility_cs.create("terrain_apply_visibility");
    if (s_terrain_apply_visibility_cs && s_terrain_apply_visibility_cs->nvrhiShader) {
        nvrhi::BindingLayoutDesc layoutDesc;
        layoutDesc.visibility = nvrhi::ShaderType::Compute;
        layoutDesc.bindings = {
            nvrhi::BindingLayoutItem::VolatileConstantBuffer(5),
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(0),
            nvrhi::BindingLayoutItem::RawBuffer_UAV(0)
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

void GPUCullingManager::CreateVariantPartitionResources(ng::RenderDevice* device)
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

    s_variant_partition_cs.create("variant_partition");
    if (!s_variant_partition_cs || !s_variant_partition_cs->nvrhiShader) {
        Msg("! [GPUCulling] variant_partition.cs not found - variant partition disabled");
        return;
    }

    {
        nvrhi::BindingLayoutDesc layoutDesc;
        layoutDesc.visibility = nvrhi::ShaderType::Compute;
        layoutDesc.bindings = {
            nvrhi::BindingLayoutItem::VolatileConstantBuffer(5),
            nvrhi::BindingLayoutItem::RawBuffer_SRV(0),
            nvrhi::BindingLayoutItem::RawBuffer_SRV(1),
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(2),
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(3),
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(8),
            nvrhi::BindingLayoutItem::StructuredBuffer_UAV(0),
            nvrhi::BindingLayoutItem::RawBuffer_UAV(1),
            nvrhi::BindingLayoutItem::StructuredBuffer_UAV(2),
            nvrhi::BindingLayoutItem::StructuredBuffer_UAV(3)
        };
        m_variantPartitionLayout = nvDevice->createBindingLayout(layoutDesc);
        R_ASSERT2(m_variantPartitionLayout, "Failed to create variant partition layout");
    }

    {
        nvrhi::ComputePipelineDesc pipeDesc;
        pipeDesc.CS = s_variant_partition_cs->nvrhiShader;
        pipeDesc.bindingLayouts = { m_variantPartitionLayout };
        m_variantPartitionPipeline = nvDevice->createComputePipeline(pipeDesc);
        R_ASSERT2(m_variantPartitionPipeline, "Failed to create variant partition pipeline");
    }

    {
        nvrhi::BufferDesc desc;
        desc.debugName = "VariantPartition_Params";
        desc.byteSize = 16;
        desc.isConstantBuffer = true;
        desc.isVolatile = true;
        desc.maxVersions = 16;
        m_variantPartitionParamsCB = nvDevice->createBuffer(desc);
        R_ASSERT2(m_variantPartitionParamsCB, "Failed to create variant partition params CB");
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
        desc.initialState = nvrhi::ResourceStates::VertexBuffer;
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
    cmdList->writeBuffer(m_variantPartitionParamsCB, &params, sizeof(params));

    nvrhi::BindingSetDesc bindDesc;
    bindDesc.bindings = {
        nvrhi::BindingSetItem::ConstantBuffer(5, m_variantPartitionParamsCB),
        nvrhi::BindingSetItem::RawBuffer_SRV(0, set.compactCountBuffer),
        nvrhi::BindingSetItem::RawBuffer_SRV(1, set.compactDrawArgsBuffer),
        nvrhi::BindingSetItem::StructuredBuffer_SRV(2, set.compactBatchIndicesBuffer),
        nvrhi::BindingSetItem::StructuredBuffer_SRV(3, set.compactMaterialIDBuffer),
        nvrhi::BindingSetItem::StructuredBuffer_SRV(8, materialBuffer),
        nvrhi::BindingSetItem::StructuredBuffer_UAV(0, partition.variantCountBuffer),
        nvrhi::BindingSetItem::RawBuffer_UAV(1, partition.reorderedDrawArgsBuffer),
        nvrhi::BindingSetItem::StructuredBuffer_UAV(2, partition.reorderedBatchIndicesBuffer),
        nvrhi::BindingSetItem::StructuredBuffer_UAV(3, partition.reorderedMaterialIDsBuffer)
    };
    nvrhi::BindingSetHandle bindingSet = framegraph::GetPassResourceCache().GetOrCreateBindingSet(bindDesc, m_variantPartitionLayout, nvDevice);
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

    m_cullParamsCB = nullptr;
    m_cullPipeline = nullptr;
    m_cullLayout = nullptr;
    m_pointSampler = nullptr;

    m_compactParamsCB = nullptr;
    m_compactCountPipeline = nullptr;
    m_compactScanPipeline = nullptr;
    m_compactScatterPipeline = nullptr;
    m_compactCountLayout = nullptr;
    m_compactScanLayout = nullptr;
    m_compactScatterLayout = nullptr;

    m_variantPartitionPipeline = nullptr;
    m_variantPartitionLayout = nullptr;
    m_variantPartitionParamsCB = nullptr;
    m_staticPartition = {};
    m_transparentPartition = {};
    m_variantPartitionEnabled = false;

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

    m_staticTerrainDrawArgsUploaded = false;
    m_staticDataCached = false;

    m_initialized = false;
    m_computeEnabled = false;
    m_compactEnabled = false;
    m_particleCullEnabled = false;
    m_objectCount = 0;
    m_particleCount = 0;

    m_statsReadbackBuffer = nullptr;
    m_statsReadbackPending = false;
    m_cullingStats = CullingStats();
}

void GPUCullingManager::ScheduleStatsReadback(nvrhi::ICommandList* cmdList)
{
    if (!m_compactEnabled || !m_device)
        return;

    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();
    if (!nvDevice)
        return;

    if (!m_statsReadbackBuffer)
    {
        nvrhi::BufferDesc desc;
        desc.byteSize = sizeof(u32) * 3;
        desc.debugName = "CullingStatsReadback";
        desc.cpuAccess = nvrhi::CpuAccessMode::Read;
        desc.initialState = nvrhi::ResourceStates::CopyDest;
        desc.keepInitialState = true;
        m_statsReadbackBuffer = nvDevice->createBuffer(desc);

        if (!m_statsReadbackBuffer)
            return;
    }

    if (m_staticSet.compactCountBuffer)
    {
        cmdList->copyBuffer(
            m_statsReadbackBuffer, 0,
            m_staticSet.compactCountBuffer, 0,
            sizeof(u32)
        );
    }

    if (m_dynamicSet.compactCountBuffer)
    {
        cmdList->copyBuffer(
            m_statsReadbackBuffer, sizeof(u32),
            m_dynamicSet.compactCountBuffer, 0,
            sizeof(u32)
        );
    }

    if (m_terrainCompactCountBuffer)
    {
        cmdList->copyBuffer(
            m_statsReadbackBuffer, sizeof(u32) * 2,
            m_terrainCompactCountBuffer, 0,
            sizeof(u32)
        );
    }

    m_statsReadbackPending = true;
}

void GPUCullingManager::ProcessStatsReadback()
{
    if (!m_statsReadbackPending || !m_statsReadbackBuffer || !m_device)
        return;

    static u32 frameCounter = 0;
    frameCounter++;
    const u32 throttleInterval = xray::profiler::GetCPUProfiler().GetThrottleInterval();
    if ((frameCounter % throttleInterval) != 0)
        return;

    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();
    if (!nvDevice)
        return;

    void* mappedData = nvDevice->mapBuffer(m_statsReadbackBuffer, nvrhi::CpuAccessMode::Read);
    if (mappedData)
    {
        const u32* counts = static_cast<const u32*>(mappedData);
        m_cullingStats.staticVisible = counts[0];
        m_cullingStats.dynamicVisible = counts[1];
        m_cullingStats.terrainVisible = counts[2];
        nvDevice->unmapBuffer(m_statsReadbackBuffer);
    }

    m_statsReadbackPending = false;
}

void GPUCullingManager::UploadSceneObjects(ng::RenderContext* ctx, const GeometryCollector* geometry)
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

    m_terrainObjectData.clear();
    m_terrainObjectData.reserve(totalBatches / 4);
    m_terrainDrawArgsData.clear();
    m_terrainDrawArgsData.reserve(totalBatches / 4);
    m_terrainMaterialIDData.clear();
    m_terrainMaterialIDData.reserve(totalBatches / 4);
    m_terrainInstanceData.clear();
    m_terrainInstanceData.reserve(totalBatches / 4);

    m_transparentObjectData.clear();
    m_transparentDrawArgsData.clear();
    m_transparentMaterialIDData.clear();
    m_transparentInstanceData.clear();

    auto appendBatch = [&](const GeometryBatch& batch,
                           xr_vector<GPUObjectData>& objectData,
                           xr_vector<IndirectDrawArgs>& drawArgsData,
                           xr_vector<u32>& materialIDData,
                           xr_vector<GPUInstanceData>& instanceData) {
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

        IndirectDrawArgs args;
        args.indexCountPerInstance = batch.indexCount;
        args.instanceCount = 1;
        if (batch.megaBufferAlloc.valid) {
            args.startIndexLocation = batch.megaBufferAlloc.indexOffset;
            args.baseVertexLocation = static_cast<s32>(batch.megaBufferAlloc.vertexOffset);
        } else {
            args.startIndexLocation = batch.startIndex;
            args.baseVertexLocation = batch.baseVertex;
        }
        args.startInstanceLocation = 0;
        drawArgsData.push_back(args);

        materialIDData.push_back(batch.bindlessMaterialID);

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

        if (batch.isSkinned)
            continue;

        if (batch.isTerrain) {
            GPUObjectData obj;
            obj.position = batch.worldBoundsCenter;
            obj.radius = batch.worldBoundsRadius;
            obj.batchIndex = static_cast<u32>(m_terrainObjectData.size());
            obj.flags = GPU_OBJECT_OPAQUE;
            obj.pad0 = 0.0f;
            obj.pad1 = 0.0f;
            m_terrainObjectData.push_back(obj);

            IndirectDrawArgs args;
            args.indexCountPerInstance = batch.indexCount;
            args.instanceCount = 0;
            if (batch.megaBufferAlloc.valid) {
                args.startIndexLocation = batch.megaBufferAlloc.indexOffset;
                args.baseVertexLocation = static_cast<s32>(batch.megaBufferAlloc.vertexOffset);
            } else {
                args.startIndexLocation = batch.startIndex;
                args.baseVertexLocation = batch.baseVertex;
            }
            args.startInstanceLocation = static_cast<u32>(m_terrainDrawArgsData.size());
            m_terrainDrawArgsData.push_back(args);

            m_terrainMaterialIDData.push_back(batch.terrainMaterialID);

            GPUInstanceData inst;
            inst.world = batch.worldMatrix;
            inst.materialID = batch.terrainMaterialID;
            inst.flags = GPU_OBJECT_OPAQUE;
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

    nvrhi::ICommandList* cmdList = ctx->GetCommandList();

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

    m_terrainObjectCount = std::min(static_cast<u32>(m_terrainObjectData.size()), m_maxTerrainObjects);

    if (m_terrainObjectCount > 0 && m_terrainObjectBuffer && m_terrainDrawArgsBuffer) {
        R_ASSERT2(m_terrainObjectCount <= m_maxTerrainObjects, "Terrain object count exceeds buffer capacity");
        R_ASSERT2(m_terrainDrawArgsData.size() >= m_terrainObjectCount, "Terrain draw args data smaller than object count");
        R_ASSERT2(m_terrainMaterialIDData.size() >= m_terrainObjectCount, "Terrain material ID data smaller than object count");
        R_ASSERT2(m_terrainInstanceData.size() >= m_terrainObjectCount, "Terrain instance data smaller than object count");

        cmdList->writeBuffer(m_terrainObjectBuffer,
            m_terrainObjectData.data(),
            m_terrainObjectCount * sizeof(GPUObjectData));
        cmdList->setBufferState(m_terrainObjectBuffer, nvrhi::ResourceStates::ShaderResource);

        if (!m_staticTerrainDrawArgsUploaded) {
            for (auto& args : m_terrainDrawArgsData) {
                args.instanceCount = 1;
            }

            cmdList->writeBuffer(m_terrainDrawArgsBuffer,
                m_terrainDrawArgsData.data(),
                m_terrainObjectCount * sizeof(IndirectDrawArgs));
            cmdList->setBufferState(m_terrainDrawArgsBuffer, nvrhi::ResourceStates::IndirectArgument);

            if (m_terrainMaterialIDBuffer) {
                cmdList->writeBuffer(m_terrainMaterialIDBuffer,
                    m_terrainMaterialIDData.data(),
                    m_terrainObjectCount * sizeof(u32));
                cmdList->setBufferState(m_terrainMaterialIDBuffer, nvrhi::ResourceStates::ShaderResource);

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

            if (m_terrainInstanceBuffer && !m_terrainInstanceData.empty()) {
                cmdList->writeBuffer(m_terrainInstanceBuffer,
                    m_terrainInstanceData.data(),
                    m_terrainObjectCount * sizeof(GPUInstanceData));
                cmdList->setBufferState(m_terrainInstanceBuffer, nvrhi::ResourceStates::ShaderResource);
            }

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

void GPUCullingManager::UploadSkinnedObjects(ng::RenderContext* ctx, const GeometryCollector* geometry)
{
    ZoneScopedN("GPUCull::UploadSkinnedObjects");

    if (!m_skinnedCullEnabled || !geometry) {
        m_skinnedObjectCount = 0;
        return;
    }

    const auto& batches = geometry->GetBatches();

    m_skinnedObjectData.clear();
    m_skinnedBatchPointers.clear();

    for (const auto& batch : batches) {
        if (!batch.isSkinned)
            continue;

        u32 batchIdx = static_cast<u32>(m_skinnedObjectData.size());

        GPUObjectData obj;
        obj.position = batch.worldBoundsCenter;
        obj.radius = batch.worldBoundsRadius;
        obj.batchIndex = batchIdx;
        obj.flags = 0;
        obj.pad0 = 0.0f;
        obj.pad1 = 0.0f;
        m_skinnedObjectData.push_back(obj);
        m_skinnedBatchPointers.push_back(&batch);
    }

    m_skinnedObjectCount = static_cast<u32>(m_skinnedObjectData.size());

    if (m_skinnedObjectCount == 0)
        return;

    EnsureSkinnedBufferCapacity(m_skinnedObjectCount);

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

    cmdList->copyBuffer(
        m_skinnedReadbackBuffers[writeSlot], 0,
        m_skinnedVisibilityBuffer, 0,
        m_skinnedObjectCount * sizeof(u32));

    m_skinnedVisualToIndex[writeSlot].clear();
    for (u32 i = 0; i < m_skinnedBatchPointers.size(); ++i) {
        const GeometryBatch* batch = m_skinnedBatchPointers[i];
        if (batch && batch->visual) {
            m_skinnedVisualToIndex[writeSlot][batch->visual] = i;
        }
    }

    m_skinnedReadbackWriteIndex = 1 - m_skinnedReadbackWriteIndex;

    if (m_skinnedReadbackFrameCount < SKINNED_READBACK_FRAMES) {
        m_skinnedReadbackFrameCount++;
    }
}

void GPUCullingManager::ProcessSkinnedVisibilityReadback()
{
    if (m_skinnedReadbackFrameCount < SKINNED_READBACK_FRAMES || m_skinnedObjectCount == 0) {
        return;
    }

    u32 readSlot = m_skinnedReadbackWriteIndex;

    void* mappedData = m_device->GetNVRHIDevice()->mapBuffer(
        m_skinnedReadbackBuffers[readSlot],
        nvrhi::CpuAccessMode::Read);

    if (mappedData) {
        const u32* visibilityData = static_cast<const u32*>(mappedData);
        const auto& visualToIndex = m_skinnedVisualToIndex[readSlot];

        m_skinnedVisibilityByVisual.clear();
        for (const auto& [visual, index] : visualToIndex) {
            if (index < m_maxSkinnedObjects) {
                m_skinnedVisibilityByVisual[visual] = visibilityData[index];
            }
        }

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
    auto it = m_skinnedVisibilityByVisual.find(visual);
    if (it != m_skinnedVisibilityByVisual.end()) {
        return it->second;
    }
    return 0;
}

void GPUCullingManager::ClearSkinnedVisibilityData()
{
    for (u32 i = 0; i < SKINNED_READBACK_FRAMES; ++i) {
        m_skinnedVisualToIndex[i].clear();
    }
    m_skinnedVisibilityByVisual.clear();
    m_skinnedBatchPointers.clear();
    m_skinnedReadbackFrameCount = 0;
}

void GPUCullingManager::BeginSkinnedFrame()
{
    m_skeletonOffsets.clear();
    m_currentBoneOffset = 0;
}

u32 GPUCullingManager::GetOrUploadSkeleton(nvrhi::ICommandList* cmdList, CKinematics* skeleton)
{
    if (!m_boneBufferInitialized || !skeleton || !cmdList)
        return 0;

    auto it = m_skeletonOffsets.find(skeleton);
    if (it != m_skeletonOffsets.end()) {
        return it->second;
    }

    u32 boneCount = skeleton->LL_BoneCount();
    if (boneCount == 0)
        return 0;

    if (m_currentBoneOffset + boneCount > MAX_TOTAL_BONES) {
        Msg("! [GPUCulling] Bone buffer full: need %u bones, have %u remaining",
            boneCount, MAX_TOTAL_BONES - m_currentBoneOffset);
        return 0;
    }

    u32 boneOffset = m_currentBoneOffset;
    m_skeletonOffsets[skeleton] = boneOffset;

    UploadSkeletonBones(cmdList, skeleton, boneOffset);

    m_currentBoneOffset += boneCount;

    return boneOffset;
}

void GPUCullingManager::UploadSkeletonBones(nvrhi::ICommandList* cmdList, CKinematics* skeleton, u32 boneOffset)
{
    u32 boneCount = skeleton->LL_BoneCount();

    if (m_boneStagingBuffer.size() < boneCount) {
        m_boneStagingBuffer.resize(boneCount);
    }

    for (u32 i = 0; i < boneCount; i++) {
        m_boneStagingBuffer[i] = skeleton->LL_GetTransform_R(u16(i));
    }

    u64 byteOffset = static_cast<u64>(boneOffset) * BONE_STRIDE;
    u64 byteSize = static_cast<u64>(boneCount) * BONE_STRIDE;

    cmdList->writeBuffer(m_globalBoneBuffer, m_boneStagingBuffer.data(), byteSize, byteOffset);
}

void GPUCullingManager::ExtractFrustumPlanes(Fmatrix& M, Fvector4* outPlanes)
{
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

    if (!m_computeEnabled) {
        return output;
    }

    if (!geometry || geometry->GetBatches().empty()) {
        return output;
    }

    struct GPUCullPassData {
        VirtualResourceHandle hizPyramid;
        VirtualResourceHandle staticDrawArgsBuffer;
        VirtualResourceHandle dynamicDrawArgsBuffer;

        GPUCullingManager* manager;
        const GeometryCollector* geometry;
        Fmatrix prevViewProj;
        u32 hizWidth;
        u32 hizHeight;
        u32 hizMipLevels;
    };

    ResourceDesc drawArgsDesc;
    drawArgsDesc.type = ResourceDesc::Type::Buffer;
    drawArgsDesc.debugName = "GPUCull_DrawArgs";
    drawArgsDesc.bufferSize = m_maxObjects * sizeof(IndirectDrawArgs);
    drawArgsDesc.structStride = sizeof(IndirectDrawArgs);
    drawArgsDesc.isUAV = true;
    drawArgsDesc.isTransient = false;

    VirtualResourceHandle staticDrawArgsHandle = fg.ImportBuffer("gpu_cull_static_drawargs", m_staticSet.drawArgsBuffer, drawArgsDesc);
    VirtualResourceHandle dynamicDrawArgsHandle = fg.ImportBuffer("gpu_cull_dynamic_drawargs", m_dynamicSet.drawArgsBuffer, drawArgsDesc);

    auto& passData = fg.addCallbackPass<GPUCullPassData>(
        "GPU Culling",

        [&, hizWidth, hizHeight, hizMipLevels, staticDrawArgsHandle, dynamicDrawArgsHandle, geometry, prevViewProj](FrameGraph& builder, PassHandle passHandle, GPUCullPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);
            passBuilder.asyncCompute();

            data.manager = this;
            data.geometry = geometry;
            data.prevViewProj = prevViewProj;
            data.hizWidth = hizWidth;
            data.hizHeight = hizHeight;
            data.hizMipLevels = hizMipLevels;

            data.hizPyramid = passBuilder.read(hizPyramid, ResourceState::ShaderResource);

            data.staticDrawArgsBuffer = passBuilder.write(staticDrawArgsHandle, ResourceState::UnorderedAccess);
            data.dynamicDrawArgsBuffer = passBuilder.write(dynamicDrawArgsHandle, ResourceState::UnorderedAccess);
        },

        [](const GPUCullPassData& data,
           const FrameGraph& fg,
           ng::RenderContext* ctx) {

            GPUCullingManager* mgr = data.manager;
            if (!mgr->m_computeEnabled)
                return;

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();

            nvrhi::IDevice* nvDevice = mgr->m_device->GetNVRHIDevice();
            u32 frameId = Device.dwFrame + 1u;
            if (frameId == 0)
                frameId = 1;

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

                u32 zero = 0;
                cmdList->writeBuffer(set.visibleCountBuffer, &zero, sizeof(u32));

                CullParamsCB cb;
                cb.viewProj = Device.mFullTransform;
                cb.prevViewProj = data.prevViewProj;
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

                cmdList->writeBuffer(mgr->m_cullParamsCB, &cb, sizeof(cb));

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

                nvrhi::ComputeState state;
                state.pipeline = mgr->m_cullPipeline;
                state.bindings = { bindingSet };
                cmdList->setComputeState(state);

                u32 groupCount = (set.objectCount + CULL_THREAD_GROUP_SIZE - 1) / CULL_THREAD_GROUP_SIZE;
                cmdList->dispatch(groupCount, 1, 1);

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
                    cmdList->writeBuffer(mgr->m_compactParamsCB, &compactCB, sizeof(compactCB));

                    if (compactGroupCount == 0) {
                        u32 zeroCount = 0;
                        cmdList->writeBuffer(set.compactCountBuffer, &zeroCount, sizeof(u32));
                        u32 zeroDispatch[3] = { 0, 1, 1 };
                        cmdList->writeBuffer(set.compactDispatchArgsBuffer, zeroDispatch, sizeof(zeroDispatch));
                    } else {
                        cmdList->setBufferState(set.visibilityBuffer, nvrhi::ResourceStates::ShaderResource);
                        cmdList->setBufferState(set.compactLocalPrefixBuffer, nvrhi::ResourceStates::UnorderedAccess);
                        cmdList->setBufferState(set.compactGroupCountsBuffer, nvrhi::ResourceStates::UnorderedAccess);

                        nvrhi::BindingSetDesc countBindDesc;
                        countBindDesc.bindings = {
                            nvrhi::BindingSetItem::ConstantBuffer(5, mgr->m_compactParamsCB),
                            nvrhi::BindingSetItem::StructuredBuffer_SRV(0, set.visibilityBuffer),
                            nvrhi::BindingSetItem::StructuredBuffer_UAV(0, set.compactLocalPrefixBuffer),
                            nvrhi::BindingSetItem::StructuredBuffer_UAV(1, set.compactGroupCountsBuffer)
                        };

                        nvrhi::BindingSetHandle countBindingSet = framegraph::GetPassResourceCache().GetOrCreateBindingSet(countBindDesc, mgr->m_compactCountLayout, nvDevice);
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

                        nvrhi::BindingSetDesc scanBindDesc;
                        scanBindDesc.bindings = {
                            nvrhi::BindingSetItem::ConstantBuffer(5, mgr->m_compactParamsCB),
                            nvrhi::BindingSetItem::StructuredBuffer_SRV(0, set.compactGroupCountsBuffer),
                            nvrhi::BindingSetItem::StructuredBuffer_UAV(0, set.compactGroupOffsetsBuffer),
                            nvrhi::BindingSetItem::RawBuffer_UAV(1, set.compactCountBuffer),
                            nvrhi::BindingSetItem::RawBuffer_UAV(2, set.compactDispatchArgsBuffer)
                        };

                        nvrhi::BindingSetHandle scanBindingSet = framegraph::GetPassResourceCache().GetOrCreateBindingSet(scanBindDesc, mgr->m_compactScanLayout, nvDevice);
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

                        nvrhi::BindingSetDesc scatterBindDesc;
                        scatterBindDesc.bindings = {
                            nvrhi::BindingSetItem::ConstantBuffer(5, mgr->m_compactParamsCB),
                            nvrhi::BindingSetItem::RawBuffer_SRV(0, set.drawArgsBuffer),
                            nvrhi::BindingSetItem::StructuredBuffer_SRV(1, set.materialIDBuffer),
                            nvrhi::BindingSetItem::StructuredBuffer_SRV(2, set.visibilityBuffer),
                            nvrhi::BindingSetItem::StructuredBuffer_SRV(3, set.compactLocalPrefixBuffer),
                            nvrhi::BindingSetItem::StructuredBuffer_SRV(4, set.compactGroupOffsetsBuffer),
                            nvrhi::BindingSetItem::RawBuffer_UAV(0, set.compactDrawArgsBuffer),
                            nvrhi::BindingSetItem::StructuredBuffer_UAV(1, set.compactBatchIndicesBuffer),
                            nvrhi::BindingSetItem::StructuredBuffer_UAV(2, set.compactMaterialIDBuffer)
                        };

                        nvrhi::BindingSetHandle scatterBindingSet = framegraph::GetPassResourceCache().GetOrCreateBindingSet(scatterBindDesc, mgr->m_compactScatterLayout, nvDevice);
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

            if (mgr->m_terrainObjectCount > 0) {
                R_ASSERT2(mgr->m_terrainObjectBuffer && mgr->m_terrainDrawArgsBuffer,
                    "Terrain buffers not initialized for culling");
            }

            if (mgr->m_terrainObjectCount > 0 && mgr->m_terrainObjectBuffer && mgr->m_terrainDrawArgsBuffer) {
                u32 zeroTerrain = 0;
                cmdList->writeBuffer(mgr->m_terrainVisibleCountBuffer, &zeroTerrain, sizeof(u32));
                CullParamsCB terrainCB;
                terrainCB.viewProj = Device.mFullTransform;
                terrainCB.prevViewProj = data.prevViewProj;
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
                cmdList->writeBuffer(mgr->m_cullParamsCB, &terrainCB, sizeof(terrainCB));

                nvrhi::BindingSetDesc terrainBindDesc;
                terrainBindDesc.bindings = {
                    nvrhi::BindingSetItem::ConstantBuffer(5, mgr->m_cullParamsCB),
                    nvrhi::BindingSetItem::StructuredBuffer_SRV(0, mgr->m_terrainObjectBuffer),
                    nvrhi::BindingSetItem::Texture_SRV(1, hizTexture),
                    nvrhi::BindingSetItem::Sampler(0, mgr->m_pointSampler),
                    nvrhi::BindingSetItem::StructuredBuffer_UAV(0, mgr->m_terrainVisibleIndexBuffer),
                    nvrhi::BindingSetItem::RawBuffer_UAV(1, mgr->m_terrainVisibleCountBuffer),
                    nvrhi::BindingSetItem::StructuredBuffer_UAV(2, mgr->m_terrainVisibilityBuffer)
                };

                nvrhi::BindingSetHandle terrainBindingSet = nvDevice->createBindingSet(terrainBindDesc, mgr->m_cullLayout);
                R_ASSERT2(terrainBindingSet, "Terrain culling binding set creation failed");

                nvrhi::ComputeState terrainState;
                terrainState.pipeline = mgr->m_cullPipeline;
                terrainState.bindings = { terrainBindingSet };
                cmdList->setComputeState(terrainState);

                u32 terrainGroupCount = (mgr->m_terrainObjectCount + CULL_THREAD_GROUP_SIZE - 1) / CULL_THREAD_GROUP_SIZE;
                cmdList->dispatch(terrainGroupCount, 1, 1);

                R_ASSERT2(mgr->m_compactEnabled, "Terrain compaction requires batch compaction to be enabled");
                R_ASSERT2(mgr->m_compactCountPipeline && mgr->m_compactScanPipeline && mgr->m_compactScatterPipeline,
                    "Terrain compaction pipelines not initialized");
                R_ASSERT2(mgr->m_terrainCompactDrawArgsBuffer && mgr->m_terrainCompactBatchIndicesBuffer &&
                              mgr->m_terrainCompactMaterialIDBuffer && mgr->m_terrainCompactCountBuffer &&
                              mgr->m_terrainCompactLocalPrefixBuffer && mgr->m_terrainCompactGroupCountsBuffer &&
                              mgr->m_terrainCompactGroupOffsetsBuffer,
                    "Terrain compaction buffers not initialized");
                R_ASSERT2(mgr->m_terrainMaterialIDBuffer, "Terrain material ID buffer missing");

                struct CompactParamsCB {
                    u32 batchCount;
                    u32 frameId;
                    u32 padding[2];
                };
                CompactParamsCB terrainCompactCB;
                terrainCompactCB.batchCount = mgr->m_terrainObjectCount;
                terrainCompactCB.frameId = frameId;
                terrainCompactCB.padding[0] = terrainCompactCB.padding[1] = 0;
                cmdList->writeBuffer(mgr->m_compactParamsCB, &terrainCompactCB, sizeof(terrainCompactCB));

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

                    nvrhi::BindingSetDesc terrainCountBindDesc;
                    terrainCountBindDesc.bindings = {
                        nvrhi::BindingSetItem::ConstantBuffer(5, mgr->m_compactParamsCB),
                        nvrhi::BindingSetItem::StructuredBuffer_SRV(0, mgr->m_terrainVisibilityBuffer),
                        nvrhi::BindingSetItem::StructuredBuffer_UAV(0, mgr->m_terrainCompactLocalPrefixBuffer),
                        nvrhi::BindingSetItem::StructuredBuffer_UAV(1, mgr->m_terrainCompactGroupCountsBuffer)
                    };

                    nvrhi::BindingSetHandle terrainCountBindingSet =
                        framegraph::GetPassResourceCache().GetOrCreateBindingSet(terrainCountBindDesc, mgr->m_compactCountLayout, nvDevice);
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

                    nvrhi::BindingSetDesc terrainScanBindDesc;
                    terrainScanBindDesc.bindings = {
                        nvrhi::BindingSetItem::ConstantBuffer(5, mgr->m_compactParamsCB),
                        nvrhi::BindingSetItem::StructuredBuffer_SRV(0, mgr->m_terrainCompactGroupCountsBuffer),
                        nvrhi::BindingSetItem::StructuredBuffer_UAV(0, mgr->m_terrainCompactGroupOffsetsBuffer),
                        nvrhi::BindingSetItem::RawBuffer_UAV(1, mgr->m_terrainCompactCountBuffer),
                        nvrhi::BindingSetItem::RawBuffer_UAV(2, mgr->m_terrainCompactDispatchArgsBuffer)
                    };

                    nvrhi::BindingSetHandle terrainScanBindingSet =
                        framegraph::GetPassResourceCache().GetOrCreateBindingSet(terrainScanBindDesc, mgr->m_compactScanLayout, nvDevice);
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

                    nvrhi::BindingSetDesc terrainScatterBindDesc;
                    terrainScatterBindDesc.bindings = {
                        nvrhi::BindingSetItem::ConstantBuffer(5, mgr->m_compactParamsCB),
                        nvrhi::BindingSetItem::RawBuffer_SRV(0, mgr->m_terrainDrawArgsBuffer),
                        nvrhi::BindingSetItem::StructuredBuffer_SRV(1, mgr->m_terrainMaterialIDBuffer),
                        nvrhi::BindingSetItem::StructuredBuffer_SRV(2, mgr->m_terrainVisibilityBuffer),
                        nvrhi::BindingSetItem::StructuredBuffer_SRV(3, mgr->m_terrainCompactLocalPrefixBuffer),
                        nvrhi::BindingSetItem::StructuredBuffer_SRV(4, mgr->m_terrainCompactGroupOffsetsBuffer),
                        nvrhi::BindingSetItem::RawBuffer_UAV(0, mgr->m_terrainCompactDrawArgsBuffer),
                        nvrhi::BindingSetItem::StructuredBuffer_UAV(1, mgr->m_terrainCompactBatchIndicesBuffer),
                        nvrhi::BindingSetItem::StructuredBuffer_UAV(2, mgr->m_terrainCompactMaterialIDBuffer)
                    };

                    nvrhi::BindingSetHandle terrainScatterBindingSet =
                        framegraph::GetPassResourceCache().GetOrCreateBindingSet(terrainScatterBindDesc, mgr->m_compactScatterLayout, nvDevice);
                    R_ASSERT2(terrainScatterBindingSet, "Terrain compaction scatter binding set creation failed");

                    nvrhi::ComputeState terrainScatterState;
                    terrainScatterState.pipeline = mgr->m_compactScatterPipeline;
                    terrainScatterState.bindings = { terrainScatterBindingSet };
                    cmdList->setComputeState(terrainScatterState);
                    cmdList->dispatch(terrainCompactGroupCount, 1, 1);
                }

                cmdList->setBufferState(mgr->m_terrainCompactDrawArgsBuffer, nvrhi::ResourceStates::IndirectArgument);
                cmdList->setBufferState(mgr->m_terrainCompactCountBuffer, nvrhi::ResourceStates::IndirectArgument);
            }

            dispatchCullSet(mgr->m_transparentSet,
                mgr->m_variantPartitionEnabled ? &mgr->m_transparentPartition : nullptr);

        }
    );

    output.visibleIndices = VirtualResourceHandle();
    output.visibleCount = VirtualResourceHandle();
    output.drawArgsBuffer = passData.staticDrawArgsBuffer;
    output.staticDrawArgsBuffer = passData.staticDrawArgsBuffer;
    output.dynamicDrawArgsBuffer = passData.dynamicDrawArgsBuffer;
    output.staticObjectCount = m_staticSet.objectCount;
    output.dynamicObjectCount = m_dynamicSet.objectCount;

    if (m_compactEnabled) {
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

    output.terrainObjectCount = m_terrainObjectCount;

    if (m_terrainObjectCount > 0) {
        R_ASSERT2(m_terrainDrawArgsBuffer, "Terrain draw args buffer not initialized");
        R_ASSERT2(m_compactEnabled, "Terrain compaction requires batch compaction to be enabled");
        R_ASSERT2(m_terrainCompactDrawArgsBuffer && m_terrainCompactBatchIndicesBuffer &&
                      m_terrainCompactMaterialIDBuffer && m_terrainCompactCountBuffer,
            "Terrain compaction buffers not initialized");

        ResourceDesc terrainArgsDesc;
        terrainArgsDesc.type = ResourceDesc::Type::Buffer;
        terrainArgsDesc.debugName = "GPUCull_TerrainDrawArgs";
        terrainArgsDesc.bufferSize = m_maxTerrainObjects * sizeof(IndirectDrawArgs);
        terrainArgsDesc.isUAV = true;
        terrainArgsDesc.isTransient = false;

        output.terrainDrawArgsBuffer = fg.ImportBuffer("gpu_cull_terrain_drawargs", m_terrainDrawArgsBuffer, terrainArgsDesc);

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

    if (!m_skinnedCullEnabled) {
        return;
    }

    struct SkinnedCullPassData {
        VirtualResourceHandle hizPyramid;
        VirtualResourceHandle visibilityBuffer;
        GPUCullingManager* manager;
        const GeometryCollector* geometry;
        Fmatrix prevViewProj;
        u32 hizWidth;
        u32 hizHeight;
        u32 hizMipLevels;
    };

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

        [&, hizWidth, hizHeight, hizMipLevels, geometry, prevViewProj, visBufferHandle](FrameGraph& builder, PassHandle passHandle, SkinnedCullPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);
            passBuilder.asyncCompute();

            data.manager = this;
            data.geometry = geometry;
            data.prevViewProj = prevViewProj;
            data.hizWidth = hizWidth;
            data.hizHeight = hizHeight;
            data.hizMipLevels = hizMipLevels;

            data.hizPyramid = passBuilder.read(hizPyramid, ResourceState::ShaderResource);

            data.visibilityBuffer = passBuilder.write(visBufferHandle, ResourceState::UnorderedAccess);
        },

        [](const SkinnedCullPassData& data,
           const FrameGraph& fg,
           ng::RenderContext* ctx) {

            GPUCullingManager* mgr = data.manager;
            if (!mgr->m_skinnedCullEnabled)
                return;

            mgr->UploadSkinnedObjects(ctx, data.geometry);

            if (mgr->m_skinnedObjectCount == 0)
                return;

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            nvrhi::IDevice* nvDevice = mgr->m_device->GetNVRHIDevice();

            u32 frameId = Device.dwFrame + 1u;
            if (frameId == 0)
                frameId = 1;
            mgr->m_skinnedFrameId = frameId;

            nvrhi::ITexture* hizTexture = fg.GetPhysicalTexture(data.hizPyramid);
            if (!hizTexture) {
                Msg("! [GPUCulling] Hi-Z texture not available for skinned culling");
                return;
            }

            cmdList->clearBufferUInt(mgr->m_skinnedVisibilityBuffer, 0);

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

            cmdList->writeBuffer(mgr->m_cullParamsCB, &cb, sizeof(cb));

            nvrhi::BindingSetDesc bindDesc;
            bindDesc.bindings = {
                nvrhi::BindingSetItem::ConstantBuffer(5, mgr->m_cullParamsCB),
                nvrhi::BindingSetItem::StructuredBuffer_SRV(0, mgr->m_skinnedObjectBuffer),
                nvrhi::BindingSetItem::Texture_SRV(1, hizTexture),
                nvrhi::BindingSetItem::Sampler(0, mgr->m_pointSampler),
                nvrhi::BindingSetItem::StructuredBuffer_UAV(0, mgr->m_staticSet.visibleIndexBuffer),
                nvrhi::BindingSetItem::RawBuffer_UAV(1, mgr->m_staticSet.visibleCountBuffer),
                nvrhi::BindingSetItem::StructuredBuffer_UAV(2, mgr->m_skinnedVisibilityBuffer)
            };

            nvrhi::BindingSetHandle bindingSet = nvDevice->createBindingSet(bindDesc, mgr->m_cullLayout);
            R_ASSERT2(bindingSet, "Failed to create skinned culling binding set");

            nvrhi::ComputeState state;
            state.pipeline = mgr->m_cullPipeline;
            state.bindings = { bindingSet };
            cmdList->setComputeState(state);

            u32 groupCount = (mgr->m_skinnedObjectCount + CULL_THREAD_GROUP_SIZE - 1) / CULL_THREAD_GROUP_SIZE;
            cmdList->dispatch(groupCount, 1, 1);

            mgr->ScheduleSkinnedVisibilityReadback(cmdList);
        }
    );
}

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
        desc.keepInitialState = true;

        m_debugBuffer = nvDevice->createBuffer(desc);
        if (!m_debugBuffer) {
            Msg("! [GPUCulling] Failed to create debug buffer");
            return;
        }
    }

    {
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

    {
        nvrhi::BindingLayoutDesc layoutDesc;
        layoutDesc.visibility = nvrhi::ShaderType::Compute;
        layoutDesc.bindings = {
            nvrhi::BindingLayoutItem::VolatileConstantBuffer(5),
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

    {
        nvrhi::BindingLayoutDesc layoutDesc;
        layoutDesc.visibility = nvrhi::ShaderType::Vertex | nvrhi::ShaderType::Pixel;
        layoutDesc.bindings = {
            nvrhi::BindingLayoutItem::VolatileConstantBuffer(5),
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(0)
        };

        m_debugGraphicsLayout = nvDevice->createBindingLayout(layoutDesc);
        if (!m_debugGraphicsLayout) {
            Msg("! [GPUCulling] Failed to create debug graphics binding layout");
            return;
        }

        nvrhi::GraphicsPipelineDesc pipeDesc;
        pipeDesc.VS = s_cull_debug_vs->nvrhiShader;
        pipeDesc.PS = s_cull_debug_ps->nvrhiShader;
        pipeDesc.bindingLayouts = { m_debugGraphicsLayout };
        pipeDesc.primType = nvrhi::PrimitiveType::TriangleStrip;
        pipeDesc.inputLayout = nullptr;

        pipeDesc.renderState.blendState.targets[0].setBlendEnable(true);
        pipeDesc.renderState.blendState.targets[0].setSrcBlend(nvrhi::BlendFactor::SrcAlpha);
        pipeDesc.renderState.blendState.targets[0].setDestBlend(nvrhi::BlendFactor::InvSrcAlpha);
        pipeDesc.renderState.blendState.targets[0].setBlendOp(nvrhi::BlendOp::Add);
        pipeDesc.renderState.blendState.targets[0].setSrcBlendAlpha(nvrhi::BlendFactor::One);
        pipeDesc.renderState.blendState.targets[0].setDestBlendAlpha(nvrhi::BlendFactor::Zero);
        pipeDesc.renderState.blendState.targets[0].setBlendOpAlpha(nvrhi::BlendOp::Add);

        pipeDesc.renderState.depthStencilState.setDepthTestEnable(false);
        pipeDesc.renderState.depthStencilState.setDepthWriteEnable(false);

        pipeDesc.renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);

        nvrhi::FramebufferInfoEx framebufferInfo;
        framebufferInfo.addColorFormat(nvrhi::Format::RGBA16_FLOAT);
        framebufferInfo.setDepthFormat(nvrhi::Format::D32);

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

        [](const DebugPassData& data,
           const FrameGraph& fg,
           ng::RenderContext* ctx) {

            GPUCullingManager* mgr = data.manager;
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            nvrhi::IDevice* nvDevice = mgr->m_device->GetNVRHIDevice();

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
                vsCB.view = Device.mView;
                vsCB.viewProj = Device.mFullTransform;
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
        desc.keepInitialState = true;

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
            nvrhi::BindingLayoutItem::VolatileConstantBuffer(5),
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

    m_megaVertices.clear();
    m_megaVertices.reserve(estimatedVertices);
    m_megaIndices.clear();
    m_megaIndices.reserve(estimatedIndices);

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

    alloc.vertexOffset = m_totalVertexCount;
    alloc.indexOffset = m_totalIndexCount;
    alloc.vertexCount = vertexCount;
    alloc.indexCount = indexCount;

    u32 prevSize = static_cast<u32>(m_megaVertices.size());
    m_megaVertices.resize(prevSize + vertexCount);

    u32 converted = bindless::VertexConverter::ConvertVertices(
        vertices, vertexStride, vertexCount, format,
        &m_megaVertices[prevSize]
    );

    if (converted != vertexCount) {
        Msg("! [GPUCulling] RegisterMesh: Vertex conversion failed (got %u, expected %u)", converted, vertexCount);
        m_megaVertices.resize(prevSize);
        return alloc;
    }

    u32 prevIndexSize = static_cast<u32>(m_megaIndices.size());
    m_megaIndices.resize(prevIndexSize + indexCount);

    for (u32 i = 0; i < indexCount; i++) {
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

    if (m_totalVertexCount > 0) {
        nvrhi::BufferDesc desc;
        desc.debugName = "MegaVertexBuffer";
        desc.byteSize = m_totalVertexCount * sizeof(bindless::UnifiedVertex);
        desc.isVertexBuffer = true;
        desc.initialState = nvrhi::ResourceStates::VertexBuffer;
        desc.keepInitialState = true;
        desc.canHaveRawViews = true;
        desc.isAccelStructBuildInput = true;

        m_megaVertexBuffer = nvDevice->createBuffer(desc);
        if (!m_megaVertexBuffer) {
            Msg("! [GPUCulling] Failed to create mega vertex buffer (%u vertices, %zu bytes)",
                m_totalVertexCount, desc.byteSize);
            return;
        }

        m_maxMegaVertices = m_totalVertexCount;
    }

    if (m_totalIndexCount > 0) {
        nvrhi::BufferDesc desc;
        desc.debugName = "MegaIndexBuffer";
        desc.byteSize = m_totalIndexCount * sizeof(u32);
        desc.initialState = nvrhi::ResourceStates::IndexBuffer;
        desc.keepInitialState = true;
        desc.isIndexBuffer = true;
        desc.canHaveRawViews = true;
        desc.isAccelStructBuildInput = true;

        m_megaIndexBuffer = nvDevice->createBuffer(desc);
        if (!m_megaIndexBuffer) {
            Msg("! [GPUCulling] Failed to create mega index buffer (%u indices, %zu bytes)",
                m_totalIndexCount, desc.byteSize);
            return;
        }

        m_maxMegaIndices = m_totalIndexCount;
    }

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

    CreateMegaBuffers();

    if (!m_megaVertexBuffer || !m_megaIndexBuffer) {
        Msg("! [GPUCulling] EndLevelLoad: Failed to create mega-buffers");
        m_levelLoadInProgress = false;
        return;
    }

    m_megaBuffersReady = true;
    m_levelLoadInProgress = false;

    Msg("* [GPUCulling] Mega-buffers ready for GPU upload");
}

void GPUCullingManager::UploadInstanceData(ng::RenderContext* ctx, const GeometryCollector* geometry)
{
    (void)ctx;
    (void)geometry;
}

bindless::SourceVertexFormat GPUCullingManager::DetectFormatFromDecl(
    const D3DVERTEXELEMENT9* decl,
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

    if (stride == 12) {
        return bindless::SourceVertexFormat::X_Vert;
    }
    else if (stride == 28) {
        if (hasColor) return bindless::SourceVertexFormat::R1_Vert_Unpacked;
        return bindless::SourceVertexFormat::MU_Model_Unpacked;
    }
    else if (stride == 32) {
        if (hasFloat2TexCoord && hasTexCoord1) {
            return bindless::SourceVertexFormat::R1_Lmap_Unpacked;
        }
        if (hasTexCoord1) {
            return bindless::SourceVertexFormat::R1_Lmap;
        }
        if (hasColor) {
            return bindless::SourceVertexFormat::R1_Vert;
        }
        if (hasShort4TexCoord) {
            return bindless::SourceVertexFormat::MU_Model;
        }
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

    bindless::SourceVertexFormat format = DetectFormatFromDecl(decl, vertexStride);
    if (format == bindless::SourceVertexFormat::Unknown) {
        Msg("! [GPUCulling] RegisterVBPool: Unknown vertex format (stride=%u) - registering placeholder", vertexStride);

        VBPoolInfo placeholder;
        placeholder.megaBufferVertexOffset = 0;
        placeholder.vertexCount = 0;
        placeholder.format = bindless::SourceVertexFormat::Unknown;

        xr_vector<VBPoolInfo>& pools = alternative ? m_vbPoolsAlt : m_vbPools;
        u32 poolID = static_cast<u32>(pools.size());
        pools.push_back(placeholder);

        return poolID;
    }

    VBPoolInfo poolInfo;
    poolInfo.megaBufferVertexOffset = m_totalVertexCount;
    poolInfo.vertexCount = vertexCount;
    poolInfo.format = format;

    u32 prevSize = static_cast<u32>(m_megaVertices.size());
    m_megaVertices.resize(prevSize + vertexCount);

    u32 converted = bindless::VertexConverter::ConvertVertices(
        vertices, vertexStride, vertexCount, format,
        &m_megaVertices[prevSize]
    );

    if (converted != vertexCount) {
        Msg("! [GPUCulling] RegisterVBPool: Vertex conversion failed (got %u, expected %u)", converted, vertexCount);
        m_megaVertices.resize(prevSize);
        return UINT32_MAX;
    }

    m_totalVertexCount += vertexCount;

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
        Msg("! [GPUCulling] RegisterIBPool: Invalid parameters - registering placeholder");

        IBPoolInfo placeholder;
        placeholder.megaBufferIndexOffset = 0;
        placeholder.indexCount = 0;

        xr_vector<IBPoolInfo>& pools = alternative ? m_ibPoolsAlt : m_ibPools;
        u32 poolID = static_cast<u32>(pools.size());
        pools.push_back(placeholder);

        return poolID;
    }

    IBPoolInfo poolInfo;
    poolInfo.megaBufferIndexOffset = m_totalIndexCount;
    poolInfo.indexCount = indexCount;

    u32 prevSize = static_cast<u32>(m_megaIndices.size());
    m_megaIndices.resize(prevSize + indexCount);

    for (u32 i = 0; i < indexCount; i++) {
        m_megaIndices[prevSize + i] = static_cast<u32>(indices[i]);
    }

    m_totalIndexCount += indexCount;

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
        return alloc;
    }

    const VBPoolInfo& vbPool = vbPools[vbID];
    const IBPoolInfo& ibPool = ibPools[ibID];

    if (vbPool.vertexCount == 0 || ibPool.indexCount == 0) {
        return alloc;
    }

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

    alloc.vertexOffset = vbPool.megaBufferVertexOffset + vBase;
    alloc.indexOffset = ibPool.megaBufferIndexOffset + iBase;
    alloc.vertexCount = vCount;
    alloc.indexCount = iCount;
    alloc.valid = true;

    return alloc;
}

}
