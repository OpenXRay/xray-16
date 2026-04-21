#pragma once

#include <nvrhi/nvrhi.h>

namespace xray::render {
    namespace fg {
        class RenderDevice;
        class RenderContext;
    }
}

namespace xray::render::RENDER_NAMESPACE::bindless {

template<typename T>
class GPUStructuredBuffer {
public:
    bool Initialize(fg::RenderDevice* device, const char* debugName, u32 maxElements);
    void Shutdown();
    void Upload(fg::RenderContext* ctx);

    void Set(u32 index, const T& data)
    {
        if (index >= m_maxElements)
            return;
        m_data[index] = data;
        m_dirtyRangeStart = std::min(m_dirtyRangeStart, index);
        m_dirtyRangeEnd = std::max(m_dirtyRangeEnd, index + 1);
    }

    const T* Get(u32 index) const
    {
        return index < m_maxElements ? &m_data[index] : nullptr;
    }

    nvrhi::IBuffer* GetBuffer() const { return m_buffer.Get(); }
    bool IsInitialized() const { return m_initialized; }
    u32 MaxElements() const { return m_maxElements; }

protected:
    GPUStructuredBuffer() = default;
    ~GPUStructuredBuffer() { Shutdown(); }

    fg::RenderDevice* m_device = nullptr;
    nvrhi::BufferHandle m_buffer;
    xr_vector<T> m_data;
    u32 m_maxElements = 0;
    u32 m_uploadCount = 0;
    bool m_initialized = false;
    bool m_fullUploadNeeded = false;
    u32 m_dirtyRangeStart = UINT32_MAX;
    u32 m_dirtyRangeEnd = 0;
};

} // namespace xray::render::RENDER_NAMESPACE::bindless
