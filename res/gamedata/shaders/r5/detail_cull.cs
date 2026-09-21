SamplerState smp_nofilter : register(s0);
SamplerState smp_rtlinear : register(s1);
SamplerState smp_linear : register(s2);
#include "cull_utils.h"
#include "detail_source_common.h"
#include "detail_cull_params.h"

cbuffer DetailGlobals : register(b3)
{
    float4 consts;
    float4 wave;
    float4 dir2D;
    float4 dir2D_2;
    float4x4 g_detail_VP;
    float4 detail_params;
    float4 g_wind_direction;
    float grass_wind_displacement;
    float grass_interaction_displacement;
    float grass_interaction_max_angle;
    float grass_blade_width;
    float4 grass_color_tip;
    float4 grass_color_base;
    float grass_color_variation;
    float grass_blade_height;
    uint buildDetailsIndex;
    uint buildDetailsPbrIndex;
    float4 interaction_window;
    float4 interaction_window_prev;
    uint buildDetailsBumpIndex;
};

static const float PACK_MAX_SCALE = 4.0;


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

StructuredBuffer<uint> g_visible_slot_ids : register(t1);
StructuredBuffer<SlotAABB> g_slot_aabbs : register(t2);
Texture2D<float> g_hiz_pyramid : register(t3);
StructuredBuffer<DetailModelGPU> g_detail_models : register(t4);
ByteAddressBuffer g_visible_slot_count : register(t5);
Texture3D g_Perlin4D : register(t12);
Texture2D g_Interaction : register(t13);


RWStructuredBuffer<uint2> g_visible_lod0 : register(u0);
RWByteAddressBuffer g_indirect_args_lod0 : register(u1);
RWStructuredBuffer<uint2> g_visible_lod1 : register(u2);
RWByteAddressBuffer g_indirect_args_lod1 : register(u3);
RWStructuredBuffer<uint2> g_visible_lod2 : register(u4);
RWByteAddressBuffer g_indirect_args_lod2 : register(u5);
RWStructuredBuffer<uint2> g_visible_decals : register(u6);
RWByteAddressBuffer g_indirect_args_decal : register(u7);
RWStructuredBuffer<uint2> g_visible_billboard : register(u8);
RWByteAddressBuffer g_indirect_args_billboard : register(u9);
RWStructuredBuffer<PreparedBlade> g_prepared_lod0 : register(u10);
RWStructuredBuffer<PreparedBlade> g_prepared_lod1 : register(u11);
RWStructuredBuffer<PreparedBlade> g_prepared_lod2 : register(u12);
RWByteAddressBuffer g_work_status : register(u13);

static const uint DO_NO_WAVING = 0x0001;
static const uint MEMBERSHIP_KIND_BILLBOARD = 3u;

PreparedBlade PrepareBlade(DetailInstance inst)
{
    BladeInstance b = DecodeBlade(inst, g_Perlin4D, smp_linear, grass_blade_height);
    float2 inter = SampleGrassInteraction(g_Interaction, smp_rtlinear, b.pos.xz, interaction_window);
    BladeBend w = EvalBladeBend(b, wave.w, g_wind_direction.xy, grass_wind_displacement, inter, grass_interaction_displacement, grass_interaction_max_angle, g_Perlin4D, smp_linear);
    return PackPreparedBlade(b, w);
}

bool ReserveVisibleSlot(RWByteAddressBuffer args, uint capacity, out uint index)
{
    args.InterlockedAdd(4, 1u, index);
    if (index < capacity)
        return true;
    g_work_status.InterlockedOr(28, 1u);
    return false;
}

uint MembershipFingerprint0(uint kind, uint2 address)
{
    uint h = kind ^ (address.x * 0x9E3779B9u) ^ (address.y * 0x85EBCA6Bu) ^ 0x7FEB352Du;
    h = (h ^ (h >> 16u)) * 0x7FEB352Du;
    h = (h ^ (h >> 15u)) * 0x846CA68Bu;
    return h ^ (h >> 16u);
}

uint MembershipFingerprint1(uint kind, uint2 address)
{
    uint h = 0x85EBCA6Bu + kind * 0x9E3779B9u;
    h += address.x * 0x165667B1u;
    h = (h << 15u) | (h >> 17u);
    h += address.y * 0xC2B2AE3Du;
    h = (h << 13u) | (h >> 19u);
    h ^= h >> 15u;
    h *= 0x2545F491u;
    h ^= h >> 13u;
    return h;
}

