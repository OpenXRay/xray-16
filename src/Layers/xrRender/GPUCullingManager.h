#pragma once

#include "xrCore/xrCore.h"
#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/Bindless/UnifiedVertex.h"
#include "Layers/xrRender/ShaderVariant/VariantPartitionConfig.h"

namespace xray::render::RENDER_NAMESPACE::passes {
    struct ParticleBatch;
}

namespace xray::render {
    class GeometryCollector;
    struct GeometryBatch;
    class dxRender_Visual;
    namespace ng {
        class RenderDevice;
        class RenderContext;
    }
    namespace framegraph {
        class FrameGraph;
    }
}

namespace xray::render::RENDER_NAMESPACE {
    class CKinematics;
}

namespace xray::render::RENDER_NAMESPACE {

struct GPUObjectData {
    Fvector position;
    float radius;
    u32 batchIndex;
    u32 flags;
    float pad0, pad1;
};
static_assert(sizeof(GPUObjectData) == 32, "GPUObjectData must be 32 bytes for GPU alignment");

struct GPUParticleData {
    Fvector position;
    float radius;
    u32 batchIndex;
    u32 flags;
    float pad0, pad1;
};
static_assert(sizeof(GPUParticleData) == 32, "GPUParticleData must be 32 bytes for GPU alignment");

struct Float3x4 {
    float m[3][4];
};
static_assert(sizeof(Float3x4) == 48, "Float3x4 must be 48 bytes");

enum GPUObjectFlags : u32 {
    GPU_OBJECT_OPAQUE      = 0x1,
    GPU_OBJECT_ALPHA_TEST  = 0x2,
    GPU_OBJECT_TRANSPARENT = 0x4,
};

enum CullState : u32 {
    CULL_STATE_VISIBLE           = 0,
    CULL_STATE_OCCLUDER          = 1,
    CULL_STATE_CULLED_DISTANCE   = 2,
    CULL_STATE_CULLED_FRUSTUM    = 3,
    CULL_STATE_CULLED_OCCLUSION  = 4,
    CULL_STATE_PARTICLE_VISIBLE  = 5,
    CULL_STATE_PARTICLE_CULLED   = 6,
};

struct CullDebugData {
    Fvector position;
    float radius;
    u32 cullState;
    float objectDepth;
    float hiZDepth;
    u32 objectIndex;
};
static_assert(sizeof(CullDebugData) == 32, "CullDebugData must be 32 bytes for GPU alignment");

struct IndirectDrawArgs {
    u32 indexCountPerInstance;
    u32 instanceCount;
    u32 startIndexLocation;
    s32 baseVertexLocation;
    u32 startInstanceLocation;
};
static_assert(sizeof(IndirectDrawArgs) == 20, "IndirectDrawArgs must be 20 bytes");

struct MeshAllocation {
    u32 vertexOffset;
    u32 indexOffset;
    u32 vertexCount;
    u32 indexCount;
    bool valid;

    MeshAllocation() : vertexOffset(0), indexOffset(0), vertexCount(0), indexCount(0), valid(false) {}
};

struct GPUInstanceData {
    Fmatrix world;
    u32 materialID;
    u32 flags;
    float pad0, pad1;
};
static_assert(sizeof(GPUInstanceData) == 80, "GPUInstanceData must be 80 bytes for GPU alignment");

struct GPUCullOutput {
    framegraph::VirtualResourceHandle visibleIndices;
    framegraph::VirtualResourceHandle visibleCount;
    framegraph::VirtualResourceHandle drawArgsBuffer;
    framegraph::VirtualResourceHandle staticDrawArgsBuffer;
    framegraph::VirtualResourceHandle dynamicDrawArgsBuffer;
    framegraph::VirtualResourceHandle staticCompactDrawArgs;
    framegraph::VirtualResourceHandle staticCompactBatchIndices;
    framegraph::VirtualResourceHandle dynamicCompactDrawArgs;
    framegraph::VirtualResourceHandle dynamicCompactBatchIndices;
    u32 maxObjects;
    u32 staticObjectCount;
    u32 dynamicObjectCount;

    framegraph::VirtualResourceHandle terrainDrawArgsBuffer;
    framegraph::VirtualResourceHandle terrainCompactDrawArgs;
    framegraph::VirtualResourceHandle terrainCompactBatchIndices;
    framegraph::VirtualResourceHandle terrainCompactMaterialIDs;
    framegraph::VirtualResourceHandle terrainCompactCount;
    u32 terrainObjectCount;

