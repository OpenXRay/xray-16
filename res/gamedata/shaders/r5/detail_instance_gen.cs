#define DETAIL_SLOT_SIZE 2.0

struct GPUSlotData
{
    float world_min_x;
    float world_min_z;
    float y_base;
    float y_height;
    uint packed_ids;
    uint packed_palette_01;
    uint packed_palette_23;
    float hemi;
};

struct InstanceData
{
    float3 pos;
    uint packed;
};

static const float PACK_MAX_SCALE = 4.0;
static const float TWO_PI = 6.28318530718;

struct SlotAABB
{
    float3 aabb_min;
    float padding0;
    float3 aabb_max;
    float padding1;
    uint instance_base;
    uint instance_count;
    int slot_x;
    int slot_z;
    float4 padding2;
};

cbuffer DetailCullParams : register(b5)
{
    float4x4 g_view_proj;
    float4x4 g_prev_view_proj;
    float3 g_camera_pos;
    float g_fade_distance_sqr;
    float4 g_frustum_planes[6];
    uint g_visible_blade_capacity;
    uint g_total_slot_count;
    uint g_hiz_width;
    uint g_hiz_height;
    uint g_hiz_mip_levels;
    float g_lod_distance_close_sqr;
    float g_lod_distance_mid_sqr;
    float g_detail_density;
    uint g_visible_decal_capacity;
    uint g_grass_mode;
    uint g_visible_billboard_capacity;
    uint g_cull_pad2;
};

struct DetailModelGPU
{
    float minScale;
    float maxScale;
    float flags;
    float geomExtentX;
    float geomExtentZ;
    float uv_min_x;
    float uv_min_y;
    float uv_max_x;
    float uv_max_y;
    uint pulledVertexBase;
    uint pulledIndexCount;
    float geomExtentY;
};

cbuffer InstanceGenParams : register(b6)
{
    float g_heightmap_world_min_x;
    float g_heightmap_world_min_z;
    float g_heightmap_texel_size;
    float g_detail_height_multiplier;
    uint g_gen_mode;
    uint g_prefix_sum_block_size;
    uint g_prefix_sum_total_blocks;
    uint g_instance_capacity;
    uint g_detail_model_count;
    uint g_pad0, g_pad1, g_pad2;
};

StructuredBuffer<GPUSlotData> g_slot_data : register(t1);
Texture2D<float> g_heightmap : register(t2);
StructuredBuffer<DetailModelGPU> g_detail_models : register(t3);
StructuredBuffer<uint> g_prefix_offsets : register(t4);

RWStructuredBuffer<InstanceData> g_instances : register(u0);
RWByteAddressBuffer g_instance_counter : register(u1);
RWStructuredBuffer<uint> g_per_slot_counts : register(u2);
RWStructuredBuffer<uint> g_local_counters : register(u3);
RWStructuredBuffer<SlotAABB> g_slot_aabbs : register(u4);

#include "common_samplers.h"