void AppendMembership(inout uint2 membership, uint kind, uint2 address)
{
    membership.x += MembershipFingerprint0(kind, address);
    membership.y += MembershipFingerprint1(kind, address);
}

void AppendBladeLOD(DetailInstance inst, uint2 address, inout uint2 membership)
{
    float3 to_camera = inst.pos - g_camera_pos;
    float dist_sqr = dot(to_camera, to_camera);

    uint idx;
    if (dist_sqr < g_lod_distance_close_sqr)
    {
        if (!ReserveVisibleSlot(g_indirect_args_lod0, g_visible_blade_capacity.x, idx))
            return;
        g_visible_lod0[idx] = address;
        AppendMembership(membership, 0u, address);
        if (idx < g_prepared_capacity.x)
            g_prepared_lod0[idx] = PrepareBlade(inst);
    }
    else if (dist_sqr < g_lod_distance_mid_sqr)
    {
        if (!ReserveVisibleSlot(g_indirect_args_lod1, g_visible_blade_capacity.y, idx))
            return;
        g_visible_lod1[idx] = address;
        AppendMembership(membership, 1u, address);
        if (idx < g_prepared_capacity.y)
            g_prepared_lod1[idx] = PrepareBlade(inst);
    }
    else
    {
        if (!ReserveVisibleSlot(g_indirect_args_lod2, g_visible_blade_capacity.z, idx))
            return;
        g_visible_lod2[idx] = address;
        AppendMembership(membership, 2u, address);
        if (idx < g_prepared_capacity.z)
            g_prepared_lod2[idx] = PrepareBlade(inst);
    }
}

void AppendBillboard(uint2 address, inout uint2 membership)
{
    uint idx;
    if (ReserveVisibleSlot(g_indirect_args_billboard, g_visible_billboard_capacity, idx))
    {
        g_visible_billboard[idx] = address;
        AppendMembership(membership, MEMBERSHIP_KIND_BILLBOARD, address);
    }
}

void AppendDecal(uint2 address)
{
    uint idx;
    if (ReserveVisibleSlot(g_indirect_args_decal, g_visible_decal_capacity, idx))
        g_visible_decals[idx] = address;
}

[numthreads(64, 1, 1)]
void main(uint3 group_id : SV_GroupID, uint3 thread_id : SV_GroupThreadID)
{
    uint visibleSlot = group_id.y * 65535u + group_id.x;
    if (visibleSlot >= g_visible_slot_count.Load(0))
        return;
    uint slot_id = g_visible_slot_ids[visibleSlot];
    SlotAABB slot = g_slot_aabbs[slot_id];
    uint2 membership = uint2(0u, 0u);

    for (uint i = thread_id.x; i < slot.instance_count; i += 64)
    {
        uint2 address = uint2(slot.instance_chunk, slot.instance_base + i);
        DetailInstance inst = LoadDetailInstance(address);

        float scale = float((inst.packed >> 18) & 0x3FF) / 1023.0 * PACK_MAX_SCALE;
        uint object_id = inst.packed & 0x3F;

        DetailModelGPU mdl = g_detail_models[object_id];
        float full_height = scale * mdl.geomExtentY;
        float bounds_radius = max(full_height, scale * max(mdl.geomExtentX, mdl.geomExtentZ) * 0.5);
        bounds_radius = max(bounds_radius, scale * 0.5);
        float3 bounds_center = inst.pos + float3(0, full_height * 0.5, 0);


        if (g_ray_mode != 0u)
        {
            float3 to_camera = bounds_center - g_camera_pos;
            float reach = g_ray_radius + bounds_radius;
            if (dot(to_camera, to_camera) > reach * reach)
                continue;
        }
        else
        {
            if (!FrustumTestSphere(bounds_center, bounds_radius, g_frustum_planes))
                continue;

            if (g_hiz_mip_levels != 0 && !HiZTestSphere(bounds_center, bounds_radius, g_camera_pos, g_prev_view_proj,
                                g_hiz_pyramid, smp_nofilter, g_hiz_width, g_hiz_height, g_hiz_mip_levels))
                continue;
        }

        uint flags = asuint(mdl.flags);
        bool is_static = (flags & DO_NO_WAVING) != 0;
        if (is_static)
            AppendDecal(address);
        else if (g_grass_mode == 0)
            AppendBillboard(address, membership);
        else
            AppendBladeLOD(inst, address, membership);
    }

    if (membership.x != 0u || membership.y != 0u)
    {
        g_work_status.InterlockedAdd(32, membership.x);
        g_work_status.InterlockedAdd(36, membership.y);
    }
}
