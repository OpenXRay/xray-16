#pragma once

#include "xrCore/xrCore.h"
#include "xrCommon/xr_map.h"
#include "xrCommon/xr_set.h"
#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/RenderContext/ResourceHandle.h"
#include "Layers/xrRender/FGDetailManager.h"
#include <nvrhi/nvrhi.h>
#include <memory>

class IRenderBackend;

namespace xray::render
{
struct GeometryBatch;
namespace framegraph
{
class FrameGraph;
class RenderPassBuilder;
}
}

namespace xray::render::fg
{
class FGDetailManager;
class GPUCullingManager;
class RenderDevice;
class IndexStagingBuffer;

class RTBatchInfo
{
public:
    u32 materialID;
    u32 startIndex;
    s32 baseVertex;
    u32 indexCount;
};
static_assert(sizeof(RTBatchInfo) == 16);

class RTBatchCounts
{
public:
    u32 identityStatic = 0;
    u32 terrain = 0;
    u32 transparent = 0;
    u32 instancedTotal = 0;
    u32 dynamic = 0;
    u32 skinned = 0;
    u32 grass = 0;
};

class RTEmissiveTriangle
{
public:
    u32 batchIndex;
    u32 primitiveIndex;
    u32 idLow;
    u32 idHigh;
};
static_assert(sizeof(RTEmissiveTriangle) == 16);

class RTBatchTransform
{
public:
    float rows[3][4];
};
static_assert(sizeof(RTBatchTransform) == 48);

class RTBatchSource
{
public:
    u32 array;
    u32 index;
};
static_assert(sizeof(RTBatchSource) == 8);

class RTFrameResources
{
public:
    framegraph::VirtualResourceHandle tlas;
    framegraph::VirtualResourceHandle batchInfo;
    framegraph::VirtualResourceHandle vertices;
    framegraph::VirtualResourceHandle indices;
    framegraph::VirtualResourceHandle materials;
    framegraph::VirtualResourceHandle terrainMaterials;
    framegraph::VirtualResourceHandle variants;
    framegraph::VirtualResourceHandle grassMaterials;
    framegraph::VirtualResourceHandle skinnedVertices;
    framegraph::VirtualResourceHandle skinnedIndices;
    framegraph::VirtualResourceHandle grassVertices;
    framegraph::VirtualResourceHandle grassIndices;
    framegraph::VirtualResourceHandle emissiveTriangles;
    framegraph::VirtualResourceHandle batchTransforms;
    framegraph::VirtualResourceHandle emissiveBatchOffsets;
    u32 emissiveCount = 0;
    nvrhi::DescriptorTableHandle textures;
};

class RTFrameBuffers
{
public:
    nvrhi::rt::IAccelStruct* tlas = nullptr;
    nvrhi::IBuffer* batchInfo = nullptr;
    nvrhi::IBuffer* vertices = nullptr;
    nvrhi::IBuffer* indices = nullptr;
    nvrhi::IBuffer* materials = nullptr;
    nvrhi::IBuffer* terrainMaterials = nullptr;
    nvrhi::IBuffer* variants = nullptr;
    nvrhi::IBuffer* grassMaterials = nullptr;
    nvrhi::IBuffer* skinnedVertices = nullptr;
    nvrhi::IBuffer* skinnedIndices = nullptr;
    nvrhi::IBuffer* grassVertices = nullptr;
    nvrhi::IBuffer* grassIndices = nullptr;
    nvrhi::IBuffer* emissiveTriangles = nullptr;
    nvrhi::IBuffer* batchTransforms = nullptr;
    nvrhi::IBuffer* emissiveBatchOffsets = nullptr;
    u32 emissiveCount = 0;
    nvrhi::IDescriptorTable* textures = nullptr;
};

class RTMemoryStats
{
public:
    u64 sourceBytes = 0;
    u64 generationBytes = 0;
    u64 accelerationBytes = 0;
    u32 pendingLeases = 0;
    u32 generations = 0;
    bool accelerationBytesKnown = true;
};

class RTSkinningCB
{
public:
    Fmatrix worldMatrix;
    Fmatrix normalMatrix;
    u32 vertexCount;
    u32 vertexStride;
    u32 formatID;
    u32 boneOffset;
    u32 outputOffset;
    u32 inputBaseVertex;
    u32 pad[2];
};
static_assert(sizeof(RTSkinningCB) == 160);

class GrassRTCB
{
public:
    Fvector4 wind_direction;
    Fvector4 wave;
    float grass_wind_displacement;
    float grass_blade_height;
    float grass_blade_width;
    u32 segments;
    u32 vertsPerBlade;
    u32 bladeCount;
    u32 outputVertexOffset;
    u32 indicesPerBlade;
    u32 outputIndexOffset;
    u32 pad[3];
    Fvector4 interaction_window;
    float grass_interaction_displacement;
    float grass_interaction_max_angle;
    float interactionPad[2];
};
static_assert(sizeof(GrassRTCB) == 112);

class BillboardRTCB
{
public:
    u32 maxVertsPerBillboard;
    u32 billboardCount;
    u32 outputVertexOffset;
    u32 outputIndexOffset;
    Fvector4 wind_direction;
    Fvector4 wave;
    Fvector4 interaction_window;
    float grass_wind_displacement;
    float grass_interaction_displacement;
    float grass_interaction_max_angle;
    u32 detailKind;
};
static_assert(sizeof(BillboardRTCB) == 80);

class RTGeometryBuild
{
public:
    nvrhi::rt::AccelStructDesc desc;
    nvrhi::rt::AccelStructHandle handle;
    u64 topologyKey = 0;
    bool built = false;
    bool update = false;
};

class RTTextureBindings
{
public:
    RTTextureBindings();
    ~RTTextureBindings();
    RTTextureBindings(const RTTextureBindings&) = delete;
    RTTextureBindings& operator=(const RTTextureBindings&) = delete;
    void Capture(xr_vector<u32>& indices);
    nvrhi::IDescriptorTable* GetTable() const;
    const xr_set<nvrhi::ITexture*>& GetTextures() const;
    const xr_vector<u32>& GetIndices() const { return m_indices; }

private:
    IRenderBackend* m_backend = nullptr;
    nvrhi::DescriptorTableHandle m_table;
    xr_vector<u32> m_indices;
    xr_set<nvrhi::ITexture*> m_textures;
};

class RTStaticGeometry
{
public:
    nvrhi::BufferHandle vertices;
    nvrhi::BufferHandle indices;
    xr_vector<RTGeometryBuild> builds;
    xr_vector<nvrhi::rt::InstanceDesc> instances;
    xr_vector<s32> instanceBatches;
    xr_vector<RTBatchInfo> batches;
    xr_vector<RTBatchSource> batchSources;
    RTBatchCounts counts;
    RTTextureBindings textures;
    bool emptySource = false;
    bool recorded = false;
    bool failed = false;
};

class RTDynamicRange
{
public:
    u32 vertexOffset;
    u32 indexOffset;
    u32 vertexCount;
    u32 indexCount;
    bool opaque;

