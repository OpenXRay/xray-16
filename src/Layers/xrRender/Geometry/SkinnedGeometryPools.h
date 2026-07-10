#pragma once

#include <nvrhi/nvrhi.h>
#include "xrCore/xrCore.h"

class VertexStagingBuffer;
class IndexStagingBuffer;

namespace xray::render::fg {

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

    void Reset();

    u32 GetGeneration() const { return m_generation; }

private:
    u32 m_generation = 1;

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
