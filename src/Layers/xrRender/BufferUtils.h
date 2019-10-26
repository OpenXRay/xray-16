#pragma once

u32 GetFVFVertexSize(u32 FVF);
u32 GetDeclVertexSize(const VertexElement* decl, DWORD Stream);
u32 GetDeclLength(const VertexElement* decl);

struct SDeclaration;

void ConvertVertexDeclaration(u32 FVF, SDeclaration* decl);
void ConvertVertexDeclaration(const VertexElement* dxdecl, SDeclaration* decl);
void ConvertVertexDeclaration(const xr_vector<VertexElement>& declIn, xr_vector<InputElementDesc>& declOut);

namespace BufferUtils
{
HRESULT CreateVertexBuffer(VertexBufferHandle* pBuffer, const void* pData, UINT DataSize, bool bImmutable = true);
HRESULT CreateIndexBuffer(IndexBufferHandle* pBuffer, const void* pData, UINT DataSize, bool bImmutable = true);
HRESULT CreateConstantBuffer(ConstantBufferHandle* ppBuffer, UINT DataSize);
};

class VertexStagingBuffer
{
public:
    VertexStagingBuffer();
    ~VertexStagingBuffer();

    void Create(size_t size, bool allowReadBack = false);
    bool IsValid() const;
    void* Map(size_t offset = 0, size_t size = 0, bool read = false);
    void Unmap();
    VertexBufferHandle GetBufferHandle() const;
    void Flush(); // Does unmap implicitly
    void DiscardHostBuffer();

    void AddRef()
    {
        ++m_RefCounter;
    }

    u32 Release()
    {
        VERIFY2(m_RefCounter, "Attempt to release unused object");
        --m_RefCounter;
        if (m_RefCounter == 0)
            Destroy();
        return m_RefCounter;
    }

    operator VertexBufferHandle() const
    {
        return m_DeviceBuffer;
    }

    bool VertexStagingBuffer::operator==(VertexBufferHandle other) const
    {
        return other == m_DeviceBuffer;
    }
    bool VertexStagingBuffer::operator==(const VertexStagingBuffer& other) const
    {
        return other.m_DeviceBuffer == m_DeviceBuffer;
    }

    size_t GetSystemMemoryUsage() const;
    size_t GetVideoMemoryUsage() const;

private:
    void Destroy();

    VertexBufferHandle m_DeviceBuffer;
    HostBufferHandle m_HostBuffer;
    size_t m_Size{ 0 };
    u32 m_RefCounter{ 0 };
    bool m_AllowReadBack{ false }; // specifies whether host will want to have the data back (e.g. skinning code)
};

class IndexStagingBuffer
{
public:
    IndexStagingBuffer();
    ~IndexStagingBuffer();

    void Create(size_t size, bool allowReadBack = false);
    bool IsValid() const;
    void* Map(size_t offset = 0, size_t size = 0, bool read = false);
    void Unmap();
    IndexBufferHandle GetBufferHandle() const;
    void Flush(); // Does unmap implicitly
    void DiscardHostBuffer();

    void AddRef()
    {
        ++m_RefCounter;
    }

    u32 Release()
    {
        VERIFY2(m_RefCounter, "Attempt to release unused object");
        --m_RefCounter;
        if (m_RefCounter == 0)
            Destroy();
        return m_RefCounter;
    }

    operator IndexBufferHandle() const
    {
        return m_DeviceBuffer;
    }

    bool IndexStagingBuffer::operator==(IndexBufferHandle other) const
    {
        return other == m_DeviceBuffer;
    }
    bool IndexStagingBuffer::operator==(const IndexStagingBuffer& other) const
    {
        return other.m_DeviceBuffer == m_DeviceBuffer;
    }

    size_t GetSystemMemoryUsage() const;
    size_t GetVideoMemoryUsage() const;

private:
    void Destroy();

    IndexBufferHandle m_DeviceBuffer;
    HostBufferHandle m_HostBuffer;
    size_t m_Size{ 0 };
    u32 m_RefCounter{ 0 };
    bool m_AllowReadBack{ false }; // specifies whether host will want to have the data back (e.g. skinning code)
};