    bool operator<(const RTDynamicRange& other) const;
};

class RTDynamicGeometry
{
public:
    xr_vector<RTGeometryBuild> builds;
    xr_map<RTDynamicRange, u32> recordBuilds;
    bool failed = false;
    bool recorded = false;
};

class RTSkinTopology
{
public:
    nvrhi::BufferHandle indices;
    xr_vector<u32> staging;
    u32 indexCount = 0;
    bool uploadPending = false;
};

class RTSkinJob
{
public:
    RTSkinningCB constants;
    u32 sourceSlot;
    u32 indexOffset;
    u32 indexCount;
    u32 materialID;
    u64 geometryID;
};

class RTSkinSourcePlan
{
public:
    nvrhi::IBuffer* source = nullptr;
    IndexStagingBuffer* staging = nullptr;
    u32 format = 0;
    u32 firstIndex = 0;
    u32 indexCount = 0;
    u32 indexOffset = 0;
    u32 vertexCount = 0;
    u32 baseVertex = 0;
    bool pooled = false;
};

class RTGrassJob
{
public:
    nvrhi::BufferHandle visible;
    GrassRTCB constants;
    u32 lod = 0;
};

class RTPulledJob
{
public:
    nvrhi::BufferHandle visible;
    BillboardRTCB constants;
};

enum class RTBuildScope : u8
{
    None,
    Pose,
    Full,
};

class RTPoseSignature
{
public:
    u64 refresh = 0;
    u64 motion = 0;
};

class RTSceneGeneration
{
public:
    std::shared_ptr<RTStaticGeometry> geometry;
    std::shared_ptr<RTDynamicGeometry> dynamicGeometry;
    RTTextureBindings textures;
    nvrhi::rt::AccelStructHandle tlas;
    u32 tlasCapacity = 0;
    u32 tlasBuildCount = 0;
    bool tlasBuilt = false;
    bool tlasUpdate = false;
    RTGeometryBuild skinBuild;
    RTGeometryBuild hudSkinBuild;
    RTGeometryBuild grassBuild;
    nvrhi::BufferHandle batchInfo;
    nvrhi::BufferHandle materials;
    nvrhi::BufferHandle terrainMaterials;
    nvrhi::BufferHandle sourceMaterials;
    nvrhi::BufferHandle sourceTerrainMaterials;
    nvrhi::BufferHandle sourceVariants;
    nvrhi::BufferHandle variants;
    nvrhi::BufferHandle grassMaterials;
    nvrhi::BufferHandle bones;
    xr_vector<nvrhi::BufferHandle> skinSources;
    nvrhi::BufferHandle skinnedVertices;
    nvrhi::BufferHandle skinnedIndices;
    nvrhi::BufferHandle grassVertices;
    nvrhi::BufferHandle grassIndices;
    std::shared_ptr<const FGDetailManager::VisibilityFrame> grassFrame;
    nvrhi::ComputePipelineHandle grassPipeline;
    nvrhi::BindingLayoutHandle grassLayout;
    nvrhi::ComputePipelineHandle pulledPipeline;
    nvrhi::BindingLayoutHandle pulledLayout;
    nvrhi::TextureHandle grassWind;
    nvrhi::TextureHandle grassInteraction[2];
    xr_vector<RTSkinJob> skinJobs;
    xr_vector<RTSkinJob> hudSkinJobs;
    xr_vector<RTGrassJob> grassJobs;
    xr_vector<RTPulledJob> detailMeshJobs;
    xr_vector<RTPulledJob> staticDetailJobs;
    std::shared_ptr<RTSkinTopology> skinTopology;
    xr_vector<RTBatchInfo> batches;
    xr_vector<RTBatchTransform> batchTransforms;
    xr_vector<u64> batchIdentities;
    xr_vector<RTEmissiveTriangle> emissiveTriangles;
    xr_vector<u32> emissiveBatchOffsets;
    xr_vector<nvrhi::rt::InstanceDesc> instances;
    RTBatchCounts counts;
    nvrhi::BufferHandle emissiveTriangleBuffer;
    nvrhi::BufferHandle batchTransformBuffer;
    nvrhi::BufferHandle emissiveBatchOffsetBuffer;
    u32 emissiveCount = 0;
    u32 grassVertexCount = 0;
    u32 grassIndexCount = 0;
    u32 detailAtlasIndex = 0;
    u32 detailMeshBatchStart = UINT32_MAX;
    u32 staticDetailBatchStart = UINT32_MAX;
    u32 detailPbrIndex = 0;
    u32 detailBumpIndex = 0;
    u32 staticDetailInstanceCount = 0;
    u32 leases = 0;
    u32 retention = 0;
    u32 tableClearMask = 0;
    bool billboard = false;
    bool recorded = false;
    bool failed = false;
};

class RTLeaseRecord
{
public:
    std::shared_ptr<RTSceneGeneration> scene;
    u64 lease = 0;
};

class RTBuildPassData
{
public:
    class RTAccelStructManager* manager = nullptr;
    FGDetailManager* detailManager = nullptr;
    std::shared_ptr<RTSceneGeneration> scene;
    RTFrameResources resources;
    framegraph::VirtualResourceHandle sourceMaterials;
    framegraph::VirtualResourceHandle sourceTerrainMaterials;
    framegraph::VirtualResourceHandle sourceVariants;
    framegraph::VirtualResourceHandle bones;
    xr_vector<framegraph::VirtualResourceHandle> skinSources;
    xr_vector<framegraph::VirtualResourceHandle> grassSources;
    xr_vector<framegraph::VirtualResourceHandle> grassVisible;
    xr_vector<framegraph::VirtualResourceHandle> detailMeshVisible;
    xr_vector<framegraph::VirtualResourceHandle> staticDetailVisible;
    framegraph::VirtualResourceHandle detailModels;
    framegraph::VirtualResourceHandle detailPulledVertices;
    xr_vector<framegraph::VirtualResourceHandle> buffers;
    xr_vector<framegraph::VirtualResourceHandle> structures;
    framegraph::VirtualResourceHandle wind;
    framegraph::VirtualResourceHandle interaction[2];
    RTBuildScope scope = RTBuildScope::Full;
};

class RTAccelStructManager
{
public:
    static constexpr u32 SKIN_VERTEX_STRIDE = 32u;