    framegraph::VirtualResourceHandle transparentCompactDrawArgs;
    framegraph::VirtualResourceHandle transparentCompactBatchIndices;
    framegraph::VirtualResourceHandle transparentCompactMaterialIDs;
    framegraph::VirtualResourceHandle transparentCompactCount;
    u32 transparentObjectCount;

};

struct GPUParticleCullOutput {
    framegraph::VirtualResourceHandle drawArgsBuffer;
    u32 maxParticles;
};

class GPUCullingManager {
public:
    GPUCullingManager();
    ~GPUCullingManager();

    void Initialize(ng::RenderDevice* device);

    void Shutdown();

    void UploadSceneObjects(ng::RenderContext* ctx, const GeometryCollector* geometry);

    void InvalidateStaticCullingData();

    GPUCullOutput SetupCullingPass(
        framegraph::FrameGraph& fg,
        framegraph::VirtualResourceHandle hizPyramid,
        u32 hizWidth,
        u32 hizHeight,
        u32 hizMipLevels,
        const GeometryCollector* geometry,
        const Fmatrix& prevViewProj
    );

    u32 GetObjectCount() const { return m_objectCount; }
    u32 GetStaticObjectCount() const { return m_staticSet.objectCount; }
    u32 GetDynamicObjectCount() const { return m_dynamicSet.objectCount; }

    bool IsEnabled() const { return m_initialized && m_computeEnabled; }

    void BeginLevelLoad(u32 estimatedVertices = 1000000, u32 estimatedIndices = 3000000);

    MeshAllocation RegisterMesh(
        const void* vertices,
        u32 vertexCount,
        u32 vertexStride,
        bindless::SourceVertexFormat format,
        const u16* indices,
        u32 indexCount
    );

    void EndLevelLoad();

    bool AreMegaBuffersReady() const { return m_megaBuffersReady; }

    nvrhi::IBuffer* GetMegaVertexBuffer() const { return m_megaVertexBuffer.Get(); }
    nvrhi::IBuffer* GetMegaIndexBuffer() const { return m_megaIndexBuffer.Get(); }
    bool IsCompactionEnabled() const { return m_compactEnabled; }

    void UploadInstanceData(ng::RenderContext* ctx, const GeometryCollector* geometry);

    u32 GetTotalVertexCount() const { return m_totalVertexCount; }
    u32 GetTotalIndexCount() const { return m_totalIndexCount; }

    void SetupDebugVisualizationPass(
        framegraph::FrameGraph& fg,
        framegraph::VirtualResourceHandle hizPyramid,
        framegraph::VirtualResourceHandle colorTarget,
        framegraph::VirtualResourceHandle depthTarget,
        u32 hizWidth,
        u32 hizHeight,
        u32 hizMipLevels,
        const xr_vector<passes::ParticleBatch>* particleBatches = nullptr
    );

    bool IsDebugEnabled() const;

    void UploadParticleBatches(ng::RenderContext* ctx, const xr_vector<passes::ParticleBatch>* batches);

    GPUParticleCullOutput SetupParticleCullingPass(
        framegraph::FrameGraph& fg,
        framegraph::VirtualResourceHandle hizPyramid,
        u32 hizWidth,
        u32 hizHeight,
        u32 hizMipLevels,
        const xr_vector<passes::ParticleBatch>* batches
    );

    u32 GetParticleCount() const { return m_particleCount; }
    bool IsParticleCullingEnabled() const { return m_initialized && m_particleCullEnabled; }
    nvrhi::IBuffer* GetParticleDrawArgsBuffer() const { return m_particleDrawArgsBuffer.Get(); }

    nvrhi::IBuffer* GetStaticCompactDrawArgsBuffer() const { return m_staticSet.compactDrawArgsBuffer.Get(); }
    nvrhi::IBuffer* GetStaticCompactBatchIndicesBuffer() const { return m_staticSet.compactBatchIndicesBuffer.Get(); }
    nvrhi::IBuffer* GetStaticCompactMaterialIDBuffer() const { return m_staticSet.compactMaterialIDBuffer.Get(); }
    nvrhi::IBuffer* GetStaticCompactCountBuffer() const { return m_staticSet.compactCountBuffer.Get(); }
    nvrhi::IBuffer* GetStaticInstanceBuffer() const { return m_staticSet.instanceBuffer.Get(); }

