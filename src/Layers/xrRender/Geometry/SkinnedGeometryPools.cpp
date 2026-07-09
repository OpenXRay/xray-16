#include "stdafx.h"
#include "SkinnedGeometryPools.h"
#include "Layers/xrRender/BufferUtils.h"
#include "Layers/xrRender/ShaderVariant/VariantPSOCache.h"

namespace xray::render::fg {

u32 SkinnedFormatFromRenderMode(u16 renderMode, u32 vertexStride)
{
    enum {
        RM_SKINNING_SOFT = 0,
        RM_SINGLE = 1,
        RM_SINGLE_HQ = 2,
        RM_SKINNING_1B = 3,
        RM_SKINNING_1B_HQ = 4,
        RM_SKINNING_2B = 5,
        RM_SKINNING_2B_HQ = 6,
        RM_SKINNING_3B = 7,
        RM_SKINNING_3B_HQ = 8,
        RM_SKINNING_4B = 9,
        RM_SKINNING_4B_HQ = 10
    };

    if (renderMode == RM_SKINNING_3B || renderMode == RM_SKINNING_3B_HQ) return VF_SKINNED_HQ3W;
    if (renderMode == RM_SKINNING_2B || renderMode == RM_SKINNING_2B_HQ) return VF_SKINNED_HQ2W;
    if (renderMode == RM_SKINNING_4B || renderMode == RM_SKINNING_4B_HQ) return VF_SKINNED_HQ4W;
    if (renderMode == RM_SKINNING_1B_HQ || renderMode == RM_SINGLE_HQ) return VF_SKINNED_HQ1W;
    if (renderMode == RM_SKINNING_1B || renderMode == RM_SINGLE) return VF_SKINNED_NONHQ;
    if (vertexStride == 36) return VF_SKINNED_HQ1W;
    if (vertexStride == 40) return VF_SKINNED_HQ4W;
    if (vertexStride == 44) return VF_SKINNED_HQ2W;
    return VF_SKINNED_NONHQ;
}

u32 SkinnedFormatStride(u32 formatID)
{
    switch (formatID) {
    case VF_SKINNED_NONHQ: return 24;
    case VF_SKINNED_HQ1W: return 36;
    case VF_SKINNED_HQ4W: return 40;
    case VF_SKINNED_HQ2W: return 44;
    case VF_SKINNED_HQ3W: return 44;
    default: return 0;
    }
}

bool SkinnedGeometryPools::Register(VertexStagingBuffer* vsb, IndexStagingBuffer* isb,
    u32 vCount, u32 vStride, u32 iCount, u32 formatID)
{
    if (!vsb || !isb || vCount == 0 || iCount == 0)
        return false;
    if (formatID < FIRST_FORMAT || formatID >= FORMAT_COUNT)
        return false;
    if (vStride != SkinnedFormatStride(formatID))
        return false;

    if (vsb->skinned_pool_format == formatID)
        return true;
    if (vsb->skinned_pool_format != UINT32_MAX)
        return false;

    const void* vdata = vsb->Map(0, 0, true);
    if (!vdata)
        return false;
    const void* idata = isb->Map(0, 0, true);
    if (!idata) {
        vsb->Unmap();
        return false;
    }

    Pool& pool = m_pools[formatID];
    const u32 baseVertex = pool.vertexCount;
    const u32 firstIndex = pool.indexCount;

    const size_t vBytes = size_t(vCount) * vStride;
    const size_t iBytes = size_t(iCount) * sizeof(u16);
    const u8* vsrc = static_cast<const u8*>(vdata);
    const u8* isrc = static_cast<const u8*>(idata);
    pool.vertexData.insert(pool.vertexData.end(), vsrc, vsrc + vBytes);
    pool.indexData.insert(pool.indexData.end(), isrc, isrc + iBytes);
    pool.vertexCount += vCount;
    pool.indexCount += iCount;

    vsb->Unmap();
    isb->Unmap();

    vsb->skinned_pool_format = formatID;
    vsb->skinned_pool_base_vertex = baseVertex;
    vsb->skinned_pool_first_index = firstIndex;
    return true;
}

void SkinnedGeometryPools::FlushUploads(nvrhi::IDevice* nvDevice, nvrhi::ICommandList* cmdList)
{
    for (u32 f = FIRST_FORMAT; f < FORMAT_COUNT; ++f) {
        Pool& pool = m_pools[f];

        if (!pool.vertexData.empty()) {
            if (!pool.vertexBuffer || pool.vertexBuffer->getDesc().byteSize < pool.vertexData.size()) {
                size_t capacity = 256 * 1024;
                while (capacity < pool.vertexData.size())
                    capacity *= 2;

                nvrhi::BufferDesc desc;
                desc.debugName = "SkinnedPool_VB";
                desc.byteSize = capacity;
                desc.isVertexBuffer = true;
                desc.initialState = nvrhi::ResourceStates::VertexBuffer;
                desc.keepInitialState = true;
                pool.vertexBuffer = nvDevice->createBuffer(desc);
                pool.vertexBytesUploaded = 0;
            }
            if (pool.vertexBuffer && pool.vertexBytesUploaded < pool.vertexData.size()) {
                cmdList->writeBuffer(pool.vertexBuffer,
                    pool.vertexData.data() + pool.vertexBytesUploaded,
                    pool.vertexData.size() - pool.vertexBytesUploaded,
                    pool.vertexBytesUploaded);
                pool.vertexBytesUploaded = pool.vertexData.size();
            }
        }

        if (!pool.indexData.empty()) {
            if (!pool.indexBuffer || pool.indexBuffer->getDesc().byteSize < pool.indexData.size()) {
                size_t capacity = 128 * 1024;
                while (capacity < pool.indexData.size())
                    capacity *= 2;

                nvrhi::BufferDesc desc;
                desc.debugName = "SkinnedPool_IB";
                desc.byteSize = capacity;
                desc.isIndexBuffer = true;
                desc.initialState = nvrhi::ResourceStates::IndexBuffer;
                desc.keepInitialState = true;
                pool.indexBuffer = nvDevice->createBuffer(desc);
                pool.indexBytesUploaded = 0;
            }
            if (pool.indexBuffer && pool.indexBytesUploaded < pool.indexData.size()) {
                cmdList->writeBuffer(pool.indexBuffer,
                    pool.indexData.data() + pool.indexBytesUploaded,
                    pool.indexData.size() - pool.indexBytesUploaded,
                    pool.indexBytesUploaded);
                pool.indexBytesUploaded = pool.indexData.size();
            }
        }
    }
}

nvrhi::IBuffer* SkinnedGeometryPools::GetVertexBuffer(u32 formatID) const
{
    if (formatID >= FORMAT_COUNT)
        return nullptr;
    return m_pools[formatID].vertexBuffer.Get();
}

nvrhi::IBuffer* SkinnedGeometryPools::GetIndexBuffer(u32 formatID) const
{
    if (formatID >= FORMAT_COUNT)
        return nullptr;
    return m_pools[formatID].indexBuffer.Get();
}

void SkinnedGeometryPools::Reset()
{
    for (auto& pool : m_pools)
        pool = Pool{};
}

} // namespace xray::render::fg
