#include "stdafx.h"
#include "Layers/xrRender/BufferUtils.h"

namespace BufferUtils
{
HRESULT CreateBuffer(GLuint* pBuffer, const void* pData, UINT DataSize, bool bImmutable, bool bIndexBuffer)
{
    GLenum usage = bImmutable ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW;
    GLenum target = bIndexBuffer ? GL_ELEMENT_ARRAY_BUFFER : GL_ARRAY_BUFFER;

    glGenBuffers(1, pBuffer);
    glBindBuffer(target, *pBuffer);
    CHK_GL(glBufferData(target, DataSize, pData, usage));
    return S_OK;
}

HRESULT CreateVertexBuffer(VertexBufferHandle* pBuffer, const void* pData, UINT DataSize, bool bImmutable)
{
    return CreateBuffer(pBuffer, pData, DataSize, bImmutable, false);
}

HRESULT CreateIndexBuffer(IndexBufferHandle* pBuffer, const void* pData, UINT DataSize, bool bImmutable)
{
    return CreateBuffer(static_cast<GLuint*>(pBuffer), pData, DataSize, bImmutable, true);
}
} // namespace glBufferUtils

const GLsizei VertexSizeList[] =
{
    1, // D3DDECLTYPE_FLOAT1
    2, // D3DDECLTYPE_FLOAT2
    3, // D3DDECLTYPE_FLOAT3
    4, // D3DDECLTYPE_FLOAT4
    4, // D3DDECLTYPE_D3DCOLOR
    4, // D3DDECLTYPE_UBYTE4
    2, // D3DDECLTYPE_SHORT2
    4, // D3DDECLTYPE_SHORT4
    4, // D3DDECLTYPE_UBYTE4N
    2, // D3DDECLTYPE_SHORT2N
    4, // D3DDECLTYPE_SHORT4N
    2, // D3DDECLTYPE_USHORT2N
    4, // D3DDECLTYPE_USHORT4N
    1, // D3DDECLTYPE_UDEC3
    1, // D3DDECLTYPE_DEC3N
    2, // D3DDECLTYPE_FLOAT16_2
    4 // D3DDECLTYPE_FLOAT16_4
};

const GLenum VertexTypeList[] =
{
    GL_FLOAT, // D3DDECLTYPE_FLOAT1
    GL_FLOAT, // D3DDECLTYPE_FLOAT2
    GL_FLOAT, // D3DDECLTYPE_FLOAT3
    GL_FLOAT, // D3DDECLTYPE_FLOAT4
    GL_UNSIGNED_BYTE, // D3DDECLTYPE_D3DCOLOR
    GL_UNSIGNED_BYTE, // D3DDECLTYPE_UBYTE4
    GL_SHORT, // D3DDECLTYPE_SHORT2
    GL_SHORT, // D3DDECLTYPE_SHORT4
    GL_UNSIGNED_BYTE, // D3DDECLTYPE_UBYTE4N
    GL_SHORT, // D3DDECLTYPE_SHORT2N
    GL_SHORT, // D3DDECLTYPE_SHORT4N
    GL_UNSIGNED_SHORT, // D3DDECLTYPE_USHORT2N
    GL_UNSIGNED_SHORT, // D3DDECLTYPE_USHORT4N
    GL_INT_2_10_10_10_REV, // D3DDECLTYPE_UDEC3
    GL_INT_2_10_10_10_REV, // D3DDECLTYPE_DEC3N
    GL_HALF_FLOAT, // D3DDECLTYPE_FLOAT16_2
    GL_HALF_FLOAT // D3DDECLTYPE_FLOAT16_4
};

const GLboolean VertexNormalizedList[] =
{
    GL_FALSE, // D3DDECLTYPE_FLOAT1
    GL_FALSE, // D3DDECLTYPE_FLOAT2
    GL_FALSE, // D3DDECLTYPE_FLOAT3
    GL_FALSE, // D3DDECLTYPE_FLOAT4
    GL_TRUE, // D3DDECLTYPE_D3DCOLOR
    GL_FALSE, // D3DDECLTYPE_UBYTE4
    GL_FALSE, // D3DDECLTYPE_SHORT2
    GL_FALSE, // D3DDECLTYPE_SHORT4
    GL_TRUE, // D3DDECLTYPE_UBYTE4N
    GL_TRUE, // D3DDECLTYPE_SHORT2N
    GL_TRUE, // D3DDECLTYPE_SHORT4N
    GL_TRUE, // D3DDECLTYPE_USHORT2N
    GL_TRUE, // D3DDECLTYPE_USHORT4N
    GL_FALSE, // D3DDECLTYPE_UDEC3
    GL_TRUE, // D3DDECLTYPE_DEC3N
    GL_FALSE, // D3DDECLTYPE_FLOAT16_2
    GL_FALSE // D3DDECLTYPE_FLOAT16_4
};

