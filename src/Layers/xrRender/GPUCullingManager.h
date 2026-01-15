#pragma once

#include "xrCore/xrCore.h"
#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/Bindless/UnifiedVertex.h"

namespace xray::render::RENDER_NAMESPACE::passes {
    struct ParticleBatch;
}

namespace xray::render {
    class GeometryCollector;
    struct GeometryBatch;
    namespace ng {
        class RenderDevice;
        class RenderContext;
    }
    namespace framegraph {
        class FrameGraph;
    }
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
    nvrhi::BufferHandle m_terrainCompactMaterialIDBuffer;
    nvrhi::BufferHandle m_terrainCompactLocalPrefixBuffer;
    nvrhi::BufferHandle m_terrainCompactGroupCountsBuffer;
    nvrhi::BufferHandle m_terrainCompactGroupOffsetsBuffer;
    nvrhi::BufferHandle m_terrainMaterialIDBuffer;

    nvrhi::ComputePipelineHandle m_terrainApplyVisibilityPipeline;
    nvrhi::BindingLayoutHandle m_terrainApplyVisibilityLayout;

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