    nvrhi::IBuffer* GetDynamicCompactDrawArgsBuffer() const { return m_dynamicSet.compactDrawArgsBuffer.Get(); }
    nvrhi::IBuffer* GetDynamicCompactBatchIndicesBuffer() const { return m_dynamicSet.compactBatchIndicesBuffer.Get(); }
    nvrhi::IBuffer* GetDynamicCompactMaterialIDBuffer() const { return m_dynamicSet.compactMaterialIDBuffer.Get(); }
    nvrhi::IBuffer* GetDynamicCompactCountBuffer() const { return m_dynamicSet.compactCountBuffer.Get(); }
    nvrhi::IBuffer* GetDynamicInstanceBuffer() const { return m_dynamicSet.instanceBuffer.Get(); }

    nvrhi::IBuffer* GetStaticDrawArgsBuffer() const { return m_staticSet.drawArgsBuffer.Get(); }
    nvrhi::IBuffer* GetDynamicDrawArgsBuffer() const { return m_dynamicSet.drawArgsBuffer.Get(); }

    u32 GetTerrainObjectCount() const { return m_terrainObjectCount; }
    nvrhi::IBuffer* GetTerrainDrawArgsBuffer() const { return m_terrainDrawArgsBuffer.Get(); }
    nvrhi::IBuffer* GetTerrainMaterialIDBuffer() const { return m_terrainMaterialIDBuffer.Get(); }

    u32 GetTransparentObjectCount() const { return m_transparentSet.objectCount; }
    nvrhi::IBuffer* GetTransparentInstanceBuffer() const { return m_transparentSet.instanceBuffer.Get(); }
    nvrhi::IBuffer* GetTransparentCompactDrawArgsBuffer() const { return m_transparentSet.compactDrawArgsBuffer.Get(); }
    nvrhi::IBuffer* GetTransparentCompactBatchIndicesBuffer() const { return m_transparentSet.compactBatchIndicesBuffer.Get(); }
    nvrhi::IBuffer* GetTransparentCompactMaterialIDBuffer() const { return m_transparentSet.compactMaterialIDBuffer.Get(); }
    nvrhi::IBuffer* GetTransparentCompactCountBuffer() const { return m_transparentSet.compactCountBuffer.Get(); }

    static constexpr u32 MAX_SHADER_VARIANTS = 32;

    struct VariantPartitionBuffers {
        nvrhi::BufferHandle variantCountBuffer;
        nvrhi::BufferHandle reorderedDrawArgsBuffer;
        nvrhi::BufferHandle reorderedBatchIndicesBuffer;
        nvrhi::BufferHandle reorderedMaterialIDsBuffer;
        nvrhi::BufferHandle drawIndexBuffer;
        u32 binCapacity = 0;
        u32 variantCount = 0;

        VariantPartitionConfig ToConfig() const {
            return {
                variantCountBuffer.Get(), reorderedDrawArgsBuffer.Get(),
                reorderedBatchIndicesBuffer.Get(), reorderedMaterialIDsBuffer.Get(),
                drawIndexBuffer.Get(), binCapacity, variantCount
            };
        }
    };

    bool IsVariantPartitionEnabled() const { return m_variantPartitionEnabled; }
    const VariantPartitionBuffers& GetStaticPartition() const { return m_staticPartition; }
    const VariantPartitionBuffers& GetTransparentPartition() const { return m_transparentPartition; }

    struct CullingStats {
        u32 staticVisible = 0;
        u32 dynamicVisible = 0;
        u32 terrainVisible = 0;
        u32 transparentVisible = 0;
        u32 totalVisible() const { return staticVisible + dynamicVisible + terrainVisible + transparentVisible; }
    };
    const CullingStats& GetCullingStats() const { return m_cullingStats; }

    void ScheduleStatsReadback(nvrhi::ICommandList* cmdList);

    void UploadSkinnedObjects(ng::RenderContext* ctx, const GeometryCollector* geometry);

    void SetupSkinnedCullingPass(
        framegraph::FrameGraph& fg,
        framegraph::VirtualResourceHandle hizPyramid,
        u32 hizWidth,
        u32 hizHeight,
        u32 hizMipLevels,
        const GeometryCollector* geometry,
        const Fmatrix& prevViewProj
    );

    u32 GetSkinnedVisibilityByVisual(const dxRender_Visual* visual) const;

    bool HasSkinnedVisibilityData() const { return !m_skinnedVisibilityByVisual.empty(); }

