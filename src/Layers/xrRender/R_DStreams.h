#pragma once

#include "BufferUtils.h"

class ECORE_API _VertexStream
{
    VertexStreamBuffer pVB;
    u32 mSize{}; // size in bytes
    u32 mPosition{}; // position in bytes
    u32 mDiscardID{}; // ID of discard - usually for caching
#ifdef DEBUG
    u32 dbg_lock{};
#endif

public:
    VertexBufferHandle old_pVB{};

public:
    _VertexStream() = default;
    ~_VertexStream() = default;

    void Create();
    void Destroy();
    void reset_begin();
    void reset_end();

    VertexBufferHandle Buffer() const { return pVB; }
    u32 DiscardID() const { return mDiscardID; }
    void Flush() { mPosition = mSize; }
    void* Lock(u32 vl_Count, u32 Stride, u32& vOffset);
    void Unlock(u32 Count, u32 Stride);
    u32 GetSize() const { return mSize; }

private:
    void _clear()
    {
        mSize = 0;
        mPosition = 0;
        mDiscardID = 0;
#ifdef DEBUG
        dbg_lock = 0;
#endif
    }
};

class ECORE_API _IndexStream
{
    IndexStreamBuffer pIB;
    u32 mSize{}; // real size (usually mCount, aligned on 512b boundary)
    u32 mPosition{};
    u32 mDiscardID{};

public:
    IndexBufferHandle old_pIB{};

public:
    _IndexStream() = default;
    ~_IndexStream() = default;

    void Create();
    void Destroy();
    void reset_begin();
    void reset_end();

    IndexBufferHandle Buffer() const { return pIB; }
    u32 DiscardID() const { return mDiscardID; }
    void Flush() { mPosition = mSize; }
    u16* Lock(u32 Count, u32& vOffset);
    void Unlock(u32 RealCount);
    u32 GetSize() const { return mSize; }

private:
    void _clear()
    {
        mSize = 0;
        mPosition = 0;
        mDiscardID = 0;
    }
};
