#pragma once

#ifdef USE_OGL

struct SDeclaration;

GLsizei GetFVFVertexSize(u32 FVF);
GLsizei GetDeclVertexSize(const D3DVERTEXELEMENT9* decl, DWORD Stream);
u32 GetDeclLength(const D3DVERTEXELEMENT9* decl);
void ConvertVertexDeclaration(u32 FVF, SDeclaration* decl);
void ConvertVertexDeclaration(const D3DVERTEXELEMENT9* dxdecl, SDeclaration* decl);

namespace glBufferUtils
{
void CreateVertexBuffer(GLuint* pBuffer, const void* pData, UINT DataSize, bool bImmutable = true);
void CreateIndexBuffer(GLuint* pBuffer, const void* pData, UINT DataSize, bool bImmutable = true);
};

class VertexStagingBuffer
{
public:
    void Create(size_t size);
    void Destroy();

    void* GetHostPointer() const;
    GLuint GetBufferHandle() const;
    void Flush();

    operator GLuint() const
    {
        return m_DeviceBuffer;
    }

    bool VertexStagingBuffer::operator==(GLuint other) const
    {
        return other == m_DeviceBuffer;
    }
    bool VertexStagingBuffer::operator==(const VertexStagingBuffer& other) const
    {
        return other.m_DeviceBuffer == m_DeviceBuffer;
    }

private:
    GLuint m_DeviceBuffer{ 0 };
    void* m_HostData{ nullptr };
    size_t m_Size{ 0 };
};

class IndexStagingBuffer
{
public:
    void Create(size_t size);
    void Destroy();

    void* GetHostPointer() const;
    GLuint GetBufferHandle() const;
    void Flush();

    operator GLuint() const
    {
        return m_DeviceBuffer;
    }

    bool IndexStagingBuffer::operator==(GLuint other) const
    {
        return other == m_DeviceBuffer;
    }
    bool IndexStagingBuffer::operator==(const IndexStagingBuffer& other) const
    {
        return other.m_DeviceBuffer == m_DeviceBuffer;
    }

private:
    GLuint m_DeviceBuffer{ 0 };
    void* m_HostData{ nullptr };
    size_t m_Size{ 0 };
};

#endif // USE_OGL
