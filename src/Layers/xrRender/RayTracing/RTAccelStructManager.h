#pragma once

#include "xrCore/xrCore.h"
#include <nvrhi/nvrhi.h>

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

class RTAccelStructManager {
public:
    void Initialize(ng::RenderDevice* device);
    void Shutdown();

    void BuildIfNeeded(nvrhi::ICommandList* cmdList, GPUCullingManager* gpuCulling);

    bool IsReady() const { return m_isReady; }
    bool IsSupported() const { return m_rtSupported; }

    nvrhi::rt::IAccelStruct* GetTLAS() const { return m_tlas.Get(); }
    nvrhi::IBuffer* GetBatchInfoBuffer() const { return m_batchInfoBuffer.Get(); }
    nvrhi::IBuffer* GetMegaVB() const { return m_megaVB; }
    nvrhi::IBuffer* GetMegaIB() const { return m_megaIB; }
    nvrhi::IBuffer* GetMaterialBuffer() const { return m_materialBuffer; }
    u32 GetBatchCount() const { return m_batchCount; }

    void SetMaterialBuffer(nvrhi::IBuffer* buf) { m_materialBuffer = buf; }

private:
    void BuildBLAS(nvrhi::ICommandList* cmdList, GPUCullingManager* gpuCulling);
    void BuildTLAS(nvrhi::ICommandList* cmdList);
    void CreateBatchInfoBuffer(nvrhi::ICommandList* cmdList, GPUCullingManager* gpuCulling);

    ng::RenderDevice* m_device = nullptr;
    bool m_rtSupported = false;
    bool m_isReady = false;
    u32 m_batchCount = 0;
    u32 m_staticBatchCount = 0;
    u32 m_terrainBatchCount = 0;
    u32 m_transparentBatchCount = 0;

    nvrhi::rt::AccelStructHandle m_blas;
    nvrhi::rt::AccelStructHandle m_tlas;
    nvrhi::BufferHandle m_batchInfoBuffer;

    nvrhi::IBuffer* m_megaVB = nullptr;
    nvrhi::IBuffer* m_megaIB = nullptr;
    nvrhi::IBuffer* m_materialBuffer = nullptr;
};

}