    u32 GetSkinnedObjectCount() const { return m_skinnedObjectCount; }
    bool IsSkinnedCullingEnabled() const { return m_initialized && m_skinnedCullEnabled; }


    struct SkinnedCullingStats {
        u32 submitted = 0;
        u32 visible = 0;
        u32 culled = 0;
    };
    const SkinnedCullingStats& GetSkinnedCullingStats() const { return m_skinnedCullingStats; }
    void UpdateSkinnedCullingStats(u32 rendered, u32 culled);

    void ScheduleSkinnedVisibilityReadback(nvrhi::ICommandList* cmdList);

    void ProcessSkinnedVisibilityReadback();

    void BeginSkinnedFrame();

    u32 GetOrUploadSkeleton(nvrhi::ICommandList* cmdList, CKinematics* skeleton);

    nvrhi::IBuffer* GetGlobalBoneBuffer() const { return m_globalBoneBuffer.Get(); }

    void ClearSkinnedVisibilityData();

    void ProcessStatsReadback();
    nvrhi::IBuffer* GetTerrainInstanceBuffer() const { return m_terrainInstanceBuffer.Get(); }
    nvrhi::IBuffer* GetTerrainBatchIndicesBuffer() const { return m_terrainBatchIndicesBuffer.Get(); }
    nvrhi::IBuffer* GetTerrainCompactDrawArgsBuffer() const { return m_terrainCompactDrawArgsBuffer.Get(); }
    nvrhi::IBuffer* GetTerrainCompactBatchIndicesBuffer() const { return m_terrainCompactBatchIndicesBuffer.Get(); }
    nvrhi::IBuffer* GetTerrainCompactCountBuffer() const { return m_terrainCompactCountBuffer.Get(); }
    nvrhi::IBuffer* GetTerrainCompactMaterialIDBuffer() const { return m_terrainCompactMaterialIDBuffer.Get(); }

private:
    void CreateBuffers(ng::RenderDevice* device);
    void CreateComputePipeline(ng::RenderDevice* device);
    void CreateCompactionResources(ng::RenderDevice* device);
    void CreateDebugResources(ng::RenderDevice* device);
    void CreateParticleResources(ng::RenderDevice* device);
    void CreateMegaBuffers();

    void ExtractFrustumPlanes(Fmatrix& viewProj, Fvector4* outPlanes);

    struct CullSetBuffers {
        nvrhi::BufferHandle objectBuffer;
        nvrhi::BufferHandle visibleIndexBuffer;
        nvrhi::BufferHandle visibleCountBuffer;
        nvrhi::BufferHandle drawArgsBuffer;
        nvrhi::BufferHandle materialIDBuffer;
        nvrhi::BufferHandle visibilityBuffer;
        nvrhi::BufferHandle compactDrawArgsBuffer;
        nvrhi::BufferHandle compactBatchIndicesBuffer;
        nvrhi::BufferHandle compactMaterialIDBuffer;
        nvrhi::BufferHandle compactCountBuffer;
        nvrhi::BufferHandle compactDispatchArgsBuffer;
        nvrhi::BufferHandle compactLocalPrefixBuffer;
        nvrhi::BufferHandle compactGroupCountsBuffer;
        nvrhi::BufferHandle compactGroupOffsetsBuffer;
        nvrhi::BufferHandle instanceBuffer;
        u32 objectCount = 0;
        u32 maxObjects = 0;
        bool drawArgsUploaded = false;
        bool objectsUploaded = false;
    };

    CullSetBuffers m_staticSet;
    CullSetBuffers m_dynamicSet;

    nvrhi::BufferHandle m_cullParamsCB;

    nvrhi::ComputePipelineHandle m_cullPipeline;
    nvrhi::ComputePipelineHandle m_clearArgsPipeline;
    nvrhi::ComputePipelineHandle m_compactCountPipeline;
    nvrhi::ComputePipelineHandle m_compactScanPipeline;
    nvrhi::ComputePipelineHandle m_compactScatterPipeline;
    nvrhi::BindingLayoutHandle m_cullLayout;
    nvrhi::BindingLayoutHandle m_clearArgsLayout;
    nvrhi::BindingLayoutHandle m_compactCountLayout;
    nvrhi::BindingLayoutHandle m_compactScanLayout;
    nvrhi::BindingLayoutHandle m_compactScatterLayout;
    nvrhi::SamplerHandle m_pointSampler;

    nvrhi::BufferHandle m_compactParamsCB;

