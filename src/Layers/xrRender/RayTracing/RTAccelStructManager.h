#pragma once

#include "xrCore/xrCore.h"
#include <nvrhi/nvrhi.h>

namespace xray::render {
    struct GeometryBatch;
}

namespace xray::render::fg {
    class FGDetailManager;
}

namespace xray::render::fg::passes {
    struct ParticleBatch;
}

namespace xray::render::fg {
class RenderDevice;
class RenderContext;
}

namespace xray::render::fg {
class GPUCullingManager;

struct RTBatchInfo {
    u32 materialID;
    u32 startIndex;
    s32 baseVertex;
    u32 indexCount;
};
static_assert(sizeof(RTBatchInfo) == 16, "RTBatchInfo must be 16 bytes");

enum : u32 {
    kRTMaskScene = 0x01,
    kRTMaskParticles = 0x02,
    kRTMaskGrass = 0x04,
};

struct RTBatchCounts {
    u32 identityStatic = 0;
    u32 terrain = 0;
    u32 transparent = 0;
    u32 instancedTotal = 0;
    u32 skinned = 0;
    u32 skinnedWorld = 0;
    u32 skinnedHud = 0;
    u32 grass = 0;
    u32 particles = 0;
};

class RTAccelStructManager {
public:
    void Initialize(fg::RenderDevice* device);
    void Shutdown();

    void BuildIfNeeded(nvrhi::ICommandList* cmdList, GPUCullingManager* gpuCulling);
    void Invalidate();

    void BuildSkinnedBLAS(nvrhi::ICommandList* cmdList, GPUCullingManager* gpuCulling,
                          const xr_vector<GeometryBatch>& worldBatches,
                          const xr_vector<GeometryBatch>& hudBatches);
    void BuildGrassBLAS(nvrhi::ICommandList* cmdList, FGDetailManager* detailMgr);
    void BuildParticleBLAS(nvrhi::ICommandList* cmdList, const xr_vector<passes::ParticleBatch>& worldBatches);
    void RebuildDynamic(nvrhi::ICommandList* cmdList, GPUCullingManager* gpuCulling);
    void InvalidateSkinned();
    void InvalidateGrass();
    void InvalidateParticles();
    static void InvalidateShaderPipelines();

    bool IsReady() const { return m_isReady; }
    bool IsSupported() const { return m_rtSupported; }

    nvrhi::rt::IAccelStruct* GetTLAS() const { return m_tlas.Get(); }
    nvrhi::rt::IAccelStruct* GetOrCreateEmptyTLAS(nvrhi::ICommandList* cmd);
    nvrhi::IBuffer* GetBatchInfoBuffer() const { return m_batchInfoBuffer.Get(); }
    nvrhi::IBuffer* GetMegaVB() const { return m_megaVB; }
    nvrhi::IBuffer* GetMegaIB() const { return m_megaIB; }
    nvrhi::IBuffer* GetMaterialBuffer() const { return m_materialBuffer; }
    nvrhi::IBuffer* GetTerrainMaterialBuffer() const { return m_terrainMaterialBuffer; }
    nvrhi::IBuffer* GetSkinnedOutputVB() const { return m_skinnedSlots[m_skinnedSlot].vb.Get(); }
    nvrhi::IBuffer* GetSkinnedIB() const { return m_skinnedSlots[m_skinnedSlot].ib.Get(); }
    nvrhi::IBuffer* GetGrassOutputVB() const { return m_grassSlots[m_grassSlot].vb.Get(); }
    nvrhi::IBuffer* GetGrassIB() const { return m_grassSlots[m_grassSlot].ib.Get(); }
    nvrhi::IBuffer* GetParticleOutputVB() const { return m_particleOutputVB.Get(); }
    nvrhi::IBuffer* GetParticleIB() const { return m_particleIB.Get(); }
    u32 GetBatchCount() const { return m_batchCount; }
    const RTBatchCounts& GetBatchCounts() const { return m_batchCounts; }
    u32 GetDetailAtlasIndex() const { return m_detailAtlasIndex; }
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
        u32 flags = 0;
    };

