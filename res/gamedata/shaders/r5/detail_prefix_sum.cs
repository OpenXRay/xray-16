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
};

RWStructuredBuffer<uint> g_per_slot_counts : register(u0);
RWStructuredBuffer<uint> g_prefix_output   : register(u1);
RWStructuredBuffer<uint> g_block_totals    : register(u2);

#define BLOCK_SIZE 256

groupshared uint gs_data[BLOCK_SIZE];

[numthreads(BLOCK_SIZE, 1, 1)]
void main_scan_blocks(uint3 group_id : SV_GroupID, uint3 thread_id : SV_GroupThreadID)
{
    uint tid = thread_id.x;
    uint block_idx = group_id.x;
    uint global_idx = block_idx * BLOCK_SIZE + tid;
    uint padded_total = g_prefix_sum_block_size * g_prefix_sum_total_blocks;

    gs_data[tid] = (global_idx < padded_total) ? g_per_slot_counts[global_idx] : 0;
    GroupMemoryBarrierWithGroupSync();

    uint offset = 1;
    for (uint d = BLOCK_SIZE >> 1; d > 0; d >>= 1)
    {
        GroupMemoryBarrierWithGroupSync();
        if (tid < d)
        {
            uint ai = offset * (2 * tid + 1) - 1;
            uint bi = offset * (2 * tid + 2) - 1;
            gs_data[bi] += gs_data[ai];
        }
        offset <<= 1;
    }

    GroupMemoryBarrierWithGroupSync();
    if (tid == 0)
    {
        g_block_totals[block_idx] = gs_data[BLOCK_SIZE - 1];
        gs_data[BLOCK_SIZE - 1] = 0;
    }

    for (uint d2 = 1; d2 < BLOCK_SIZE; d2 <<= 1)
    {
        offset >>= 1;
        GroupMemoryBarrierWithGroupSync();
        if (tid < d2)
        {
            uint ai = offset * (2 * tid + 1) - 1;
            uint bi = offset * (2 * tid + 2) - 1;
            uint temp = gs_data[ai];
            gs_data[ai] = gs_data[bi];
            gs_data[bi] += temp;
        }
    }

    GroupMemoryBarrierWithGroupSync();

    if (global_idx < padded_total)
        g_prefix_output[global_idx] = gs_data[tid];
}

[numthreads(BLOCK_SIZE, 1, 1)]
void main_scan_top(uint3 group_id : SV_GroupID, uint3 thread_id : SV_GroupThreadID)
{
    uint tid = thread_id.x;
    uint num_blocks = g_prefix_sum_total_blocks;
    uint padded_total = g_prefix_sum_block_size * num_blocks;

    if (tid == 0)
    {
        uint running = 0;
        for (uint i = 0; i < num_blocks; i++)
        {
            uint val = g_block_totals[i];
            g_block_totals[i] = running;
            running += val;
        }
    }

    GroupMemoryBarrierWithGroupSync();

    for (uint global_idx = tid; global_idx < padded_total; global_idx += BLOCK_SIZE)
    {
        uint block_idx = global_idx / BLOCK_SIZE;
        g_prefix_output[global_idx] += g_block_totals[block_idx];
    }
}