    nvrhi::ComputePipelineHandle m_variantPartitionPipeline;
    nvrhi::BindingLayoutHandle m_variantPartitionLayout;
    nvrhi::BufferHandle m_variantPartitionParamsCB;
    VariantPartitionBuffers m_staticPartition;
    VariantPartitionBuffers m_transparentPartition;
    bool m_variantPartitionEnabled = false;

    void CreateVariantPartitionResources(ng::RenderDevice* device);
    void InitPartitionBuffers(nvrhi::IDevice* nvDevice, VariantPartitionBuffers& part,
        const char* prefix, u32 variantCount, u32 maxObjects);
    void DispatchVariantPartition(
        nvrhi::ICommandList* cmdList,
        nvrhi::IDevice* nvDevice,
        const CullSetBuffers& set,
        VariantPartitionBuffers& partition);

    bool m_staticTerrainDrawArgsUploaded = false;

    bool m_compactEnabled = false;

    bool m_staticDataCached = false;

    nvrhi::BufferHandle m_terrainObjectBuffer;
    nvrhi::BufferHandle m_terrainDrawArgsBuffer;
    nvrhi::BufferHandle m_terrainVisibleIndexBuffer;
    nvrhi::BufferHandle m_terrainVisibleCountBuffer;
    nvrhi::BufferHandle m_terrainVisibilityBuffer;
    nvrhi::BufferHandle m_terrainInstanceBuffer;
    nvrhi::BufferHandle m_terrainBatchIndicesBuffer;
    nvrhi::BufferHandle m_terrainCompactDrawArgsBuffer;
    nvrhi::BufferHandle m_terrainCompactBatchIndicesBuffer;
    nvrhi::BufferHandle m_terrainCompactCountBuffer;
    nvrhi::BufferHandle m_terrainCompactDispatchArgsBuffer;
    nvrhi::BufferHandle m_terrainCompactMaterialIDBuffer;
    nvrhi::BufferHandle m_terrainCompactLocalPrefixBuffer;
    nvrhi::BufferHandle m_terrainCompactGroupCountsBuffer;
    nvrhi::BufferHandle m_terrainCompactGroupOffsetsBuffer;
    nvrhi::BufferHandle m_terrainMaterialIDBuffer;

    nvrhi::ComputePipelineHandle m_terrainApplyVisibilityPipeline;
    nvrhi::BindingLayoutHandle m_terrainApplyVisibilityLayout;

    CullSetBuffers m_transparentSet;

    nvrhi::BufferHandle m_debugBuffer;
    nvrhi::BufferHandle m_debugComputeParamsCB;
    nvrhi::BufferHandle m_debugGraphicsParamsCB;

    nvrhi::ComputePipelineHandle m_debugComputePipeline;
    nvrhi::ComputePipelineHandle m_particleDebugComputePipeline;
    nvrhi::BindingLayoutHandle m_debugComputeLayout;

    nvrhi::GraphicsPipelineHandle m_debugGraphicsPipeline;
    nvrhi::BindingLayoutHandle m_debugGraphicsLayout;
    nvrhi::InputLayoutHandle m_debugInputLayout;

    nvrhi::BufferHandle m_particleBuffer;
    nvrhi::BufferHandle m_particleDrawArgsBuffer;
    nvrhi::BufferHandle m_particleVisibleCountBuffer;
    nvrhi::BufferHandle m_particleCullParamsCB;
    nvrhi::ComputePipelineHandle m_particleCullPipeline;
    nvrhi::BindingLayoutHandle m_particleCullLayout;

    u32 m_particleCount = 0;
    u32 m_maxParticles = 0;
    bool m_particleCullEnabled = false;
    xr_vector<GPUParticleData> m_particleData;
    xr_vector<IndirectDrawArgs> m_particleDrawArgsData;
    ng::RenderDevice* m_device = nullptr;
    u32 m_objectCount = 0;
    u32 m_maxObjects = 0;
    bool m_initialized = false;
    bool m_computeEnabled = false;

    xr_vector<GPUObjectData> m_staticObjectData;
    xr_vector<IndirectDrawArgs> m_staticDrawArgsData;
    xr_vector<u32> m_staticMaterialIDData;
    xr_vector<GPUInstanceData> m_staticInstanceData;

    xr_vector<GPUObjectData> m_dynamicObjectData;
    xr_vector<IndirectDrawArgs> m_dynamicDrawArgsData;
    xr_vector<u32> m_dynamicMaterialIDData;
    xr_vector<GPUInstanceData> m_dynamicInstanceData;

