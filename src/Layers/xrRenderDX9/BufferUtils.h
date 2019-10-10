#pragma once

class IndexStagingBuffer
{
public:
    void Create(size_t size);
    void Destroy();

    void* GetHostPointer() const;
    ID3DIndexBuffer* GetBufferHandle() const;
    void Flush();

    operator ID3DIndexBuffer*() const
    {
        return m_DeviceBuffer;
    }

    bool IndexStagingBuffer::operator==(ID3DIndexBuffer* other) const
    {
        return other == m_DeviceBuffer;
    }
    bool IndexStagingBuffer::operator==(const IndexStagingBuffer& other) const
    {
        return other.m_DeviceBuffer == m_DeviceBuffer;
    }

private:
    ID3DIndexBuffer* m_DeviceBuffer{ nullptr };
    void* m_HostData{ nullptr };
    size_t m_Size{ 0 };
};