const GLsizei VertexTypeSizeList[] =
{
    sizeof(GLfloat), // D3DDECLTYPE_FLOAT1
    sizeof(GLfloat), // D3DDECLTYPE_FLOAT2
    sizeof(GLfloat), // D3DDECLTYPE_FLOAT3
    sizeof(GLfloat), // D3DDECLTYPE_FLOAT4
    sizeof(GLubyte), // D3DDECLTYPE_D3DCOLOR
    sizeof(GLubyte), // D3DDECLTYPE_UBYTE4
    sizeof(GLshort), // D3DDECLTYPE_SHORT2
    sizeof(GLshort), // D3DDECLTYPE_SHORT4
    sizeof(GLubyte), // D3DDECLTYPE_UBYTE4N
    sizeof(GLshort), // D3DDECLTYPE_SHORT2N
    sizeof(GLshort), // D3DDECLTYPE_SHORT4N
    sizeof(GLushort), // D3DDECLTYPE_USHORT2N
    sizeof(GLushort), // D3DDECLTYPE_USHORT4N
    sizeof(GLuint), // D3DDECLTYPE_UDEC3
    sizeof(GLint), // D3DDECLTYPE_DEC3N
    sizeof(GLhalf), // D3DDECLTYPE_FLOAT16_2
    sizeof(GLhalf) // D3DDECLTYPE_FLOAT16_4
};

const GLuint VertexUsageList[] =
{
    3, // D3DDECLUSAGE_POSITION
    ~0u, // D3DDECLUSAGE_BLENDWEIGHT
    ~0u, // D3DDECLUSAGE_BLENDINDICES
    5, // D3DDECLUSAGE_NORMAL
    ~0u, // D3DDECLUSAGE_PSIZE
    8, // D3DDECLUSAGE_TEXCOORD
    4, // D3DDECLUSAGE_TANGENT
    6, // D3DDECLUSAGE_BINORMAL
    ~0u, // D3DDECLUSAGE_TESSFACTOR
    ~0u, // D3DDECLUSAGE_POSITIONT
    0, // D3DDECLUSAGE_COLOR
    7, // D3DDECLUSAGE_FOG
    ~0u, // D3DDECLUSAGE_DEPTH
    ~0u, // D3DDECLUSAGE_SAMPLE
};

u32 GetDeclVertexSize(const VertexElement* decl, DWORD Stream)
{
    GLsizei size = 0;
    for (int i = 0; i < MAXD3DDECLLENGTH; ++i)
    {
        const D3DVERTEXELEMENT9& desc = decl[i];

        if (desc.Stream == 0xFF)
            break;

        size += VertexSizeList[desc.Type] * VertexTypeSizeList[desc.Type];
    }
    return size;
}

void ConvertVertexDeclaration(const VertexElement* dxdecl, SDeclaration* decl)
{
    RCache.set_Format(decl);

    // XXX: tamlin: use 'stride', or drop it.
    GLsizei stride = GetDeclVertexSize(dxdecl, 0);
    for (int i = 0; i < MAXD3DDECLLENGTH; ++i)
    {
        const D3DVERTEXELEMENT9& desc = dxdecl[i];

        if (desc.Stream == 0xFF)
            break;

        GLuint location = VertexUsageList[desc.Usage];
        GLint size = VertexSizeList[desc.Type];
        GLenum type = VertexTypeList[desc.Type];
        GLboolean normalized = VertexNormalizedList[desc.Type];

        if (location < 0)
            continue; // Unsupported

        location += desc.UsageIndex;
        CHK_GL(glVertexAttribFormat(location, size, type, normalized, desc.Offset));
        CHK_GL(glVertexAttribBinding(location, desc.Stream));
        CHK_GL(glEnableVertexAttribArray(location));
    }
}

