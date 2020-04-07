#pragma once

#include "BufferUtils.h"

class ECORE_API _VertexStream
{
private:
    VertexStreamBuffer pVB;
    u32 mSize; // size in bytes
    u32 mPosition; // position in bytes
    u32 mDiscardID; // ID of discard - usually for caching
public:
    VertexBufferHandle old_pVB;
#ifdef DEBUG
    u32 dbg_lock;
#endif
private:
    void _clear();

public:
    void Create();
    void Destroy();
    void reset_begin();
    void reset_end();

    VertexBufferHandle Buffer() { return pVB; }
    u32 DiscardID() { return mDiscardID; }
    void Flush() { mPosition = mSize; }
    void* Lock(u32 vl_Count, u32 Stride, u32& vOffset);
    void Unlock(u32 Count, u32 Stride);
    u32 GetSize() { return mSize; }
    _VertexStream();
    ~_VertexStream() { _clear(); };
};

class ECORE_API _IndexStream
{
private:
    IndexStreamBuffer pIB;
    u32 mSize; // real size (usually mCount, aligned on 512b boundary)
    u32 mPosition;
    u32 mDiscardID;

public:
    IndexBufferHandle old_pIB;

private:
    void _clear()
    {
        mSize = 0;
        mPosition = 0;
        mDiscardID = 0;
    }

public:
    void Create();
    void Destroy();
    void reset_begin();
    void reset_end();

    IndexBufferHandle Buffer() { return pIB; }
    u32 DiscardID() { return mDiscardID; }
    void Flush() { mPosition = mSize; }
    u16* Lock(u32 Count, u32& vOffset);
    void Unlock(u32 RealCount);

    _IndexStream() { _clear(); };
    ~_IndexStream() { _clear(); };
};
