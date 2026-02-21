#pragma once

#include "xrCore/xrCore.h"
#include <nvrhi/nvrhi.h>

namespace xray::render {
    struct GeometryBatch;
}

namespace xray::render::RENDER_NAMESPACE {
    class FGDetailManager;
}

namespace xray::render::ng {
class RenderDevice;
class RenderContext;
}

namespace xray::render::RENDER_NAMESPACE {
class GPUCullingManager;

struct RTBatchInfo {
    u32 materialID;
    u32 startIndex;
    s32 baseVertex;
    u32 indexCount;
};
static_assert(sizeof(RTBatchInfo) == 16, "RTBatchInfo must be 16 bytes");

struct RTBatchCounts {
    u32 identityStatic = 0;
    u32 terrain = 0;
    u32 transparent = 0;
    u32 instancedTotal = 0;
    u32 skinned = 0;
    u32 grass = 0;
};

class RTAccelStructManager {
public:
    void Initialize(ng::RenderDevice* device);
    void Shutdown();

    void BuildIfNeeded(nvrhi::ICommandList* cmdList, GPUCullingManager* gpuCulling);

    void BuildSkinnedBLAS(nvrhi::ICommandList* cmdList, GPUCullingManager* gpuCulling,
                          const xr_vector<GeometryBatch>& worldBatches,
                          const xr_vector<GeometryBatch>& hudBatches);
    void BuildGrassBLAS(nvrhi::ICommandList* cmdList, FGDetailManager* detailMgr);
    void RebuildDynamic(nvrhi::ICommandList* cmdList, GPUCullingManager* gpuCulling);
    void InvalidateSkinned();
    void InvalidateGrass();

    bool IsReady() const { return m_isReady; }
    bool IsSupported() const { return m_rtSupported; }

    nvrhi::rt::IAccelStruct* GetTLAS() const { return m_tlas.Get(); }
    nvrhi::IBuffer* GetBatchInfoBuffer() const { return m_batchInfoBuffer.Get(); }
    nvrhi::IBuffer* GetMegaVB() const { return m_megaVB; }
    nvrhi::IBuffer* GetMegaIB() const { return m_megaIB; }
    nvrhi::IBuffer* GetMaterialBuffer() const { return m_materialBuffer; }
    nvrhi::IBuffer* GetTerrainMaterialBuffer() const { return m_terrainMaterialBuffer; }
    nvrhi::IBuffer* GetSkinnedOutputVB() const { return m_skinnedOutputVB.Get(); }
    nvrhi::IBuffer* GetSkinnedIB() const { return m_skinnedIB.Get(); }
    nvrhi::IBuffer* GetGrassOutputVB() const { return m_grassOutputVB.Get(); }
    nvrhi::IBuffer* GetGrassIB() const { return m_grassIB.Get(); }
    u32 GetBatchCount() const { return m_batchCount; }
    const RTBatchCounts& GetBatchCounts() const { return m_batchCounts; }

    void SetMaterialBuffer(nvrhi::IBuffer* buf) { m_materialBuffer = buf; }
    void SetTerrainMaterialBuffer(nvrhi::IBuffer* buf) { m_terrainMaterialBuffer = buf; }

private:
    struct GeometryKey {
        u32 startIndex;
        s32 baseVertex;
        u32 indexCount;
        bool operator<(const GeometryKey& o) const {
            if (startIndex != o.startIndex) return startIndex < o.startIndex;
            if (baseVertex != o.baseVertex) return baseVertex < o.baseVertex;
            return indexCount < o.indexCount;
        }
    };

    struct InstanceInfo {
        Fmatrix world;
        u32 materialID;
    };

    struct UniqueGeometry {
        GeometryKey key;
        u32 vertexCount;
        nvrhi::rt::AccelStructHandle blas;
        xr_vector<InstanceInfo> instances;
    };

    struct SkinnedBatchRT {
        u32 vertexOffset;
        u32 vertexCount;
        u32 indexOffset;
        u32 indexCount;
        u32 materialID;
        nvrhi::IBuffer* srcVB;
        u32 srcStride;
        u32 srcBaseVertex;
        u32 formatID;
        u32 boneOffset;
        Fmatrix worldMatrix;
        nvrhi::IBuffer* srcIB;
        u32 srcStartIndex;
    };

    void BuildStaticBLAS(nvrhi::ICommandList* cmdList, GPUCullingManager* gpuCulling);
    void BuildInstancedBLAS(nvrhi::ICommandList* cmdList, GPUCullingManager* gpuCulling);
    void BuildTLAS(nvrhi::ICommandList* cmdList);
    void CreateBatchInfoBuffer(nvrhi::ICommandList* cmdList, GPUCullingManager* gpuCulling);
    void InitSkinningPipeline();
    void InitGrassPipeline();
    u32 GetSkinningFormatID(u16 renderMode, u32 stride);

    ng::RenderDevice* m_device = nullptr;
    bool m_rtSupported = false;
    bool m_isReady = false;
    u32 m_batchCount = 0;
    RTBatchCounts m_batchCounts = {};

    nvrhi::rt::AccelStructHandle m_staticBlas;
    xr_vector<UniqueGeometry> m_uniqueGeometries;
    nvrhi::rt::AccelStructHandle m_tlas;
    nvrhi::BufferHandle m_batchInfoBuffer;

    nvrhi::IBuffer* m_megaVB = nullptr;
    nvrhi::IBuffer* m_megaIB = nullptr;
    nvrhi::IBuffer* m_materialBuffer = nullptr;
    nvrhi::IBuffer* m_terrainMaterialBuffer = nullptr;

    nvrhi::BufferHandle m_skinnedOutputVB;
    nvrhi::BufferHandle m_skinnedIB;
    nvrhi::rt::AccelStructHandle m_skinnedBlas;
    xr_vector<SkinnedBatchRT> m_skinnedBatchData;
    bool m_skinnedReady = false;

    nvrhi::BufferHandle m_grassOutputVB;
    nvrhi::BufferHandle m_grassIB;
    nvrhi::rt::AccelStructHandle m_grassBlas;
    u32 m_grassTotalVerts = 0;
    u32 m_grassTotalIndices = 0;
    bool m_grassReady = false;

    static nvrhi::ComputePipelineHandle s_skinPipeline;
    static nvrhi::BindingLayoutHandle s_skinLayout;
    static nvrhi::BufferHandle s_skinCB;
    static bool s_skinInitialized;

    static nvrhi::ComputePipelineHandle s_grassPipeline;
    static nvrhi::BindingLayoutHandle s_grassLayout;
    static nvrhi::BufferHandle s_grassCB;
    static nvrhi::SamplerHandle s_grassSampler;
    static bool s_grassInitialized;
};

}
