#include "stdafx.h"
#include "Layers/xrRender/BufferUtils.h"

#include <FlexibleVertexFormat.h>

namespace xray::render::RENDER_NAMESPACE
{
#ifdef XR_PLATFORM_WEB
enum
{
    LOCKFLAGS_FLUSH  = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT,
    LOCKFLAGS_APPEND = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT,
};
#else
enum
{
    LOCKFLAGS_FLUSH  = GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_INVALIDATE_BUFFER_BIT,
    LOCKFLAGS_APPEND = GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT, // TODO: Implement buffer object appending using glBufferSubData
};
#endif

u32 GetFVFVertexSize(u32 FVF)
{
    return static_cast<u32>(::FVF::ComputeVertexSize(FVF));
}

u32 GetDeclVertexSize(const VertexElement* decl, u32 Stream)
{
    return static_cast<u32>(::FVF::ComputeVertexSize(decl, Stream));
}

u32 GetDeclLength(const VertexElement* decl)
{
    return static_cast<u32>(::FVF::GetDeclLength(decl));
}

static HRESULT CreateBuffer(GLuint* pBuffer, const void* pData, u32 dataSize, bool bDynamic, bool bIndexBuffer)
{
    const GLenum usage = bDynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
    const GLenum target = bIndexBuffer ? GL_ELEMENT_ARRAY_BUFFER : GL_ARRAY_BUFFER;

    glGenBuffers(1, pBuffer);
#ifdef XR_PLATFORM_WEB
    if (bIndexBuffer)
        BindVertexArray(0);
#endif
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
    3, // D3DDECLUSAGE_POSITIONT
    0, // D3DDECLUSAGE_COLOR
    7, // D3DDECLUSAGE_FOG
    ~0u, // D3DDECLUSAGE_DEPTH
    ~0u, // D3DDECLUSAGE_SAMPLE
};

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

        if (location == GLuint(~0u))
            continue; // Unsupported

        location += desc.UsageIndex;
        callback(location, size, type, normalized, desc.Offset, desc.Stream);
    }
}

void SetVertexDeclaration(const VertexElement* dxdecl, u32 baseOffset)
{
    auto stride = GetDeclVertexSize(dxdecl, 0);
    IterVertexDeclaration(dxdecl,
    [&](GLuint location, GLint size, GLenum type, GLboolean normalized, intptr_t offset, GLuint /*stream*/)
    {
        CHK_GL(glVertexAttribPointer(
            location, size, type, normalized, stride, (void*)(offset + baseOffset)));
    });
}

void ConvertVertexDeclaration(const VertexElement* dxdecl, SDeclaration* decl)
{
    RCache.set_Format(decl);
    IterVertexDeclaration(dxdecl,
    [](GLuint location, GLint size, GLenum type, GLboolean normalized, GLuint offset, GLuint stream)
    {
        CHK_GL(glEnableVertexAttribArray(location));
        if (GLAD_GL_ARB_vertex_attrib_binding)
        {
            CHK_GL(glVertexAttribFormat(location, size, type, normalized, offset));
            CHK_GL(glVertexAttribBinding(location, stream));
        }
    });
}

void SetGLVertexPointer(SDeclaration* decl, u32 baseOffset)
{
    SetVertexDeclaration(decl->dcl_code.data(), baseOffset);
}

#ifdef XR_PLATFORM_WEB
namespace
{
struct VertexArrayKey
{
    const SDeclaration* decl;
    GLuint vb;
    GLuint ib;
    u32 stride;
    u32 baseVertex;

    bool operator==(const VertexArrayKey& other) const = default;
};

struct VertexArrayKeyHash
{
    size_t operator()(const VertexArrayKey& key) const
    {
        size_t hash = std::hash<const void*>()(key.decl);
        for (const u32 value : { key.vb, key.ib, key.stride, key.baseVertex })
            hash = hash * 31 + value;
        return hash;
    }
};

xr_unordered_map<VertexArrayKey, GLuint, VertexArrayKeyHash> vertexArrays;
xr_vector<GLuint> streamVertexBuffers;

template <typename Predicate>
void ForgetVertexArraysIf(Predicate&& shouldForget)
{
    for (auto it = vertexArrays.begin(); it != vertexArrays.end();)
    {
        if (shouldForget(it->first))
        {
            if (g_boundVertexArray == it->second)
                g_boundVertexArray = 0;
            glDeleteVertexArrays(1, &it->second);
            it = vertexArrays.erase(it);
        }
        else
            ++it;
    }
}
} // namespace

GLuint GetVertexArray(SDeclaration* decl, u32 baseVertex)
{
    const VertexArrayKey key{ decl, decl->bound_vb, decl->bound_ib, decl->bound_stride, baseVertex };
    const auto [it, created] = vertexArrays.try_emplace(key, 0);
    if (!created)
        return it->second;

    glGenVertexArrays(1, &it->second);
    glBindVertexArray(it->second);
    g_boundVertexArray = it->second;
    glBindBuffer(GL_ARRAY_BUFFER, key.vb);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, key.ib);
    IterVertexDeclaration(decl->dcl_code.data(),
    [](GLuint location, GLint, GLenum, GLboolean, intptr_t, GLuint)
    {
        CHK_GL(glEnableVertexAttribArray(location));
    });
    SetVertexDeclaration(decl->dcl_code.data(), baseVertex * key.stride);
    return it->second;
}

bool IsStreamVertexBuffer(GLuint buffer)
{
    return std::find(streamVertexBuffers.begin(), streamVertexBuffers.end(), buffer) != streamVertexBuffers.end();
}

