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

    void Create(size_t size);
    void Destroy();

    bool IsValid() const;
    void* GetHostPointer();
    VertexBufferHandle GetBufferHandle() const;
    void Flush();

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

private:
    VertexBufferHandle m_DeviceBuffer;
    void* m_HostData{ nullptr };
    size_t m_Size{ 0 };
};

class IndexStagingBuffer
{
public:
    IndexStagingBuffer();
    ~IndexStagingBuffer();

    void Create(size_t size);
    void Destroy();

    bool IsValid() const;
    void* GetHostPointer();
    IndexBufferHandle GetBufferHandle() const;
    void Flush();

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

private:
    IndexBufferHandle m_DeviceBuffer;
    void* m_HostData{ nullptr };
    size_t m_Size{ 0 };
};
