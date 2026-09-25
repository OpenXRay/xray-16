#include "cull_utils.h"
#include "detail_cull_params.h"
#include "sw_dispatch_common.h"

StructuredBuffer<SlotAABB> g_slot_aabbs;
RWStructuredBuffer<uint> g_visible_slot_ids;
RWByteAddressBuffer g_visible_slot_counter;

[numthreads(256, 1, 1)]
void main(uint3 group_id : SV_GroupID, uint group_index : SV_GroupIndex)
{
    uint slot_idx = SwDispatchLinearGroup(group_id) * 256u + group_index;
    if (slot_idx >= g_total_slot_count)
        return;
    SlotAABB slot = g_slot_aabbs[slot_idx];
    if (slot.instance_count == 0u)
        return;
    if (g_ray_mode != 0u)
    {
        float2 closest = clamp(g_camera_pos.xz, slot.aabb_min.xz, slot.aabb_max.xz);
        float2 delta = closest - g_camera_pos.xz;
        if (dot(delta, delta) > g_ray_cell_radius * g_ray_cell_radius)
            return;
    }
    else if (!FrustumTestAABB(slot.aabb_min, slot.aabb_max, g_frustum_planes))
        return;
    uint insert_index;
    g_visible_slot_counter.InterlockedAdd(0, 1u, insert_index);
    g_visible_slot_ids[insert_index] = slot_idx;
}
