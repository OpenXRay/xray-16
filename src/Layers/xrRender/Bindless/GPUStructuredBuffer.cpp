#include "stdafx.h"
#include "GPUStructuredBuffer.h"
#include "BindlessTypes.h"
#include "VariantTextureBuffer.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"

namespace xray::render::RENDER_NAMESPACE::bindless {

template<typename T>
bool GPUStructuredBuffer<T>::Initialize(ng::RenderDevice* device, const char* debugName, u32 maxElements)
{
    if (m_initialized)
        return true;

    m_device = device;
    m_maxElements = maxElements;

    nvrhi::BufferDesc desc;
    desc.debugName = debugName;
    desc.byteSize = maxElements * sizeof(T);
    desc.structStride = sizeof(T);
    desc.initialState = nvrhi::ResourceStates::ShaderResource;
    desc.keepInitialState = true;

    m_buffer = device->GetNVRHIDevice()->createBuffer(desc);
    if (!m_buffer)
        return false;

    if (m_data.size() < maxElements)
        m_data.resize(maxElements);

    m_initialized = true;
    m_dirtyRangeStart = UINT32_MAX;
    m_dirtyRangeEnd = 0;
    return true;
}

template<typename T>
void GPUStructuredBuffer<T>::Shutdown()
{
    m_buffer = nullptr;
    m_initialized = false;
    m_fullUploadNeeded = false;
    m_dirtyRangeStart = UINT32_MAX;
    m_dirtyRangeEnd = 0;
}

template<typename T>
void GPUStructuredBuffer<T>::Upload(ng::RenderContext* ctx)
{
    if (!m_initialized || !m_buffer)
        return;

    nvrhi::ICommandList* cmdList = ctx->GetCommandList();
    if (!cmdList)
        return;

    if (m_fullUploadNeeded)
    {
        if (m_uploadCount > 0)
            cmdList->writeBuffer(m_buffer, m_data.data(), m_uploadCount * sizeof(T));
        m_fullUploadNeeded = false;
        m_dirtyRangeStart = UINT32_MAX;
        m_dirtyRangeEnd = 0;
    }
    else if (m_dirtyRangeEnd > m_dirtyRangeStart)
    {
        u32 offset = m_dirtyRangeStart * sizeof(T);
        u32 size = (m_dirtyRangeEnd - m_dirtyRangeStart) * sizeof(T);
        cmdList->writeBuffer(m_buffer, &m_data[m_dirtyRangeStart], size, offset);
        m_dirtyRangeStart = UINT32_MAX;
        m_dirtyRangeEnd = 0;
    }
}

template class GPUStructuredBuffer<MaterialData>;
template class GPUStructuredBuffer<TerrainMaterialData>;
template class GPUStructuredBuffer<VariantTextureData>;

} // namespace xray::render::RENDER_NAMESPACE::bindless