u32 GetFVFVertexSize(u32 FVF)
{
    GLsizei offset = 0;

    // Position attribute
    if (FVF & D3DFVF_XYZRHW)
        offset += sizeof(Fvector4);
    else if (FVF & D3DFVF_XYZ)
        offset += sizeof(Fvector);

    // Diffuse color attribute
    if (FVF & D3DFVF_DIFFUSE)
        offset += sizeof(u32);

    // Specular color attribute
    if (FVF & D3DFVF_SPECULAR)
        offset += sizeof(u32);

    // Texture coordinates
    for (u32 i = 0; i < (FVF & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT; i++)
    {
        u32 size = 2;
        if (FVF & D3DFVF_TEXCOORDSIZE1(i))
            size = 1;
        if (FVF & D3DFVF_TEXCOORDSIZE3(i))
            size = 3;
        if (FVF & D3DFVF_TEXCOORDSIZE4(i))
            size = 4;

        offset += size * sizeof(float);
    }

    return offset;
}

void ConvertVertexDeclaration(u32 FVF, SDeclaration* decl)
{
    RCache.set_Format(decl);

    GLsizei stride = GetFVFVertexSize(FVF);
    u32 offset = 0;

    // Position attribute
    if (FVF & D3DFVF_XYZRHW)
    {
        GLuint attrib = VertexUsageList[D3DDECLUSAGE_POSITION];
        CHK_GL(glVertexAttribFormat(attrib, 4, GL_FLOAT, GL_FALSE, offset));
        CHK_GL(glVertexAttribBinding(attrib, 0));
        CHK_GL(glEnableVertexAttribArray(attrib));
        offset += sizeof(Fvector4);
    }
    else if (FVF & D3DFVF_XYZ)
    {
        GLuint attrib = VertexUsageList[D3DDECLUSAGE_POSITION];
        CHK_GL(glVertexAttribFormat(attrib, 3, GL_FLOAT, GL_FALSE, offset));
        CHK_GL(glVertexAttribBinding(attrib, 0));
        CHK_GL(glEnableVertexAttribArray(attrib));
        offset += sizeof(Fvector);
    }

    // Diffuse color attribute
    if (FVF & D3DFVF_DIFFUSE)
    {
        GLuint attrib = VertexUsageList[D3DDECLUSAGE_COLOR];
        CHK_GL(glVertexAttribFormat(attrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, offset));
        CHK_GL(glVertexAttribBinding(attrib, 0));
        CHK_GL(glEnableVertexAttribArray(attrib));
        offset += sizeof(u32);
    }

    // Specular color attribute
    if (FVF & D3DFVF_SPECULAR)
    {
        GLuint attrib = VertexUsageList[D3DDECLUSAGE_COLOR] + 1;
        CHK_GL(glVertexAttribFormat(attrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, offset));
        CHK_GL(glVertexAttribBinding(attrib, 0));
        CHK_GL(glEnableVertexAttribArray(attrib));
        offset += sizeof(u32);
    }

    // Texture coordinates
    for (u32 i = 0; i < (FVF & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT; i++)
    {
        GLuint attrib = VertexUsageList[D3DDECLUSAGE_TEXCOORD] + i;

        u32 size = 2;
        if (FVF & D3DFVF_TEXCOORDSIZE1(i))
            size = 1;
        if (FVF & D3DFVF_TEXCOORDSIZE3(i))
            size = 3;
        if (FVF & D3DFVF_TEXCOORDSIZE4(i))
            size = 4;

        CHK_GL(glVertexAttribFormat(attrib, size, GL_FLOAT, GL_FALSE, offset));
        CHK_GL(glVertexAttribBinding(attrib, 0));
        CHK_GL(glEnableVertexAttribArray(attrib));
        offset += size * sizeof(float);
    }

    VERIFY(stride == offset);
}

u32 GetDeclLength(const D3DVERTEXELEMENT9* decl)
{
    const D3DVERTEXELEMENT9* element;

    for (element = decl; element->Stream != 0xff; ++element);

    return element - decl;
}

//-----------------------------------------------------------------------------
VertexStagingBuffer::VertexStagingBuffer()
    : m_DeviceBuffer{ 0 }
    , m_HostBuffer{ nullptr }
{
}

VertexStagingBuffer::~VertexStagingBuffer()
{
    Destroy();
}

void VertexStagingBuffer::Create(size_t size, bool allowReadBack /*= false*/)
{
    m_Size = size;
    m_AllowReadBack = allowReadBack;

    m_HostBuffer = xr_alloc<u8>(size);
    AddRef();
}

bool VertexStagingBuffer::IsValid() const
{
    return !!m_DeviceBuffer;
}

void* VertexStagingBuffer::Map(
    size_t offset /*= 0*/,
    size_t size /*= 0*/,
    bool read /*= false*/)
{
    VERIFY2(m_HostBuffer, "Buffer wasn't created or already discarded");
    VERIFY2(!read || m_AllowReadBack, "Can't read from write only buffer");
    VERIFY2((size + offset) <= m_Size, "Map region is too large");

    return static_cast<u8*>(m_HostBuffer) + offset;
}

void VertexStagingBuffer::Unmap(bool doFlush /*= false*/)
{
    if (!doFlush)
    {
        /* Do nothing*/
        return;
    }

    VERIFY2(!m_DeviceBuffer, "Attempting to upload buffer twice");
    VERIFY(m_HostBuffer && m_Size);

    // Upload data to device
    BufferUtils::CreateVertexBuffer(&m_DeviceBuffer, m_HostBuffer, m_Size, true);

    if (!m_AllowReadBack)
    {
        // Cache buffer isn't required anymore. Free host memory
        DiscardHostBuffer();
    }
}

VertexBufferHandle VertexStagingBuffer::GetBufferHandle() const
{
    return m_DeviceBuffer;
}

void VertexStagingBuffer::Destroy()
{
    DiscardHostBuffer();
    m_Size = 0;

    if (m_DeviceBuffer)
    {
        glDeleteBuffers(1, &m_DeviceBuffer);
        m_DeviceBuffer = 0;
    }
}

void VertexStagingBuffer::DiscardHostBuffer()
{
    if (m_HostBuffer)
        xr_free(m_HostBuffer);
}

size_t VertexStagingBuffer::GetSystemMemoryUsage() const
{
    return m_HostBuffer ? m_Size : 0;
}

size_t VertexStagingBuffer::GetVideoMemoryUsage() const
{
    if (!m_DeviceBuffer)
        return 0;

    GLint bufferSize;
    glBindBuffer(GL_ARRAY_BUFFER, m_DeviceBuffer);
    CHK_GL(glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize));
    return bufferSize;
}

//-----------------------------------------------------------------------------
IndexStagingBuffer::IndexStagingBuffer()
    : m_DeviceBuffer{ 0 }
    , m_HostBuffer{ nullptr }
{
}

IndexStagingBuffer::~IndexStagingBuffer()
{
    Destroy();
}

void IndexStagingBuffer::Create(size_t size, bool allowReadBack /*= false*/, bool /*managed = true*/)
{
    m_Size = size;
    m_AllowReadBack = allowReadBack;

    m_HostBuffer = xr_alloc<u8>(size);
    AddRef();
}

bool IndexStagingBuffer::IsValid() const
{
    return !!m_DeviceBuffer;
}

void* IndexStagingBuffer::Map(
    size_t offset /*= 0*/,
    size_t size /*= 0*/,
    bool read /*= false*/)
{
    VERIFY2(m_HostBuffer, "Buffer wasn't created or already discarded");
    VERIFY2(!read || m_AllowReadBack, "Can't read from write only buffer");
    VERIFY2((size + offset) <= m_Size, "Map region is too large");

    return static_cast<u8*>(m_HostBuffer) + offset;
}

void IndexStagingBuffer::Unmap(bool doFlush /*= false*/)
{
    if (!doFlush)
    {
        /* Do nothing*/
        return;
    }

    VERIFY2(!m_DeviceBuffer, "Attempting to upload buffer twice");
    VERIFY(m_HostBuffer && m_Size);

    // Upload data to device
    BufferUtils::CreateVertexBuffer(&m_DeviceBuffer, m_HostBuffer, m_Size, true);

    if (!m_AllowReadBack)
    {
        // Cache buffer isn't required anymore. Free host memory
        DiscardHostBuffer();
    }
}

IndexBufferHandle IndexStagingBuffer::GetBufferHandle() const
{
    return m_DeviceBuffer;
}

void IndexStagingBuffer::Destroy()
{
    DiscardHostBuffer();
    m_Size = 0;

    if (m_DeviceBuffer)
    {
        glDeleteBuffers(1, &m_DeviceBuffer);
        m_DeviceBuffer = 0;
    }
}

void IndexStagingBuffer::DiscardHostBuffer()
{
    if (m_HostBuffer)
        xr_free(m_HostBuffer);
}

size_t IndexStagingBuffer::GetSystemMemoryUsage() const
{
    return m_HostBuffer ? m_Size : 0;
}

size_t IndexStagingBuffer::GetVideoMemoryUsage() const
{
    if (!m_DeviceBuffer)
        return 0;

    GLint bufferSize;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_DeviceBuffer);
    CHK_GL(glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize));
    return bufferSize;
}
