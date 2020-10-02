#pragma once

u32 GetFVFVertexSize(u32 FVF);
u32 GetDeclVertexSize(const VertexElement* decl, u32 Stream);
u32 GetDeclLength(const VertexElement* decl);

struct SDeclaration;

void ConvertVertexDeclaration(u32 FVF, SDeclaration* decl);
void ConvertVertexDeclaration(const VertexElement* dxdecl, SDeclaration* decl);
void ConvertVertexDeclaration(const xr_vector<VertexElement>& declIn, xr_vector<InputElementDesc>& declOut);

namespace BufferUtils
{
HRESULT CreateConstantBuffer(ConstantBufferHandle* ppBuffer, u32 DataSize);
};

/**
 * Staging buffer abstraction provides a convenient way to upload vertex or index data into GPU memory.
 * It uses intermediate heap storage to handle host data which can be discarded or preserved for future
 * use in accordance to allocation policy.
 *
 * Two types of allocation available:
 * 1. Push once
 *    Such buffers can be mapped only for write and no data read back is available. Use this mode for cases
 *    when you have static geometry which you are going to upload only once and forget about it. To
 *    improve memory use the intermediate buffer will be discarded as soon as `Unmap()` called.
 * 2. Persistent
 *    In this case unmap operation doesn't discard heap storage content which can be accessed in order
 *    to read the data back. Mapping with read flag can be done multiple times but you won't be able to
 *    modify anything. The intermediate storage can be discarded explicitly to reclaim host memory.
 */

class VertexStagingBuffer
{
public:
    VertexStagingBuffer();
    ~VertexStagingBuffer();

    void Create(size_t size, bool allowReadBack = false);
    bool IsValid() const;
    void* Map(size_t offset = 0, size_t size = 0, bool read = false);
    void Unmap(bool doFlush = false);
    VertexBufferHandle GetBufferHandle() const;
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

    bool operator==(VertexBufferHandle other) const
    {
        return other == m_DeviceBuffer;
    }
    bool operator==(const VertexStagingBuffer& other) const
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

    void Create(size_t size, bool allowReadBack = false, bool managed = true);
    bool IsValid() const;
    void* Map(size_t offset = 0, size_t size = 0, bool read = false);
    void Unmap(bool doFlush = false);
    IndexBufferHandle GetBufferHandle() const;
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

    bool operator==(IndexBufferHandle other) const
    {
        return other == m_DeviceBuffer;
    }
    bool operator==(const IndexStagingBuffer& other) const
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

/**
 * Stream buffer is another abstraction for mutable GPU data. It uses mapping to provide access
 * to its content. The buffer data is considered to be write only.
 *
 * There are two modes of mapping available:
 * 1. Append (default)
 *    All previous data kept untouched but can't be overwritten.
 * 2. Flush
 *    Previous buffer content is discarded.
 */
class VertexStreamBuffer
{
public:
    VertexStreamBuffer();
    ~VertexStreamBuffer();

    void Create(size_t size);

    void* Map(size_t offset, size_t size, bool flush = false);
    void Unmap();
    bool IsValid() const;

    void AddRef()
    {
        ++m_RefCounter;
    }

    u32 Release()
    {
        VERIFY2(m_RefCounter, "Attempt to release unused object");
        --m_RefCounter;
        if (m_RefCounter == 0)
        {
            Destroy();
        }
        return m_RefCounter;
    }

    operator VertexBufferHandle() const
    {
        return m_DeviceBuffer;
    }

private:
    void Destroy();

    VertexBufferHandle m_DeviceBuffer;
    u32 m_RefCounter{};
};

class IndexStreamBuffer
{
public:
    IndexStreamBuffer();
    ~IndexStreamBuffer();

    void Create(size_t size);

    void* Map(size_t offset, size_t size, bool flush = false);
    void Unmap();
    bool IsValid() const;

    void AddRef()
    {
        ++m_RefCounter;
    }

    u32 Release()
    {
        VERIFY2(m_RefCounter, "Attempt to release unused object");
        --m_RefCounter;
        if (m_RefCounter == 0)
        {
            Destroy();
        }
        return m_RefCounter;
    }

    operator IndexBufferHandle() const
    {
        return m_DeviceBuffer;
    }

private:
    void Destroy();

    IndexBufferHandle m_DeviceBuffer;
    u32 m_RefCounter{};
};
