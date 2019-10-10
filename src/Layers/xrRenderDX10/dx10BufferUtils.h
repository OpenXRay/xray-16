#ifndef dx10BufferUtils_included
#define dx10BufferUtils_included
#pragma once
#if defined(USE_DX10) || defined(USE_DX11)

namespace dx10BufferUtils
{
HRESULT CreateVertexBuffer(ID3DVertexBuffer** ppBuffer, const void* pData, UINT DataSize, bool bImmutable = true);
HRESULT CreateIndexBuffer(ID3DIndexBuffer** ppBuffer, const void* pData, UINT DataSize, bool bImmutable = true);
HRESULT CreateConstantBuffer(ID3DBuffer** ppBuffer, UINT DataSize);
void ConvertVertexDeclaration(const xr_vector<D3DVERTEXELEMENT9>& declIn, xr_vector<D3D_INPUT_ELEMENT_DESC>& declOut);
};

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

#endif //	USE_DX10
#endif //	dx10BufferUtils_included