    void Initialize(RenderDevice* device);
    void Shutdown();
    bool SetupBuildPass(framegraph::FrameGraph& graph, GPUCullingManager* gpuCulling,
        FGDetailManager* detailMgr, const xr_vector<GeometryBatch>& worldBatches,
        const xr_vector<GeometryBatch>& hudBatches);
    RTFrameResources UseScene(framegraph::FrameGraph& graph,
        framegraph::RenderPassBuilder& builder) const;
    RTFrameResources UseScene(framegraph::FrameGraph& graph, framegraph::RenderPassBuilder& builder,
        const std::shared_ptr<RTSceneGeneration>& scene) const;
    std::shared_ptr<RTSceneGeneration> GetScene() const;
    void RetainScene(const std::shared_ptr<RTSceneGeneration>& scene);
    void ReleaseScene(const std::shared_ptr<RTSceneGeneration>& scene);
    bool IsSceneValid(const RTSceneGeneration& scene) const;
    static RTFrameBuffers ResolveScene(const framegraph::FrameGraph& graph,
        const RTFrameResources& resources);
    static void InvalidateShaderPipelines();
    bool IsReady() const;
    bool IsSupported() const;
    const RTBatchCounts& GetBatchCounts() const;
    u32 GetDetailAtlasIndex() const;
    RTMemoryStats GetMemoryStats(const GPUCullingManager* gpu) const;
    u64 GetSceneRevision() const;
    u64 GetPoseRevision() const;
    u64 GetTextureRevision(nvrhi::ITexture* sky0, nvrhi::ITexture* sky1) const;
    void RetireScenes();

private:
    static void HashSceneData(u64& signature, const void* data, size_t size);
    static bool IsOpaqueMaterialForRT(u32 materialID, bool terrain);
    static bool AccelStructShapeMatches(const nvrhi::rt::AccelStructDesc& cached,
        const nvrhi::rt::AccelStructDesc& requested);
    static bool AccelStructUpdateMatches(const nvrhi::rt::AccelStructDesc& cached,
        const nvrhi::rt::AccelStructDesc& requested);
    void AcquireGeometryBuild(const nvrhi::rt::AccelStructDesc& requested, RTGeometryBuild& slot,
        bool topologyStable, u64 topologyKey = 0);
    bool IsSceneReady(const RTSceneGeneration& scene) const;
    u64 ComputeStaticSignature(const GPUCullingManager* gpuCulling);
    u64 ComputeTopologySignature(const GPUCullingManager* gpuCulling, const FGDetailManager* detailMgr,
        const xr_vector<GeometryBatch>& worldBatches, const xr_vector<GeometryBatch>& hudBatches);
    RTPoseSignature ComputePoseSignature(const GPUCullingManager* gpuCulling, const FGDetailManager* detailMgr,
        const xr_vector<GeometryBatch>& worldBatches, const xr_vector<GeometryBatch>& hudBatches) const;
    void AppendMaterialTextures(u32 materialID, bool terrain);
    void PrepareStatic(GPUCullingManager* gpuCulling);
    bool EnsureDynamicGeometry(GPUCullingManager* gpuCulling);
    void PrepareDynamic(RTSceneGeneration& scene, GPUCullingManager* gpuCulling);
    void PrepareSkin(RTSceneGeneration& scene, GPUCullingManager* gpuCulling,
        const xr_vector<GeometryBatch>& worldBatches, const xr_vector<GeometryBatch>& hudBatches);
    void PrepareGrass(RTSceneGeneration& scene, FGDetailManager* detailMgr);
    void PrepareScene(GPUCullingManager* gpuCulling, FGDetailManager* detailMgr,
        const xr_vector<GeometryBatch>& worldBatches, const xr_vector<GeometryBatch>& hudBatches);
    void RefreshGeometryBuild(RTGeometryBuild& slot) const;
    bool RefreshPose(GPUCullingManager* gpuCulling, FGDetailManager* detailMgr,
        const xr_vector<GeometryBatch>& worldBatches, const xr_vector<GeometryBatch>& hudBatches);
    void RegisterBuildPasses(framegraph::FrameGraph& graph, FGDetailManager* detailMgr, RTBuildScope scope);
    void RecordInputs(const RTBuildPassData& data, const framegraph::FrameGraph& graph,
        nvrhi::ICommandList* commandList);
    void RecordBLAS(const RTBuildPassData& data, const framegraph::FrameGraph& graph,
        nvrhi::ICommandList* commandList);
    void RecordTLAS(const RTBuildPassData& data, const framegraph::FrameGraph& graph,
        nvrhi::ICommandList* commandList);
    RTFrameResources ImportScene(framegraph::FrameGraph& graph,
        const RTSceneGeneration& scene) const;
    bool EnsureBuildResources(FGDetailManager* detailMgr, bool needsSkin);
    bool InitSkinningPipeline();
    bool InitGrassPipeline(const FGDetailManager::InstanceGeneration& source);
    bool InitBillboardPipeline(const FGDetailManager::InstanceGeneration& source);
    static u32 GetSkinningFormatID(u32 poolFormat);

