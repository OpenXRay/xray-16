#pragma once

#include "xrCore/xrCore.h"
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
    u32 skinned = 0;
    u32 grass = 0;
};

class RTFrameResources
{
public:
    framegraph::VirtualResourceHandle tlas;
    framegraph::VirtualResourceHandle batchInfo;
    framegraph::VirtualResourceHandle vertices;
    framegraph::VirtualResourceHandle indices;
    framegraph::VirtualResourceHandle materials;
    framegraph::VirtualResourceHandle terrainMaterials;
    framegraph::VirtualResourceHandle skinnedVertices;
    framegraph::VirtualResourceHandle skinnedIndices;
    framegraph::VirtualResourceHandle grassVertices;
    framegraph::VirtualResourceHandle grassIndices;
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
    nvrhi::IBuffer* skinnedVertices = nullptr;
    nvrhi::IBuffer* skinnedIndices = nullptr;
    nvrhi::IBuffer* grassVertices = nullptr;
    nvrhi::IBuffer* grassIndices = nullptr;
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
};
static_assert(sizeof(GrassRTCB) == 80);

class BillboardRTCB
{
public:
    u32 maxVertsPerBillboard;
    u32 billboardCount;
    u32 pad[2];
};
static_assert(sizeof(BillboardRTCB) == 16);

class RTGeometryBuild
{
public:
    nvrhi::rt::AccelStructDesc desc;
    nvrhi::rt::AccelStructHandle handle;
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

private:
    IRenderBackend* m_backend = nullptr;
    nvrhi::DescriptorTableHandle m_table;
    xr_vector<u32> m_indices;
};

class RTStaticGeometry
{
public:
    nvrhi::BufferHandle vertices;
    nvrhi::BufferHandle indices;
    xr_vector<RTGeometryBuild> builds;
    xr_vector<nvrhi::rt::InstanceDesc> instances;
    xr_vector<RTBatchInfo> batches;
    RTBatchCounts counts;
    RTTextureBindings textures;
    bool emptySource = false;
    bool recorded = false;
};

class RTSkinJob
{
public:
    RTSkinningCB constants;
    u32 sourceSlot;
    u32 indexOffset;
    u32 indexCount;
    u32 materialID;
};

class RTGrassJob
{
public:
    nvrhi::BufferHandle visible;
    GrassRTCB constants;
};

class RTSceneGeneration
{
public:
    std::shared_ptr<RTStaticGeometry> geometry;
    RTTextureBindings textures;
    nvrhi::rt::AccelStructHandle tlas;
    RTGeometryBuild skinBuild;
    RTGeometryBuild grassBuild;
    nvrhi::BufferHandle batchInfo;
    nvrhi::BufferHandle materials;
    nvrhi::BufferHandle terrainMaterials;
    nvrhi::BufferHandle sourceMaterials;
    nvrhi::BufferHandle sourceTerrainMaterials;
    nvrhi::BufferHandle bones;
    xr_vector<nvrhi::BufferHandle> skinSources;
    nvrhi::BufferHandle skinnedVertices;
    nvrhi::BufferHandle skinnedIndices;
    nvrhi::BufferHandle grassVertices;
    nvrhi::BufferHandle grassIndices;
    std::shared_ptr<const FGDetailManager::VisibilityFrame> grassFrame;
    nvrhi::ComputePipelineHandle grassPipeline;
    nvrhi::BindingLayoutHandle grassLayout;
    nvrhi::TextureHandle grassWind;
    xr_vector<RTSkinJob> skinJobs;
    xr_vector<RTGrassJob> grassJobs;
    xr_vector<u32> skinIndexData;
    xr_vector<RTBatchInfo> batches;
    xr_vector<nvrhi::rt::InstanceDesc> instances;
    RTBatchCounts counts;
    BillboardRTCB billboardConstants = {};
    u32 grassVertexCount = 0;
    u32 grassIndexCount = 0;
    u32 detailAtlasIndex = 0;
    u32 leases = 0;
    bool billboard = false;
    bool recorded = false;
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
    std::shared_ptr<RTSceneGeneration> scene;
    RTFrameResources resources;
    framegraph::VirtualResourceHandle sourceMaterials;
    framegraph::VirtualResourceHandle sourceTerrainMaterials;
    framegraph::VirtualResourceHandle bones;
    xr_vector<framegraph::VirtualResourceHandle> skinSources;
    xr_vector<framegraph::VirtualResourceHandle> grassSources;
    xr_vector<framegraph::VirtualResourceHandle> grassVisible;
    framegraph::VirtualResourceHandle grassModels;
    framegraph::VirtualResourceHandle grassPulledVertices;
    xr_vector<framegraph::VirtualResourceHandle> buffers;
    xr_vector<framegraph::VirtualResourceHandle> structures;
    framegraph::VirtualResourceHandle wind;
};

class RTAccelStructManager
{
public:
    void Initialize(RenderDevice* device);
    void Shutdown();
    bool SetupBuildPass(framegraph::FrameGraph& graph, GPUCullingManager* gpuCulling,
        FGDetailManager* detailMgr, const xr_vector<GeometryBatch>& worldBatches,
        const xr_vector<GeometryBatch>& hudBatches, bool rebuildDynamic);
    RTFrameResources UseScene(framegraph::FrameGraph& graph,
        framegraph::RenderPassBuilder& builder) const;
    static RTFrameBuffers ResolveScene(const framegraph::FrameGraph& graph,
        const RTFrameResources& resources);
    static void InvalidateShaderPipelines();
    bool IsReady() const;
    bool IsSupported() const;
    const RTBatchCounts& GetBatchCounts() const;
    u32 GetDetailAtlasIndex() const;
    RTMemoryStats GetMemoryStats(const GPUCullingManager* gpu) const;
    void RetireScenes();

private:
    void AppendMaterialTextures(u32 materialID, bool terrain);
    void PrepareStatic(GPUCullingManager* gpuCulling);
    void PrepareSkin(RTSceneGeneration& scene, GPUCullingManager* gpuCulling,
        const xr_vector<GeometryBatch>& worldBatches, const xr_vector<GeometryBatch>& hudBatches);
    void PrepareGrass(RTSceneGeneration& scene, FGDetailManager* detailMgr);
    void PrepareScene(GPUCullingManager* gpuCulling, FGDetailManager* detailMgr,
        const xr_vector<GeometryBatch>& worldBatches, const xr_vector<GeometryBatch>& hudBatches);
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
    std::shared_ptr<RTStaticGeometry> m_staticGeometry;
    std::shared_ptr<RTSceneGeneration> m_scene;
    xr_vector<std::shared_ptr<RTSceneGeneration>> m_generations;
    xr_vector<RTLeaseRecord> m_leases;
    xr_vector<u32> m_textureScratch;

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
