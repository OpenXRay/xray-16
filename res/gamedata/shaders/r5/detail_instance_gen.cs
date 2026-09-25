#define DETAIL_SLOT_SIZE 2.0
#include "detail_slot_common.h"

struct GPUSlotData
{
    float world_min_x;
    float world_min_z;
    float y_base;
    float y_height;
    uint packed_ids;
    uint packed_palette_01;
    uint packed_palette_23;
    float pad;
};

struct InstanceData
{
    float3 pos;
    uint packed;
};

static const float PACK_MAX_SCALE = 4.0;
static const float TWO_PI = 6.28318530718;


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

cbuffer InstanceGenParams
{
    float g_heightmap_world_min_x;
    float g_heightmap_world_min_z;
    float g_heightmap_texel_size;
    float g_detail_height_multiplier;
    uint g_slot_offset;
    uint g_slot_count;
    uint g_instance_capacity;
    uint g_detail_model_count;
    float g_detail_density;
    uint g_grass_mode;
    uint2 g_gen_padding;
};

StructuredBuffer<GPUSlotData> g_slot_data;
Texture2D<float> g_heightmap;
StructuredBuffer<DetailModelGPU> g_detail_models;
#ifdef DETAIL_COUNT_ONLY
RWStructuredBuffer<uint4> g_per_slot_counts;
#else
StructuredBuffer<SlotAABB> g_slot_aabbs;
StructuredBuffer<uint> g_emit_slot_ids;

RWStructuredBuffer<InstanceData> g_instances;
RWByteAddressBuffer g_emit_status;
RWStructuredBuffer<uint> g_local_counters;
#endif

SamplerState smp_nofilter;

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
    precise float t = float(pcg_rand(rng)) / 4294967296.0;
    precise float value = min_val + t * (max_val - min_val);
    return value;
}

float pcg_randFs(inout PCGState rng, float range) { return pcg_randF(rng, -range, range); }

static const int g_magic4x4[4][4] = {{0,14,3,13},{11,5,8,6},{12,2,15,1},{7,9,4,10}};

int bwdithermap(uint col, uint row)
{
    uint i = col % 4, j = row % 4;
    uint k = col / 4, l = row / 4;
    const float magicfact = (255.0 - 1.0) / 16.0;
    precise float value = 0.5 + g_magic4x4[i][j] * magicfact + (g_magic4x4[k][l] / 16.0) * magicfact;
    return int(value);
}

float Interpolate(float base[4], uint x, uint y, uint size)
{
    precise float f = float(size);
    precise float fx = float(x) / f;
    precise float ifx = 1.0 - fx;
    precise float fy = float(y) / f;
    precise float ify = 1.0 - fy;

    precise float c01 = base[0] * ifx + base[1] * fx;
    precise float c23 = base[2] * ifx + base[3] * fx;
    precise float c02 = base[0] * ify + base[2] * fy;
    precise float c13 = base[1] * ify + base[3] * fy;

    precise float cx = ify * c01 + fy * c23;
    precise float cy = ifx * c02 + fx * c13;
    precise float value = (cx + cy) / 2.0;
    return value;
}

bool InterpolateAndDither(float alpha255[4], uint x, uint y, uint shift_x, uint shift_z, uint size)
{
    uint cx = clamp(x, 0u, size - 1u);
    uint cy = clamp(y, 0u, size - 1u);

    precise float value = Interpolate(alpha255, cx, cy, size) + 0.5;
    int c = int(value);
    c = clamp(c, 0, 255);

    uint row = (y + shift_z) % 16u;
    uint col = (x + shift_x) % 16u;

    return c > bwdithermap(col, row);
}

