#include "stdafx.h"
#pragma hdrstop
#include "DetailManager.h"

namespace xray::render::RENDER_NAMESPACE
{
namespace detail_manager
{
extern const int quant = 16384;
extern const int c_hdr = 10;
const int c_size = 4;

// Original vertex format for DX9/GL batching
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

#ifdef USE_DX11
// Phase 1, Milestone 1.2: Use same vertex format as per-object geometry (CDetail)
static VertexElement dwDecl_unified[] =
{
    {0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0}, // pos.frac
    {0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0}, // uv
    D3DDECL_END()
};

struct vertHW_unified
{
    Fvector4 pos_frac; // position.xyz, frac (normalized height)
    Fvector2 uv;       // texture coordinates
};
#endif

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
    clamp<size_t>(hw_BatchSize, 0, 64);
    Msg("* [DETAILS] VertexConsts(%u), Batch(%zu)", u32(HW.Caps.geometry.dwRegisters), hw_BatchSize);

#ifndef USE_DX11
    // DX9/GL: Original path - single unified geometry for batching
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
#else
    // DX11: Phase 1, Milestone 1.2 - Create unified geometry per vis_id
    // Each vis_id gets one unified VB/IB containing all objects that can appear with that vis_id

    // For simplicity in Milestone 1.2, we'll include ALL objects in each vis_id's unified geometry
    // (Later optimization: only include objects that actually appear in that vis_id)
    // This allows any object to be rendered with any animation type

    for (u32 vis_id = 0; vis_id < 3; vis_id++)
    {
        // Clear offset arrays
        vis_geometry_vertex_offsets[vis_id].clear();
        vis_geometry_index_offsets[vis_id].clear();
        vis_object_indices[vis_id].clear();

        // Calculate total geometry for this vis_id
        u32 total_verts = 0;
        u32 total_indices = 0;

        for (u32 o = 0; o < objects.size(); o++)
        {
            const CDetail& D = *objects[o];
            vis_object_indices[vis_id].push_back(o);  // Map local index to global object index
            vis_geometry_vertex_offsets[vis_id].push_back(total_verts);
            vis_geometry_index_offsets[vis_id].push_back(total_indices);

            total_verts += D.number_vertices;
            total_indices += D.number_indices;
        }

        vis_total_vertices[vis_id] = total_verts;
        vis_total_indices[vis_id] = total_indices;

        Msg("* [DETAILS] vis_id=%u: %u objects, %u vertices, %u indices",
            vis_id, objects.size(), total_verts, total_indices);

        // Create VB for this vis_id (use persistent member buffer)
        u32 vSize_unified = sizeof(vertHW_unified);
        vis_unified_VB[vis_id].Create(total_verts * vSize_unified);
        {
            vertHW_unified* pV = static_cast<vertHW_unified*>(vis_unified_VB[vis_id].Map());
            for (u32 o = 0; o < objects.size(); o++)
            {
                const CDetail& D = *objects[o];

                float height_range = D.bv_bb.vMax.y - D.bv_bb.vMin.y;
                if (height_range < 0.001f) height_range = 1.0f; // Avoid division by zero

                for (u32 v = 0; v < D.number_vertices; v++)
                {
                    const Fvector& vP = D.vertices[v].P;
                    pV->pos_frac.x = vP.x;
                    pV->pos_frac.y = vP.y;
                    pV->pos_frac.z = vP.z;
                    pV->pos_frac.w = vP.y / height_range; // Normalized height for wave animation
                    pV->uv.x = D.vertices[v].u;
                    pV->uv.y = D.vertices[v].v;
                    pV++;
                }
            }
            vis_unified_VB[vis_id].Unmap(true);
        }

        // Create IB for this vis_id (use persistent member buffer)
        vis_unified_IB[vis_id].Create(total_indices * sizeof(u16));
        {
            u16* pI = static_cast<u16*>(vis_unified_IB[vis_id].Map());
            for (u32 o = 0; o < objects.size(); o++)
            {
                const CDetail& D = *objects[o];
                u32 vertex_offset = vis_geometry_vertex_offsets[vis_id][o];

                for (u32 i = 0; i < D.number_indices; i++)
                {
                    *pI++ = u16(D.indices[i] + vertex_offset);
                }
            }
            vis_unified_IB[vis_id].Unmap(true);
        }

        // Create geometry for this vis_id (now buffers persist!)
        vis_unified_geom[vis_id].create(dwDecl_unified, vis_unified_VB[vis_id], vis_unified_IB[vis_id]);
    }

    Msg("* [DETAILS] Phase 1.2: Created 3 unified geometries (one per vis_id)");
#endif
}

void CDetailManager::hw_Unload()
{
#ifndef USE_DX11
    // Destroy VS/VB/IB (DX9/GL only - DX11 uses per-object buffers)
    if (hw_Geom)
        hw_Geom.destroy();
    if (hw_IB)
        hw_IB.Release();
    if (hw_VB)
        hw_VB.Release();
#else
    // Phase 1, Milestone 1.1: Release 3 vis_id-based structured buffers (DX11)
    for (u32 vis_id = 0; vis_id < 3; vis_id++)
    {
        _RELEASE(detailBuffer_vis[vis_id]);
        _RELEASE(detailSRV_vis[vis_id]);

        // Phase 1, Milestone 1.2: Release unified geometries and buffers
        if (vis_unified_geom[vis_id])
            vis_unified_geom[vis_id].destroy();
        if (vis_unified_VB[vis_id])
            vis_unified_VB[vis_id].Release();
        if (vis_unified_IB[vis_id])
            vis_unified_IB[vis_id].Release();

        // Clear offset tracking arrays
        vis_geometry_vertex_offsets[vis_id].clear();
        vis_geometry_index_offsets[vis_id].clear();
        vis_object_indices[vis_id].clear();
    }

    // Phase 2.0.3: Release persistent GPU buffers
    _RELEASE(persistent_instance_buffer);
    _RELEASE(persistent_instance_srv);
    persistent_buffer_capacity = 0;

    // Phase 4A: Release slot AABB buffers
    DestroySlotAABBBuffer();

    // Phase 2.1: Release GPU culling buffers
    if (cull_compute_shader)
        cull_compute_shader.destroy();
    _RELEASE(cull_constant_buffer);
    _RELEASE(gpu_visible_counts_buffer);
    _RELEASE(gpu_visible_counts_uav);
    _RELEASE(gpu_visible_counts_readback);
    for (u32 i = 0; i < max_gpu_culled_objects; i++)
    {
        _RELEASE(gpu_visible_buffers[i]);
        _RELEASE(gpu_visible_srvs[i]);
        _RELEASE(gpu_visible_uavs[i]);
    }

    // Phase 5: Release interactive grass buffers
    DestroyInteractionAtlas();
    DestroyEntityTrackingBuffers();
    DestroyWindTexture();
    if (interaction_compute_shader)
        interaction_compute_shader.destroy();
    if (wind_compute_shader)
        wind_compute_shader.destroy();
    _RELEASE(interaction_constant_buffer);
    _RELEASE(wind_constant_buffer);

    // Phase 5: Shutdown warm cache (Persistence Layer)
    ShutdownWarmCache();

    // Phase 6: Shutdown page table
    ShutdownPageTable();

    // Phase 6B: Shutdown visibility readback
    ShutdownVisibilityReadback();
#endif
}
} // namespace xray::render::RENDER_NAMESPACE