    u32 m_terrainObjectCount = 0;
    u32 m_maxTerrainObjects = 0;
    xr_vector<GPUObjectData> m_terrainObjectData;
    xr_vector<IndirectDrawArgs> m_terrainDrawArgsData;
    xr_vector<u32> m_terrainMaterialIDData;
    xr_vector<GPUInstanceData> m_terrainInstanceData;

    xr_vector<GPUObjectData> m_transparentObjectData;
    xr_vector<IndirectDrawArgs> m_transparentDrawArgsData;
    xr_vector<u32> m_transparentMaterialIDData;
    xr_vector<GPUInstanceData> m_transparentInstanceData;

    nvrhi::BufferHandle m_skinnedObjectBuffer;
    nvrhi::BufferHandle m_skinnedVisibilityBuffer;

    static constexpr u32 SKINNED_READBACK_FRAMES = 2;
    nvrhi::BufferHandle m_skinnedReadbackBuffers[SKINNED_READBACK_FRAMES];
    xr_unordered_map<const dxRender_Visual*, u32> m_skinnedVisualToIndex[SKINNED_READBACK_FRAMES];
    u32 m_skinnedReadbackWriteIndex = 0;
    u32 m_skinnedReadbackFrameCount = 0;

    xr_unordered_map<const dxRender_Visual*, u32> m_skinnedVisibilityByVisual;

    u32 m_skinnedObjectCount = 0;
    u32 m_maxSkinnedObjects = 0;
    bool m_skinnedCullEnabled = false;

    xr_vector<GPUObjectData> m_skinnedObjectData;
    xr_vector<const GeometryBatch*> m_skinnedBatchPointers;
    u32 m_skinnedFrameId = 0;
    SkinnedCullingStats m_skinnedCullingStats;

    static constexpr u32 MAX_TOTAL_BONES = 8192;
    static constexpr u32 BONE_STRIDE = sizeof(Float3x4);
    nvrhi::BufferHandle m_globalBoneBuffer;
    xr_map<CKinematics*, u32> m_skeletonOffsets;
    xr_vector<Float3x4> m_boneStagingBuffer;
    u32 m_currentBoneOffset = 0;
    bool m_boneBufferInitialized = false;

    void CreateSkinnedCullingBuffers(ng::RenderDevice* device);
    void EnsureSkinnedBufferCapacity(u32 count);
    void UploadSkeletonBones(nvrhi::ICommandList* cmdList, CKinematics* skeleton, u32 boneOffset);

    nvrhi::BufferHandle m_megaVertexBuffer;
    nvrhi::BufferHandle m_megaIndexBuffer;

    xr_vector<bindless::UnifiedVertex> m_megaVertices;
    xr_vector<u32> m_megaIndices;

    u32 m_totalVertexCount = 0;
    u32 m_totalIndexCount = 0;
    u32 m_maxMegaVertices = 0;
    u32 m_maxMegaIndices = 0;
    bool m_megaBuffersReady = false;
    bool m_levelLoadInProgress = false;

    struct VBPoolInfo {
        u32 megaBufferVertexOffset;
        u32 vertexCount;
        bindless::SourceVertexFormat format;
    };

    struct IBPoolInfo {
        u32 megaBufferIndexOffset;
        u32 indexCount;
    };

    xr_vector<VBPoolInfo> m_vbPools;
    xr_vector<IBPoolInfo> m_ibPools;
    xr_vector<VBPoolInfo> m_vbPoolsAlt;
    xr_vector<IBPoolInfo> m_ibPoolsAlt;

    nvrhi::BufferHandle m_statsReadbackBuffer;
    CullingStats m_cullingStats;
    bool m_statsReadbackPending = false;

public:

    u32 RegisterVBPool(
        const void* vertices,
        u32 vertexCount,
        u32 vertexStride,
        const D3DVERTEXELEMENT9* decl,
        bool alternative = false
    );

    u32 RegisterIBPool(
        const u16* indices,
        u32 indexCount,
        bool alternative = false
    );

    MeshAllocation GetMeshAllocation(
        u32 vbID, u32 vBase, u32 vCount,
        u32 ibID, u32 iBase, u32 iCount,
        bool alternative = false
    ) const;

    static bindless::SourceVertexFormat DetectFormatFromDecl(
        const D3DVERTEXELEMENT9* decl,
        u32 stride
    );
};

}
