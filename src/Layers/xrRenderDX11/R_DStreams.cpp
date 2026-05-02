#include "stdafx.h"
#pragma hdrstop
#include "Layers/xrRenderDX11/ResourceManager.h"
#include "Layers/xrRender/R_DStreams.h"

namespace xray::render::fg
{

int rsDVB_Size = 4096;
int rsDIB_Size = 512;

#ifdef XR_PLATFORM_WEB
constexpr bool DiscardOnEveryLock = true;
#else
constexpr bool DiscardOnEveryLock = false;
#endif

void _VertexStream::Create()
{
    ZoneScoped;

    RImplementation.Resources->Evict();

    mSize = rsDVB_Size * 1024;
    pVB.Create(mSize);

    mPosition = 0;
    mDiscardID = 0;

    Msg("* DVB created: %dK", mSize / 1024);
}

void _VertexStream::Destroy()
{
    pVB.Release();
    _clear();
}

void* _VertexStream::Lock(u32 vl_Count, u32 Stride, u32& vOffset)
{
#ifdef DEBUG
    VERIFY(0 == dbg_lock);
    dbg_lock++;
#endif

    const u32 bytes_need = vl_Count * Stride;
    R_ASSERT2((bytes_need <= mSize) && vl_Count,
        make_string("bytes_need = %d, mSize = %d, vl_Count = %d", bytes_need, mSize, vl_Count));

    const u32 vl_mSize = mSize / Stride;
    const u32 vl_mPosition = mPosition / Stride + 1;

    bool bFlush = false;
    if (DiscardOnEveryLock || (vl_Count + vl_mPosition) >= vl_mSize)
    {
        mPosition = 0;
        vOffset = 0;
        mDiscardID++;
        bFlush = true;
    }
    else
    {
        mPosition = vl_mPosition * Stride;
        vOffset = vl_mPosition;
    }
    void* pData = pVB.Map(mPosition, bytes_need, bFlush);
    VERIFY(pData);
    return pData;
}

void _VertexStream::Unlock(u32 Count, u32 Stride)
{
#ifdef DEBUG
    VERIFY(1 == dbg_lock);
    dbg_lock--;
#endif
    mPosition += Count * Stride;

#ifdef XR_PLATFORM_WEB
    pVB.Unmap(Count * Stride);
#else
    pVB.Unmap();
#endif
}

void _VertexStream::reset_begin()
{
    old_pVB = pVB;
    Destroy();
}
void _VertexStream::reset_end()
{
    Create();
}

void _IndexStream::Create()
{
    RImplementation.Resources->Evict();

    mSize = rsDIB_Size * 1024;
    pIB.Create(mSize);

    mPosition = 0;
    mDiscardID = 0;

    Msg("* DIB created: %dK", mSize / 1024);
}

void _IndexStream::Destroy()
{
    pIB.Release();
    _clear();
}

u16* _IndexStream::Lock(u32 Count, u32& vOffset)
{
    vOffset = 0;

    R_ASSERT((2 * Count <= mSize) && Count);

    bool bFlush = false;
    if (2 * (Count + mPosition) >= mSize)
    {
        mPosition = 0;
        bFlush = true;
        mDiscardID++;
    }
    void* pLockedData = pIB.Map(mPosition * sizeof(u16), Count * sizeof(u16), bFlush);
    VERIFY(pLockedData);

    vOffset = mPosition;

    return static_cast<u16*>(pLockedData);
}

void _IndexStream::Unlock(u32 RealCount)
{
    mPosition += RealCount;
#ifdef XR_PLATFORM_WEB
    pIB.Unmap(RealCount * sizeof(u16));
#else
    pIB.Unmap();
#endif
}

void _IndexStream::reset_begin()
{
    old_pIB = pIB;
    Destroy();
}
void _IndexStream::reset_end()
{
    Create();
}
}
