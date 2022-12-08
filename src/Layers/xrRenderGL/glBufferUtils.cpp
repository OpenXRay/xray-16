#include "stdafx.h"
#include "Layers/xrRender/BufferUtils.h"

enum
{
    LOCKFLAGS_FLUSH  = GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_INVALIDATE_BUFFER_BIT,
    LOCKFLAGS_APPEND = GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT, // TODO: Implement buffer object appending using glBufferSubData
};

static HRESULT CreateBuffer(GLuint* pBuffer, const void* pData, u32 dataSize, bool bDynamic, bool bIndexBuffer)
{
    const GLenum usage = bDynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
    const GLenum target = bIndexBuffer ? GL_ELEMENT_ARRAY_BUFFER : GL_ARRAY_BUFFER;

    glGenBuffers(1, pBuffer);
    glBindBuffer(target, *pBuffer);
    CHK_GL(glBufferData(target, dataSize, pData, usage));
    return S_OK;
}

static inline HRESULT CreateVertexBuffer(VertexBufferHandle* pBuffer, const void* pData, u32 dataSize, bool bDynamic)
{
    return CreateBuffer(pBuffer, pData, dataSize, bDynamic, false);
}

static inline HRESULT CreateIndexBuffer(IndexBufferHandle* pBuffer, const void* pData, u32 dataSize, bool bDynamic)
{
    return CreateBuffer(static_cast<GLuint*>(pBuffer), pData, dataSize, bDynamic, true);
}

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

u32 GetDeclVertexSize(const VertexElement* decl, u32 Stream)
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

template <typename F>
void IterVertexDeclaration(const VertexElement* dxdecl, F&& callback)
{
    // XXX: tamlin: use 'stride', or drop it.
    // GLsizei stride = GetDeclVertexSize(dxdecl, 0);
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
        callback(location, size, type, normalized, desc.Offset, desc.Stream);
    }
}

void SetVertexDeclaration(const VertexElement* dxdecl)
{
    auto stride = GetDeclVertexSize(dxdecl, 0);
    IterVertexDeclaration(dxdecl,
    [&](GLuint location, GLint size, GLenum type, GLboolean normalized, intptr_t offset, GLuint /*stream*/) 
    {
        CHK_GL(glVertexAttribPointer(
            location, size, type, normalized, stride, (void*)offset));
    });
}

void ConvertVertexDeclaration(const VertexElement* dxdecl, SDeclaration* decl)
{
    RCache.set_Format(decl);
    IterVertexDeclaration(dxdecl,
    [](GLuint location, GLint size, GLenum type, GLboolean normalized, GLuint offset, GLuint stream) 
    {
        CHK_GL(glEnableVertexAttribArray(location));
        if (GLEW_ARB_vertex_attrib_binding)
        {
            CHK_GL(glVertexAttribFormat(location, size, type, normalized, offset));
            CHK_GL(glVertexAttribBinding(location, stream));
        }
    });
}

template <typename F>
void IterVertexDeclaration(u32 FVF, F&& callback)
{
    u32 offset = 0;

    // Position attribute
    if (FVF & D3DFVF_XYZRHW)
    {
        GLuint attrib = VertexUsageList[D3DDECLUSAGE_POSITION];
        callback(attrib, 4, GL_FLOAT, GL_FALSE, offset, 0);
        offset += sizeof(Fvector4);
    }
    else if (FVF & D3DFVF_XYZ)
    {
        GLuint attrib = VertexUsageList[D3DDECLUSAGE_POSITION];
        callback(attrib, 3, GL_FLOAT, GL_FALSE, offset, 0);
        offset += sizeof(Fvector);
    }

    // Diffuse color attribute
    if (FVF & D3DFVF_DIFFUSE)
    {
        GLuint attrib = VertexUsageList[D3DDECLUSAGE_COLOR];
        callback(attrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, offset, 0);
        offset += sizeof(u32);
    }

    // Specular color attribute
    if (FVF & D3DFVF_SPECULAR)
    {
        GLuint attrib = VertexUsageList[D3DDECLUSAGE_COLOR] + 1;
        callback(attrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, offset, 0);
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

        callback(attrib, size, GL_FLOAT, GL_FALSE, offset, 0);
        offset += size * sizeof(float);
    }

    VERIFY(offset == GetFVFVertexSize(FVF));
}