uint pcg_hash(uint input)
{
    uint state = input * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

struct PCGState { uint state; };

void pcg_seed(inout PCGState rng, uint seed) { rng.state = seed; }

uint pcg_rand(inout PCGState rng)
{
    rng.state = pcg_hash(rng.state);
    return rng.state;
}

uint pcg_randI(inout PCGState rng, uint max_val) { return pcg_rand(rng) % max_val; }

float pcg_randF(inout PCGState rng, float min_val, float max_val)
{
    float t = float(pcg_rand(rng)) / 4294967296.0;
    return min_val + t * (max_val - min_val);
}

float pcg_randFs(inout PCGState rng, float range) { return pcg_randF(rng, -range, range); }

static const int g_magic4x4[4][4] = {{0,14,3,13},{11,5,8,6},{12,2,15,1},{7,9,4,10}};

int bwdithermap(uint col, uint row)
{
    uint i = col % 4, j = row % 4;
    uint k = col / 4, l = row / 4;
    const float magicfact = (255.0 - 1.0) / 16.0;
    return int(0.5 + g_magic4x4[i][j] * magicfact + (g_magic4x4[k][l] / 16.0) * magicfact);
}

float Interpolate(float base[4], uint x, uint y, uint size)
{
    float f = float(size);
    float fx = float(x) / f;
    float ifx = 1.0 - fx;
    float fy = float(y) / f;
    float ify = 1.0 - fy;

    float c01 = base[0] * ifx + base[1] * fx;
    float c23 = base[2] * ifx + base[3] * fx;
    float c02 = base[0] * ify + base[2] * fy;
    float c13 = base[1] * ify + base[3] * fy;

    float cx = ify * c01 + fy * c23;
    float cy = ifx * c02 + fx * c13;
    return (cx + cy) / 2.0;
}

bool InterpolateAndDither(float alpha255[4], uint x, uint y, uint shift_x, uint shift_z, uint size)
{
    uint cx = clamp(x, 0u, size - 1u);
    uint cy = clamp(y, 0u, size - 1u);

    int c = int(Interpolate(alpha255, cx, cy, size) + 0.5);
    c = clamp(c, 0, 255);

    uint row = (y + shift_z) % 16u;
    uint col = (x + shift_x) % 16u;

    return c > bwdithermap(col, row);
}

[numthreads(64, 1, 1)]
void main(uint3 group_id : SV_GroupID, uint3 thread_id : SV_GroupThreadID)
{
    uint slot_idx = group_id.y * 65535 + group_id.x;

    if (slot_idx >= g_total_slot_count)
        return;

    GPUSlotData slot = g_slot_data[slot_idx];

    uint d_size = uint(ceil(DETAIL_SLOT_SIZE / g_detail_density));
    uint total_grid_points = (d_size + 1) * (d_size + 1);

    int sx = int(slot.world_min_x / DETAIL_SLOT_SIZE);
    int sz = int(slot.world_min_z / DETAIL_SLOT_SIZE);
    uint p_rnd = uint(sx * sz);

    uint id0 = (slot.packed_ids >> 0) & 0xFFu;
    uint id1 = (slot.packed_ids >> 8) & 0xFFu;
    uint id2 = (slot.packed_ids >> 16) & 0xFFu;
    uint id3 = (slot.packed_ids >> 24) & 0xFFu;
    const uint ID_Empty = 0x3F;

    float alpha255[4][4];

    alpha255[0][0] = 255.0 * float((slot.packed_palette_01 >> 0) & 0xFu) / 15.0;
    alpha255[0][1] = 255.0 * float((slot.packed_palette_01 >> 4) & 0xFu) / 15.0;
    alpha255[0][2] = 255.0 * float((slot.packed_palette_01 >> 8) & 0xFu) / 15.0;
    alpha255[0][3] = 255.0 * float((slot.packed_palette_01 >> 12) & 0xFu) / 15.0;

    alpha255[1][0] = 255.0 * float((slot.packed_palette_01 >> 16) & 0xFu) / 15.0;
    alpha255[1][1] = 255.0 * float((slot.packed_palette_01 >> 20) & 0xFu) / 15.0;
    alpha255[1][2] = 255.0 * float((slot.packed_palette_01 >> 24) & 0xFu) / 15.0;
    alpha255[1][3] = 255.0 * float((slot.packed_palette_01 >> 28) & 0xFu) / 15.0;

    alpha255[2][0] = 255.0 * float((slot.packed_palette_23 >> 0) & 0xFu) / 15.0;
    alpha255[2][1] = 255.0 * float((slot.packed_palette_23 >> 4) & 0xFu) / 15.0;
    alpha255[2][2] = 255.0 * float((slot.packed_palette_23 >> 8) & 0xFu) / 15.0;
    alpha255[2][3] = 255.0 * float((slot.packed_palette_23 >> 12) & 0xFu) / 15.0;

    alpha255[3][0] = 255.0 * float((slot.packed_palette_23 >> 16) & 0xFu) / 15.0;
    alpha255[3][1] = 255.0 * float((slot.packed_palette_23 >> 20) & 0xFu) / 15.0;
    alpha255[3][2] = 255.0 * float((slot.packed_palette_23 >> 24) & 0xFu) / 15.0;
    alpha255[3][3] = 255.0 * float((slot.packed_palette_23 >> 28) & 0xFu) / 15.0;

    uint slot_instance_count = 0;

    for (uint i = thread_id.x; i < total_grid_points; i += 64)
    {
        uint z = i / (d_size + 1);
        uint x = i % (d_size + 1);

        uint pos_hash = p_rnd ^ (x * 73856093u) ^ (z * 19349663u);

        PCGState r_selection, r_jitter, r_yaw, r_scale;
        pcg_seed(r_selection, 0x12071980u ^ pos_hash);
        pcg_seed(r_jitter, 0x12071981u ^ pos_hash);
        pcg_seed(r_yaw, 0x12071982u ^ pos_hash);
        pcg_seed(r_scale, 0x12071983u ^ pos_hash);

        uint shift_x = pcg_randI(r_jitter, 16u);
        uint shift_z = pcg_randI(r_jitter, 16u);

        int selected[4];
        int selected_count = 0;

        if (id0 != ID_Empty && InterpolateAndDither(alpha255[0], x, z, shift_x, shift_z, d_size))
            selected[selected_count++] = 0;
        if (id1 != ID_Empty && InterpolateAndDither(alpha255[1], x, z, shift_x, shift_z, d_size))
            selected[selected_count++] = 1;
        if (id2 != ID_Empty && InterpolateAndDither(alpha255[2], x, z, shift_x, shift_z, d_size))
            selected[selected_count++] = 2;
        if (id3 != ID_Empty && InterpolateAndDither(alpha255[3], x, z, shift_x, shift_z, d_size))
            selected[selected_count++] = 3;

        if (selected_count == 0)
            continue;

        uint index;
        if (selected_count == 1)
            index = uint(selected[0]);
        else
            index = uint(selected[pcg_randI(r_selection, uint(selected_count))]);

        uint object_id = 0;
        if (index == 0) object_id = id0;
        else if (index == 1) object_id = id1;
        else if (index == 2) object_id = id2;
        else object_id = id3;

        if (object_id >= g_detail_model_count)
            continue;

        float jitter = g_detail_density / 1.7;
        float rx = (float(x) / float(d_size)) * DETAIL_SLOT_SIZE + slot.world_min_x;
        float rz = (float(z) / float(d_size)) * DETAIL_SLOT_SIZE + slot.world_min_z;

        float3 world_pos;
        world_pos.x = rx + pcg_randFs(r_jitter, jitter);
        world_pos.z = rz + pcg_randFs(r_jitter, jitter);

        float2 hm_pixel = (float2(world_pos.x, world_pos.z) - float2(g_heightmap_world_min_x, g_heightmap_world_min_z)) / g_heightmap_texel_size;
        uint hm_width, hm_height;
        g_heightmap.GetDimensions(hm_width, hm_height);
        float2 hm_uv = hm_pixel / float2(hm_width, hm_height);
        float terrain_y = g_heightmap.SampleLevel(smp_nofilter, hm_uv, 0).r;

        const float HEIGHTMAP_NO_TERRAIN = -1e10;
        if (terrain_y < slot.y_base || terrain_y < HEIGHTMAP_NO_TERRAIN * 0.5)
            continue;

        world_pos.y = terrain_y;

        if (g_gen_mode == 0)
        {
            slot_instance_count++;
        }
        else
        {
            DetailModelGPU mdl = g_detail_models[object_id];
            float scale = mdl.maxScale * g_detail_height_multiplier;
            float rotation = 0.0f;

            uint flags = asuint(mdl.flags);
            const uint DO_NO_WAVING = 0x0001;
            uint vis_id;
            if ((flags & DO_NO_WAVING) != 0)
                vis_id = 0;
            else
            {
                if (pcg_randI(r_selection, 4u) == 0)
                    vis_id = 2;
                else
                    vis_id = 1;
            }

            uint pack_scale = uint(clamp(scale / PACK_MAX_SCALE, 0.0, 1.0) * 1023.0);
            uint pack_rotation = uint(clamp(rotation / TWO_PI, 0.0, 0.999) * 1023.0);

            InstanceData inst;
            inst.pos = world_pos;
            inst.packed = (object_id & 0x3F)
                        | ((vis_id & 0x3) << 6)
                        | ((pack_rotation & 0x3FF) << 8)
                        | ((pack_scale & 0x3FF) << 18);

            uint base_offset = g_prefix_offsets[slot_idx];
            if (base_offset >= g_instance_capacity)
                continue;

            uint local_idx;
            InterlockedAdd(g_local_counters[slot_idx], 1, local_idx);
            uint write_idx = base_offset + local_idx;

            if (write_idx >= g_instance_capacity)
                continue;

            g_instances[write_idx] = inst;
            slot_instance_count++;
        }
    }

    if (g_gen_mode == 0)
    {
        if (slot_instance_count > 0)
            InterlockedAdd(g_per_slot_counts[slot_idx], slot_instance_count);
    }
    else
    {
        if (slot_instance_count > 0)
            g_instance_counter.InterlockedAdd(0, slot_instance_count);

        GroupMemoryBarrierWithGroupSync();
        if (thread_id.x == 0)
        {
            g_slot_aabbs[slot_idx].instance_base = g_prefix_offsets[slot_idx];
            g_slot_aabbs[slot_idx].instance_count = g_local_counters[slot_idx];
        }
    }
}
