#pragma once

u32 GetFVFVertexSize(u32 FVF);
u32 GetDeclVertexSize(const D3DVERTEXELEMENT9* decl, DWORD Stream);
u32 GetDeclLength(const D3DVERTEXELEMENT9* decl);

class VertexStagingBuffer
{
public:
    void Create(size_t size);
    void Destroy();

    void* GetHostPointer() const;
    ID3DVertexBuffer* GetBufferHandle() const;
    void Flush();

    operator ID3DVertexBuffer*() const
    {
        return m_DeviceBuffer;
    }

    bool VertexStagingBuffer::operator==(ID3DVertexBuffer* other) const
    {
        return other == m_DeviceBuffer;
    }
    bool VertexStagingBuffer::operator==(const VertexStagingBuffer& other) const
    {
        return other.m_DeviceBuffer == m_DeviceBuffer;
    }

private:
    ID3DVertexBuffer* m_DeviceBuffer{ nullptr };
    void* m_HostData{ nullptr };
    size_t m_Size{ 0 };
};

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