void ForgetVertexArrays(GLuint buffer)
{
    ForgetVertexArraysIf([buffer](const VertexArrayKey& key) { return key.vb == buffer || key.ib == buffer; });
}

void ForgetVertexArrays(const SDeclaration* decl)
{
    ForgetVertexArraysIf([decl](const VertexArrayKey& key) { return key.decl == decl; });
}
#endif

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
#ifdef XR_PLATFORM_WEB
        ForgetVertexArrays(m_DeviceBuffer);
#endif
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
#ifdef XR_PLATFORM_WEB
        ForgetVertexArrays(m_DeviceBuffer);
#endif
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
#ifdef XR_PLATFORM_WEB
    glBindBuffer(GL_COPY_READ_BUFFER, m_DeviceBuffer);
    CHK_GL(glGetBufferParameteriv(GL_COPY_READ_BUFFER, GL_BUFFER_SIZE, &bufferSize));
#else
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_DeviceBuffer);
    CHK_GL(glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize));
#endif
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
#ifdef XR_PLATFORM_WEB
    streamVertexBuffers.push_back(m_DeviceBuffer);
    m_HostBuffer.resize(size);
#endif
    AddRef();
}

void VertexStreamBuffer::Destroy()
{
    if (m_DeviceBuffer == 0)
        return;

#ifdef XR_PLATFORM_WEB
    ForgetVertexArrays(m_DeviceBuffer);
    streamVertexBuffers.erase(std::find(streamVertexBuffers.begin(), streamVertexBuffers.end(), m_DeviceBuffer));
#endif
    glDeleteBuffers(1, &m_DeviceBuffer);
    m_DeviceBuffer = 0;
}

#ifdef XR_PLATFORM_WEB
void* VertexStreamBuffer::Map(size_t offset, size_t size, bool flush /*= false*/)
{
    VERIFY(m_DeviceBuffer);
    VERIFY(offset == 0 && size <= m_HostBuffer.size());
    UNUSED(flush);
    m_MappedOffset = offset;
    m_MappedSize = size;
    return m_HostBuffer.data() + offset;
}

void VertexStreamBuffer::Unmap()
{
    Unmap(m_MappedSize);
}

void VertexStreamBuffer::Unmap(size_t writtenSize)
{
    VERIFY(m_DeviceBuffer && writtenSize <= m_MappedSize);
    if (writtenSize == 0)
        return;
    glBindBuffer(GL_ARRAY_BUFFER, m_DeviceBuffer);
    CHK_GL(glBufferData(GL_ARRAY_BUFFER, writtenSize, m_HostBuffer.data() + m_MappedOffset, GL_STREAM_DRAW));
}
#else
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
#endif

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
#ifdef XR_PLATFORM_WEB
    m_HostBuffer.resize(size);
#endif
    AddRef();
}

void IndexStreamBuffer::Destroy()
{
    if (m_DeviceBuffer == 0)
        return;

#ifdef XR_PLATFORM_WEB
    ForgetVertexArrays(m_DeviceBuffer);
#endif
    glDeleteBuffers(1, &m_DeviceBuffer);
    m_DeviceBuffer = 0;
}

#ifdef XR_PLATFORM_WEB
constexpr GLenum INDEX_UPLOAD_TARGET = GL_COPY_WRITE_BUFFER;
#else
constexpr GLenum INDEX_UPLOAD_TARGET = GL_ELEMENT_ARRAY_BUFFER;
#endif

#ifdef XR_PLATFORM_WEB
void* IndexStreamBuffer::Map(size_t offset, size_t size, bool flush /*= false*/)
{
    VERIFY(m_DeviceBuffer);
    VERIFY(offset + size <= m_HostBuffer.size());
    if (flush)
    {
        glBindBuffer(INDEX_UPLOAD_TARGET, m_DeviceBuffer);
        CHK_GL(glBufferData(INDEX_UPLOAD_TARGET, m_HostBuffer.size(), nullptr, GL_DYNAMIC_DRAW));
    }
    m_MappedOffset = offset;
    m_MappedSize = size;
    return m_HostBuffer.data() + offset;
}

void IndexStreamBuffer::Unmap()
{
    Unmap(m_MappedSize);
}

void IndexStreamBuffer::Unmap(size_t writtenSize)
{
    VERIFY(m_DeviceBuffer && writtenSize <= m_MappedSize);
    if (writtenSize == 0)
        return;
    glBindBuffer(INDEX_UPLOAD_TARGET, m_DeviceBuffer);
    CHK_GL(glBufferSubData(INDEX_UPLOAD_TARGET, m_MappedOffset, writtenSize, m_HostBuffer.data() + m_MappedOffset));
}
#else
void* IndexStreamBuffer::Map(size_t offset, size_t size, bool flush /*= false*/)
{
    VERIFY(m_DeviceBuffer);
    glBindBuffer(INDEX_UPLOAD_TARGET, m_DeviceBuffer);

    void *pData = nullptr;
    const auto flags = flush ? LOCKFLAGS_FLUSH : LOCKFLAGS_APPEND;
    CHK_GL(pData = (void*)glMapBufferRange(
        INDEX_UPLOAD_TARGET,
        offset,
        size,
        flags));

    return pData;
}

void IndexStreamBuffer::Unmap()
{
    VERIFY(m_DeviceBuffer);
    glBindBuffer(INDEX_UPLOAD_TARGET, m_DeviceBuffer);
    CHK_GL(glUnmapBuffer(INDEX_UPLOAD_TARGET));
}
#endif

bool IndexStreamBuffer::IsValid() const
{
    return !!m_DeviceBuffer;
}
} // namespace xray::render::RENDER_NAMESPACE