    RenderDevice* m_device = nullptr;
    bool m_rtSupported = false;
    bool m_inPlaceUpdates = false;
    std::shared_ptr<RTStaticGeometry> m_staticGeometry;
    std::shared_ptr<RTDynamicGeometry> m_dynamicGeometry;
    nvrhi::IBuffer* m_dynamicSourceVertices = nullptr;
    nvrhi::IBuffer* m_dynamicSourceIndices = nullptr;
    xr_map<u64, std::shared_ptr<RTSkinTopology>> m_skinTopologies;
    std::shared_ptr<RTSceneGeneration> m_scene;
    xr_vector<std::shared_ptr<RTSceneGeneration>> m_generations;
    xr_vector<RTLeaseRecord> m_leases;
    xr_vector<u32> m_textureScratch;
    u64 m_staticSignature = 0;
    u64 m_sceneSignature = 0;
    u64 m_sceneRevision = 0;
    u64 m_poseSignature = 0;
    u64 m_motionSignature = 0;
    u64 m_poseRevision = 0;
    u64 m_staticIdentityHash = 0;
    u32 m_staticIdentityBuildCount = UINT32_MAX;
    u64 m_staticArraysHash = 0;
    u32 m_staticArraysBuildCount = UINT32_MAX;
    u64 m_staticArraysMaterialRevision = 0;
    u64 m_staticArraysVariantRevision = 0;

    static nvrhi::ComputePipelineHandle s_skinPipeline;
    static nvrhi::BindingLayoutHandle s_skinLayout;
    static BufferHandle s_skinCB;
    static bool s_skinAttempted;
    static nvrhi::ComputePipelineHandle s_grassPipeline;
    static nvrhi::BindingLayoutHandle s_grassLayout;
    static nvrhi::BindingLayoutHandle s_grassSourceLayout;
    static BufferHandle s_grassCB;
    static bool s_grassAttempted;
    static nvrhi::ComputePipelineHandle s_billboardPipeline;
    static nvrhi::BindingLayoutHandle s_billboardLayout;
    static nvrhi::BindingLayoutHandle s_billboardSourceLayout;
    static BufferHandle s_billboardCB;
    static bool s_billboardAttempted;
};
}
