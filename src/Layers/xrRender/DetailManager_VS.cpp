#include "stdafx.h"
#pragma hdrstop
#include "DetailManager.h"

namespace detail_manager
{
extern const int quant = 16384;
extern const int c_hdr = 10;
const int c_size = 4;

static VertexElement dwDecl[] =
{
    {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0}, // pos
    {0, 12, D3DDECLTYPE_SHORT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0}, // uv
    D3DDECL_END()
};

#pragma pack(push, 1)
struct vertHW
{
    float x, y, z;
    short u, v, t, mid;
};
#pragma pack(pop)

short QC(float v)
{
    int t = iFloor(v * float(quant));
    clamp(t, -32768, 32767);
    return short(t & 0xffff);
}
} // namespace detail_manager

void CDetailManager::hw_Load()
{
    hw_Load_Geom();
    hw_Load_Shaders();
}

void CDetailManager::hw_Load_Geom()
{
    using namespace detail_manager;

    // Analyze batch-size
    hw_BatchSize = (u32(HW.Caps.geometry.dwRegisters) - c_hdr) / c_size;
#if RENDER == R_R4
    if (RImplementation.o.instanced_details)
    {
        hw_BatchSize = 1;
    }
#endif
    clamp<size_t>(hw_BatchSize, 0, 64);
    Msg("* [DETAILS] VertexConsts(%u), Batch(%zu)", u32(HW.Caps.geometry.dwRegisters), hw_BatchSize);

    // Pre-process objects
    u32 dwVerts = 0;
    u32 dwIndices = 0;
    for (u32 o = 0; o < objects.size(); o++)
    {
        const CDetail& D = *objects[o];
        dwVerts += D.number_vertices * hw_BatchSize;
        dwIndices += D.number_indices * hw_BatchSize;
    }
    u32 vSize = sizeof(vertHW);
    Msg("* [DETAILS] %d v(%d), %d p", dwVerts, vSize, dwIndices / 3);
    Msg("* [DETAILS] Batch(%d), VB(%dK), IB(%dK)", hw_BatchSize, (dwVerts * vSize) / 1024, (dwIndices * 2) / 1024);

    // Fill VB
    hw_VB.Create(dwVerts * vSize);
    {
        vertHW* pV = static_cast<vertHW*>(hw_VB.Map());
        for (u32 o = 0; o < objects.size(); o++)
        {
            const CDetail& D = *objects[o];
            for (u32 batch = 0; batch < hw_BatchSize; batch++)
            {
                u32 mid = batch * c_size;
                for (u32 v = 0; v < D.number_vertices; v++)
                {
                    const Fvector& vP = D.vertices[v].P;
                    pV->x = vP.x;
                    pV->y = vP.y;
                    pV->z = vP.z;
                    pV->u = QC(D.vertices[v].u);
                    pV->v = QC(D.vertices[v].v);
                    pV->t = QC(vP.y / (D.bv_bb.vMax.y - D.bv_bb.vMin.y));
                    pV->mid = short(mid);
                    pV++;
                }
            }
        }
        hw_VB.Unmap(true); // upload vertex data
    }

    // Fill IB
    hw_IB.Create(dwIndices * sizeof(u16));
    {
        u16* pI = static_cast<u16*>(hw_IB.Map());
        for (u32 o = 0; o < objects.size(); o++)
        {
            const CDetail& D = *objects[o];
            u16 offset = 0;
            for (u32 batch = 0; batch < hw_BatchSize; batch++)
            {
                for (u32 i = 0; i < u32(D.number_indices); i++)
                    *pI++ = u16(u16(D.indices[i]) + u16(offset));
                offset = u16(offset + u16(D.number_vertices));
            }
        }
        hw_IB.Unmap(true); // upload index data
    }

    // Declare geometry
    hw_Geom.create(dwDecl, hw_VB, hw_IB);
}

#if RENDER != R_R4
void CDetailManager::hw_Unload()
{
    // Destroy VS/VB/IB
    if (hw_Geom)
        hw_Geom.destroy();
    if (hw_IB)
        hw_IB.Release();
    if (hw_VB)
        hw_VB.Release();
}
#endif
