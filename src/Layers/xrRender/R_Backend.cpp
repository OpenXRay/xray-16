#include "stdafx.h"
#pragma hdrstop

CBackend RCache;

void CBackend::CreateQuadIB()
{
    constexpr auto triCount = 4 * 1024;
    constexpr auto idxCount = triCount * 2 * 3;
    constexpr auto idxSize = idxCount * sizeof(u16);

    QuadIB.Create(idxSize, false, false);

    u16* Indices = static_cast<u16*>(QuadIB.Map());
    {
        int Cnt = 0;
        int ICnt = 0;
        for (int i = 0; i < triCount; i++)
        {
            Indices[ICnt++] = u16(Cnt + 0);
            Indices[ICnt++] = u16(Cnt + 1);
            Indices[ICnt++] = u16(Cnt + 2);

            Indices[ICnt++] = u16(Cnt + 3);
            Indices[ICnt++] = u16(Cnt + 2);
            Indices[ICnt++] = u16(Cnt + 1);

            Cnt += 4;
        }
    }
    QuadIB.Unmap(true); // upload index data
}

// Device dependance
void CBackend::OnDeviceCreate()
{
    CreateQuadIB();

    // streams
    Vertex.Create();
    Index.Create();

    // Debug Draw
    InitializeDebugDraw();

    // invalidate caching
    Invalidate();
}

void CBackend::OnDeviceDestroy()
{
    // streams
    Index.Destroy();
    Vertex.Destroy();

    // Quad
    QuadIB.Release();

    // Debug Draw
    DestroyDebugDraw();
}