void SetVertexDeclaration(u32 FVF)
{
    auto stride = GetFVFVertexSize(FVF);
    IterVertexDeclaration(FVF,
    [&](GLuint location, GLint size, GLenum type, GLboolean normalized, intptr_t offset, GLuint /*stream*/) 
    {
        CHK_GL(glVertexAttribPointer(
            location, size, type, normalized, stride, reinterpret_cast<void*>(offset)));
    });
}

void ConvertVertexDeclaration(u32 FVF, SDeclaration* decl)
{
    RCache.set_Format(decl);
    IterVertexDeclaration(FVF,
    [](GLuint location, GLint size, GLenum type, GLboolean normalized, GLuint offset, GLuint stream) 
    {
        CHK_GL(glEnableVertexAttribArray(location));
        if (GLEW_ARB_vertex_attrib_binding)
        {
            CHK_GL(glVertexAttribFormat(location, size, type, normalized, offset));
            CHK_GL(glVertexAttribBinding(location, stream));
        }
    });
}

void SetGLVertexPointer(SDeclaration* decl) 
{
    if (!decl->dcl_code.empty()) 
        SetVertexDeclaration(&*decl->dcl_code.begin());
    else
        SetVertexDeclaration(decl->FVF);
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

u32 GetDeclLength(const D3DVERTEXELEMENT9* decl)
{
    const D3DVERTEXELEMENT9* element;

    for (element = decl; element->Stream != 0xff; ++element);

    return element - decl;
}

//-----------------------------------------------------------------------------
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

    // Upload data to the device
    CreateVertexBuffer(&m_DeviceBuffer, m_HostBuffer, m_Size, false);

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

    // Upload data to the device
    CreateIndexBuffer(&m_DeviceBuffer, m_HostBuffer, m_Size, false);

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

//-----------------------------------------------------------------------------
VertexStreamBuffer::~VertexStreamBuffer()
{
    Destroy();
}

void VertexStreamBuffer::Create(size_t size)
{
    CreateVertexBuffer(&m_DeviceBuffer, nullptr, size, true);
    AddRef();
}

void VertexStreamBuffer::Destroy()
{
    if (m_DeviceBuffer == 0)
        return;

    glDeleteBuffers(1, &m_DeviceBuffer);
}

void* VertexStreamBuffer::Map(size_t offset, size_t size, bool flush /*= false*/)
{
    VERIFY(m_DeviceBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_DeviceBuffer);

    void *pData = nullptr;
    const auto flags = flush ? LOCKFLAGS_FLUSH : LOCKFLAGS_APPEND;
    CHK_GL(pData = (void*)glMapBufferRange(
        GL_ARRAY_BUFFER,
        offset,
        size,
        flags));

    return pData;
}

void VertexStreamBuffer::Unmap()
{
    VERIFY(m_DeviceBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_DeviceBuffer);
    CHK_GL(glUnmapBuffer(GL_ARRAY_BUFFER));
}

bool VertexStreamBuffer::IsValid() const
{
    return !!m_DeviceBuffer;
}

//-----------------------------------------------------------------------------
IndexStreamBuffer::~IndexStreamBuffer()
{
    Destroy();
}

void IndexStreamBuffer::Create(size_t size)
{
    CreateIndexBuffer(&m_DeviceBuffer, nullptr, size, true);
    AddRef();
}

void IndexStreamBuffer::Destroy()
{
    if (m_DeviceBuffer == 0)
        return;

    glDeleteBuffers(1, &m_DeviceBuffer);
}

void* IndexStreamBuffer::Map(size_t offset, size_t size, bool flush /*= false*/)
{
    VERIFY(m_DeviceBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_DeviceBuffer);

    void *pData = nullptr;
    const auto flags = flush ? LOCKFLAGS_FLUSH : LOCKFLAGS_APPEND;
    CHK_GL(pData = (void*)glMapBufferRange(
        GL_ELEMENT_ARRAY_BUFFER,
        offset,
        size,
        flags));

    return pData;
}

void IndexStreamBuffer::Unmap()
{
    VERIFY(m_DeviceBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_DeviceBuffer);
    CHK_GL(glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER));
}

bool IndexStreamBuffer::IsValid() const
{
    return !!m_DeviceBuffer;
}
