#pragma once

#include <nvrhi/nvrhi.h>
#include "xrCore/xrCore.h"

class VertexStagingBuffer;
class IndexStagingBuffer;

namespace xray::render::fg {

enum VertexFormatID : u32
{
    VF_MDI = 0,
    VF_SKINNED_NONHQ = 1,
    VF_SKINNED_HQ1W = 2,
    VF_SKINNED_HQ4W = 3,
    VF_SKINNED_HQ2W = 4,
    VF_SKINNED_HQ3W = 5,
};

u32 SkinnedFormatFromRenderMode(u16 renderMode, u32 vertexStride);
u32 SkinnedFormatStride(u32 formatID);

class SkinnedGeometryPools
{
public:
    static constexpr u32 FIRST_FORMAT = 1;
    static constexpr u32 FORMAT_COUNT = 6;

    bool Register(VertexStagingBuffer* vsb, IndexStagingBuffer* isb,
        u32 vCount, u32 vStride, u32 iCount, u32 formatID);

    void FlushUploads(nvrhi::IDevice* nvDevice, nvrhi::ICommandList* cmdList);

    nvrhi::IBuffer* GetVertexBuffer(u32 formatID) const;
    nvrhi::IBuffer* GetIndexBuffer(u32 formatID) const;
    nvrhi::IBuffer* GetCombinedIndexBuffer() const { return m_combinedIndexBuffer.Get(); }
    u32 GetFormatIndexBase(u32 formatID) const { return formatID < FORMAT_COUNT ? m_formatIndexBase[formatID] : 0; }

    void Reset();

    u32 GetGeneration() const { return m_generation; }

private:
    u32 m_generation = 1;
    nvrhi::BufferHandle m_combinedIndexBuffer;
    u32 m_combinedIndexCount = 0;
    u32 m_formatIndexBase[FORMAT_COUNT] = {};
    bool m_combinedDirty = false;

    struct Pool
    {
        xr_vector<u8> vertexData;
        xr_vector<u8> indexData;
        u32 vertexCount = 0;
        u32 indexCount = 0;
        nvrhi::BufferHandle vertexBuffer;
        nvrhi::BufferHandle indexBuffer;
        size_t vertexBytesUploaded = 0;
        size_t indexBytesUploaded = 0;
    };

    Pool m_pools[FORMAT_COUNT];
};

} // namespace xray::render::fg
