#include "stdafx.h"
#pragma hdrstop
#include "DetailManager.h"

namespace xray::render::fg
{
extern int ps_r__detail_gpu;
namespace detail_manager
{
extern const int quant = 16384;
extern const int c_hdr = 10;
const int c_size = 4;

// Original vertex format for DX9/GL batching
static VertexElement dwDecl[] =
{
    {0, 0,  VF_FLOAT3, 0, VS_POSITION, 0},
    {0, 12, VF_SHORT4, 0, VS_TEXCOORD, 0},
    XR_VERTEX_ELEMENT_END
};

#pragma pack(push, 1)
struct vertHW
{
    float x, y, z;
    short u, v, t, mid;
};
#pragma pack(pop)

// Phase 1, Milestone 1.2: Use same vertex format as per-object geometry (CDetail)
static VertexElement dwDecl_unified[] =
{
    {0, 0,  VF_FLOAT4, 0, VS_POSITION, 0},
    {0, 16, VF_FLOAT2, 0, VS_TEXCOORD, 0},
    XR_VERTEX_ELEMENT_END
};

struct vertHW_unified
{
    Fvector4 pos_frac; // position.xyz, frac (normalized height)
    Fvector2 uv;       // texture coordinates
};

short QC(float v)
{
    int t = iFloor(v * float(quant));
    clamp(t, -32768, 32767);
    return short(t & 0xffff);
}
} // namespace detail_manager

// Basic SDF primitive - distance to circle at origin with radius r
float sdCircle(Fvector2 p, float r)
{
    return sqrtf(p.x * p.x + p.y * p.y) - r;
}

// Boolean operation for subtracting one SDF from another
float opSubtraction(float d1, float d2)
{
    return _max(d1, -d2);
}

// 2D grass blade shape using CSG (Constructive Solid Geometry)
float sdGrassBlade2d(Fvector2 p)
{
    // IMPORTANT: Don't modify p! Create new vectors for each operation.

    // Start with a large circle centered at (1.7, -1.3) with radius 2.0
    Fvector2 p_offset1;
    p_offset1.x = p.x - 1.7f;
    p_offset1.y = p.y - (-1.3f);  // p.y - (-1.3) = p.y + 1.3
    float dist = sdCircle(p_offset1, 2.0f);

    // Subtract a slightly smaller circle centered at (1.7, -1.0) with radius 1.8
    Fvector2 p_offset2;
    p_offset2.x = p.x - 1.7f;
    p_offset2.y = p.y - (-1.0f);  // p.y - (-1.0) = p.y + 1.0
    dist = opSubtraction(dist, sdCircle(p_offset2, 1.8f));

    // Cut off the bottom: keep only where p.y >= -1.0
    // This is a half-space: distance is (p.y - (-1.0)) = p.y + 1.0
    dist = opSubtraction(dist, p.y + 1.0f);

    // Cut off the left side: keep only where p.x <= 1.7
    // This is a half-space: distance is (1.7 - p.x) = -p.x + 1.7
    dist = opSubtraction(dist, -p.x + 1.7f);

    return dist;
}

// Extrude the 2D blade into 3D with thickness
float sdGrassBlade(Fvector p, float thickness)
{
    // Match shader exactly: p -= vec3(0, 1.0, 0)
    p.y -= 1.0f;

    // Get the 2D distance in the XY plane
    Fvector2 p_2d = { p.x, p.y };
    float dist2d = sdGrassBlade2d(p_2d);
    dist2d = _max(0.0f, dist2d);

    // Extend into 3D
    return sqrtf(dist2d * dist2d + p.z * p.z) - thickness;
}

void CDetailManager::hw_Load()
{
    hw_Load_Geom();
    hw_Load_Shaders();
}

void CDetailManager::hw_Load_Geom()
{
    using namespace detail_manager;

    // Analyze batch-size
    const auto& caps = GEnv.Backend->GetCapabilities();
    hw_BatchSize = (caps.geometry.dwRegisters - c_hdr) / c_size;
    clamp<size_t>(hw_BatchSize, 0, 64);
    Msg("* [DETAILS] VertexConsts(%u), Batch(%zu)", caps.geometry.dwRegisters, hw_BatchSize);

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

    // === GPU COMPUTE PATH (DX11) ===
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
}

void CDetailManager::hw_Unload()
{
    // === ALWAYS CLEANUP VANILLA GEOMETRY (both paths need it) ===
    if (hw_Geom)
        hw_Geom.destroy();
    if (hw_IB)
        hw_IB.Release();
    if (hw_VB)
        hw_VB.Release();

#ifdef USE_DX11
    // === ALWAYS CLEANUP GPU GEOMETRY (both paths need it for switching) ===
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

    // Phase 2.1: Release UNIFIED GPU culling buffers
    if (cull_compute_shader)
        cull_compute_shader.destroy();
    _RELEASE(cull_constant_buffer);
    _RELEASE(gpu_visible_count_buffer);
    _RELEASE(gpu_visible_count_uav);
    _RELEASE(gpu_visible_count_readback);
    _RELEASE(gpu_visible_unified_buffer);
    _RELEASE(gpu_visible_unified_srv);
    _RELEASE(gpu_visible_unified_uav);
    _RELEASE(gpu_indirect_args_unified);
    _RELEASE(gpu_indirect_args_unified_uav);

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

    // Destroy GPU rendering shader and blender
    if (gpu_detail_shader)
        gpu_detail_shader.destroy();
    xr_delete(b_detail_gpu);
#endif
}


// Blade mesh generation now creates simple straight geometry
// All Bezier curve evaluation and bending happens in the vertex shader (detail_gpu.vs)
// This matches Ghost of Tsushima's GPU-driven approach

void CDetailManager::GenerateGrassBlade(xr_vector<BladeVertex>& vertices, xr_vector<u16>& indices, int segments)
{
    vertices.clear();
    indices.clear();

    Msg("! [Straight Blade] Generating straight vertical blade mesh");
    Msg("! [Straight Blade] Bezier curve evaluation and bending happens in vertex shader");

    // Simple straight blade - all shape is computed in vertex shader
    const float blade_height = 1.0f;      // Normalized height (scaled by instance)
    const float blade_width = 0.04f;       // Base width

    // Generate triangle strip vertices along a straight vertical line
    for (int i = 0; i <= segments; i++)
    {
        float t = float(i) / float(segments);

        // Straight vertical position (Y-up)
        Fvector pos;
        pos.x = 0.0f;
        pos.y = t * blade_height;  // Straight up
        pos.z = 0.0f;

        // Width tapers from base to tip (keeps some width at tip to avoid degenerate triangles)
        float width_scale = 1.0f - t * 0.85f;  // Tapers to 15% of base width
        float segment_width = blade_width * width_scale;

        // Right vector is simply X-axis for a straight vertical blade
        Fvector right;
        right.set(1.0f, 0.0f, 0.0f);

        if (i % 2 == 0) {
            Msg("! [Straight Blade] Segment %d/%d: t=%.2f, y=%.3f, width=%.3f",
                i, segments, t, pos.y, segment_width);
        }

        // Generate left and right vertices (except for the final tip vertex)
        if (i < segments)
        {
            // Left vertex
            BladeVertex vtx_left;
            vtx_left.pos.x = pos.x - right.x * segment_width;
            vtx_left.pos.y = pos.y;
            vtx_left.pos.z = pos.z;
            vtx_left.uv.set(0.0f, t);
            vtx_left.t = t;
            vtx_left.width_scale = segment_width;
            vertices.push_back(vtx_left);

            // Right vertex
            BladeVertex vtx_right;
            vtx_right.pos.x = pos.x + right.x * segment_width;
            vtx_right.pos.y = pos.y;
            vtx_right.pos.z = pos.z;
            vtx_right.uv.set(1.0f, t);
            vtx_right.t = t;
            vtx_right.width_scale = segment_width;
            vertices.push_back(vtx_right);
        }
        else
        {
            // Final tip vertex (centered)
            BladeVertex vtx_tip;
            vtx_tip.pos = pos;
            vtx_tip.uv.set(0.5f, 1.0f);
            vtx_tip.t = 1.0f;
            vtx_tip.width_scale = segment_width;
            vertices.push_back(vtx_tip);
        }
    }

    Msg("! [Bezier Blade] Generated %d vertices", vertices.size());

    // Generate triangle strip indices
    // Pattern: 0,1,2,3,4,5,...,final_tip
    for (size_t i = 0; i < vertices.size(); i++)
    {
        indices.push_back(static_cast<u16>(i));
    }

    Msg("! [Bezier Blade] Generated %d indices for triangle strip", indices.size());

    // Calculate and report bounding box
    Fvector bbox_min(FLT_MAX, FLT_MAX, FLT_MAX);
    Fvector bbox_max(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    for (const auto& v : vertices)
    {
        bbox_min.x = _min(bbox_min.x, v.pos.x);
        bbox_min.y = _min(bbox_min.y, v.pos.y);
        bbox_min.z = _min(bbox_min.z, v.pos.z);
        bbox_max.x = _max(bbox_max.x, v.pos.x);
        bbox_max.y = _max(bbox_max.y, v.pos.y);
        bbox_max.z = _max(bbox_max.z, v.pos.z);
    }

    Fvector size;
    size.sub(bbox_max, bbox_min);

    Msg("! [Bezier Blade] Bounding box: (%.3f,%.3f,%.3f) to (%.3f,%.3f,%.3f)",
        bbox_min.x, bbox_min.y, bbox_min.z, bbox_max.x, bbox_max.y, bbox_max.z);
    Msg("! [Bezier Blade] Blade dimensions: %.3f wide x %.3f tall x %.3f thick",
        size.x, size.y, size.z);
    Msg("! [Bezier Blade] Mesh generation complete!");
}
} // namespace xray::render::fg