    struct UniqueGeometry {
        GeometryKey key;
        u32 vertexCount;
        bool alphaGeometry = false;
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
    void InitBillboardPipeline();
    u32 GetSkinningFormatID(u16 renderMode, u32 stride);

    fg::RenderDevice* m_device = nullptr;
    bool m_rtSupported = false;
    bool m_isReady = false;
    u32 m_batchCount = 0;
    RTBatchCounts m_batchCounts = {};
    bool m_staticHasAlpha = false;

    nvrhi::rt::AccelStructHandle m_staticBlas;
    xr_vector<RTBatchInfo> m_staticGeomInfos;
    xr_vector<UniqueGeometry> m_uniqueGeometries;
    nvrhi::rt::AccelStructHandle m_tlas;
    nvrhi::rt::AccelStructHandle m_emptyTlas;
    nvrhi::rt::AccelStructHandle m_tlasSlots[3];
    u32 m_tlasSlot = 0;
    u32 m_tlasMaxInstances[3] = {};
    nvrhi::BufferHandle m_batchInfoBuffer;
    nvrhi::BufferHandle m_batchInfoSlots[3];
    u32 m_batchInfoSlot = 0;

    nvrhi::IBuffer* m_megaVB = nullptr;
    nvrhi::IBuffer* m_megaIB = nullptr;
    nvrhi::IBuffer* m_materialBuffer = nullptr;
    nvrhi::IBuffer* m_terrainMaterialBuffer = nullptr;

    struct SkinnedFrameSlot {
        nvrhi::BufferHandle vb;
        nvrhi::BufferHandle ib;
        nvrhi::rt::AccelStructHandle blas;
        u64 topoHash = 0;
        u32 totalVerts = 0;
        u32 totalIndices = 0;
    };
    static constexpr u32 kSkinnedSlots = 3;
    SkinnedFrameSlot m_skinnedSlots[kSkinnedSlots];
    u32 m_skinnedSlot = 0;
    nvrhi::rt::AccelStructHandle m_skinnedBlas;
    xr_vector<SkinnedBatchRT> m_skinnedBatchData;
    u32 m_skinnedTotalVerts = 0;
    u32 m_skinnedTotalIndices = 0;
    u64 m_skinnedTopoHash = 0;
    bool m_skinnedReady = false;

    struct GrassFrameSlot {
        nvrhi::BufferHandle vb;
        nvrhi::BufferHandle ib;
        nvrhi::rt::AccelStructHandle blas;
        u32 totalVerts = 0;
        u32 totalIndices = 0;
    };
    static constexpr u32 kGrassSlots = 3;
    GrassFrameSlot m_grassSlots[kGrassSlots];
    u32 m_grassSlot = 0;
    nvrhi::BufferHandle m_grassOutputVB;
    nvrhi::BufferHandle m_grassIB;
    nvrhi::rt::AccelStructHandle m_grassBlas;
    u32 m_grassTotalVerts = 0;
    u32 m_grassTotalIndices = 0;
    bool m_grassReady = false;
    bool m_grassBillboardMode = false;
    u32 m_detailAtlasIndex = 0;
    nvrhi::BufferHandle m_particleOutputVB;
    nvrhi::BufferHandle m_particleIB;
    nvrhi::rt::AccelStructHandle m_particleBlas;
    u32 m_particleTotalVerts = 0;
    u32 m_particleTotalIndices = 0;
    bool m_particleReady = false;

    static nvrhi::ComputePipelineHandle s_skinPipeline;
    static nvrhi::BindingLayoutHandle s_skinLayout;
    static fg::BufferHandle s_skinCB;
    static bool s_skinInitialized;

    static nvrhi::ComputePipelineHandle s_grassPipeline;
    static nvrhi::BindingLayoutHandle s_grassLayout;
    static fg::BufferHandle s_grassCB;
    static nvrhi::SamplerHandle s_grassSampler;
    static bool s_grassInitialized;

    static nvrhi::ComputePipelineHandle s_billboardPipeline;
    static nvrhi::BindingLayoutHandle s_billboardLayout;
    static fg::BufferHandle s_billboardCB;
    static bool s_billboardInitialized;
};

}