[numthreads(64, 1, 1)]
void main(uint3 group_id : SV_GroupID, uint3 thread_id : SV_GroupThreadID)
{
    uint dispatch_slot = group_id.y * 65535u + group_id.x;
    if (dispatch_slot >= g_slot_count)
        return;
#ifdef DETAIL_COUNT_ONLY
    uint slot_idx = dispatch_slot;
#else
    uint slot_idx = g_emit_slot_ids[g_slot_offset + dispatch_slot];
#endif

    GPUSlotData slot = g_slot_data[slot_idx];

    precise float grid_size = ceil(DETAIL_SLOT_SIZE / g_detail_density);
    uint d_size = uint(grid_size);
    uint total_grid_points = (d_size + 1) * (d_size + 1);

    int sx = int(slot.world_min_x / DETAIL_SLOT_SIZE);
    int sz = int(slot.world_min_z / DETAIL_SLOT_SIZE);
    uint p_rnd = uint(sx * sz);

    uint id0 = (slot.packed_ids >> 0) & 0xFFu;
    uint id1 = (slot.packed_ids >> 8) & 0xFFu;
    uint id2 = (slot.packed_ids >> 16) & 0xFFu;
    uint id3 = (slot.packed_ids >> 24) & 0xFFu;
    const uint ID_Empty = 0x3F;

    precise float alpha255[4][4];

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
    uint slot_waving_count = 0;

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

        precise float jitter = g_detail_density / 1.7;
        precise float rx = (float(x) / float(d_size)) * DETAIL_SLOT_SIZE + slot.world_min_x;
        precise float rz = (float(z) / float(d_size)) * DETAIL_SLOT_SIZE + slot.world_min_z;

        precise float3 world_pos;
        world_pos.x = rx + pcg_randFs(r_jitter, jitter);
        world_pos.z = rz + pcg_randFs(r_jitter, jitter);

        precise float2 hm_pixel = (float2(world_pos.x, world_pos.z) - float2(g_heightmap_world_min_x, g_heightmap_world_min_z)) / g_heightmap_texel_size;
        uint hm_width, hm_height;
        g_heightmap.GetDimensions(hm_width, hm_height);
        precise float2 hm_uv = hm_pixel / float2(hm_width, hm_height);
        float terrain_y = g_heightmap.SampleLevel(smp_nofilter, hm_uv, 0).r;

        const float HEIGHTMAP_NO_TERRAIN = -1e10;
        if (terrain_y < slot.y_base || terrain_y < HEIGHTMAP_NO_TERRAIN * 0.5)
            continue;

        world_pos.y = terrain_y;

        DetailModelGPU mdl = g_detail_models[object_id];
        uint flags = asuint(mdl.flags);
        const uint DO_NO_WAVING = 0x0001;
#ifdef DETAIL_COUNT_ONLY
            slot_instance_count++;
            slot_waving_count += uint((flags & DO_NO_WAVING) == 0u);
#else
            bool pulled = (flags & DO_NO_WAVING) != 0 || g_grass_mode == 0u;

            float scale = (pulled ? pcg_randF(r_scale, mdl.minScale * 0.5, mdl.maxScale * 0.9) : mdl.maxScale) * g_detail_height_multiplier;
            float rotation = pulled ? pcg_randF(r_yaw, 0.0, TWO_PI) : 0.0;

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

            SlotAABB range = g_slot_aabbs[slot_idx];
            uint base_offset = range.instance_base;

            uint local_idx;
            InterlockedAdd(g_local_counters[slot_idx], 1, local_idx);

            if (base_offset > g_instance_capacity || local_idx >= range.instance_count ||
                local_idx >= g_instance_capacity - base_offset)
            {
                g_emit_status.InterlockedOr(0, 1u);
                continue;
            }

            g_instances[base_offset + local_idx] = inst;
            slot_instance_count++;
#endif
    }

#ifdef DETAIL_COUNT_ONLY
        InterlockedAdd(g_per_slot_counts[slot_idx].x, slot_instance_count);
        InterlockedAdd(g_per_slot_counts[slot_idx].y, slot_waving_count);
        InterlockedAdd(g_per_slot_counts[slot_idx].z, slot_instance_count - slot_waving_count);
#else
        DeviceMemoryBarrierWithGroupSync();
        if (thread_id.x == 0 && g_local_counters[slot_idx] != g_slot_aabbs[slot_idx].instance_count)
            g_emit_status.InterlockedOr(0, 2u);
#endif
}
