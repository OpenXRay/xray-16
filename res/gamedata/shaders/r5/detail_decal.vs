#define SM_6_0
#include "common.h"
#include "bindless_common.h"

struct InstanceData
{
	float3 pos;
	uint packed;
};

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

struct DecalPulledVertex
{
	float px, py, pz;
	float u, v;
};

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
	uint interaction_atlas_index;
	uint perlin4d_texture_index;
	float4 grass_color_tip;
	float4 grass_color_base;
	float4 grass_sss_color;
	float grass_color_variation;
	float grass_blade_height;
	uint build_details_index;
	uint build_details_pbr_index;
};

StructuredBuffer<uint> visible_indices : register(t33);
StructuredBuffer<DetailModelGPU> detail_models : register(t35);
StructuredBuffer<DecalPulledVertex> decal_vertices : register(t36);
StructuredBuffer<InstanceData> all_instances : register(t37);
StructuredBuffer<GPUSlotData> slot_data : register(t38);

v2p_decal main(uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID)
{
	v2p_decal O;

	uint src_idx = visible_indices[instance_id];
	InstanceData raw = all_instances[src_idx];

	uint object_id = raw.packed & 0x3F;
	float rotation = float((raw.packed >> 8) & 0x3FF) / 1023.0 * TWO_PI;
	float scale = float((raw.packed >> 18) & 0x3FF) / 1023.0 * PACK_MAX_SCALE;

	DetailModelGPU mdl = detail_models[object_id];

	if (vertex_id >= mdl.pulledIndexCount)
	{
		O = (v2p_decal)0;
		O.hpos = asfloat(0x7FC00000);
		return O;
	}

	DecalPulledVertex v = decal_vertices[mdl.pulledVertexBase + vertex_id];

	float3 local_pos = float3(v.px, v.py, v.pz) * scale;

	float c = cos(rotation);
	float s = sin(rotation);
	float3 rotated;
	rotated.x = local_pos.x * c - local_pos.z * s;
	rotated.y = local_pos.y;
	rotated.z = local_pos.x * s + local_pos.z * c;

	float4 world_pos = float4(rotated + raw.pos, 1.0);

	float2 uv = float2(v.u, v.v);

	const float slot_size = 2.0;
	int slot_x = int(floor(raw.pos.x / slot_size));
	int slot_z = int(floor(raw.pos.z / slot_size));
	uint x_size = uint(detail_params.x);
	int x_offs = int(detail_params.z);
	int z_offs = int(detail_params.w);
	int sx_local = clamp(slot_x + x_offs, 0, int(x_size) - 1);
	int sz_local = clamp(slot_z + z_offs, 0, int(detail_params.y) - 1);
	uint slot_idx = uint(sz_local) * x_size + uint(sx_local);

	float slot_hemi = slot_data[slot_idx].hemi;
	float hemi = abs(slot_hemi);
	float sun = sign(slot_hemi) * 0.25 + 0.25;

#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
	O.tcdh = float4(uv, hemi, sun);
#else
	O.tcdh = uv;
#endif

	O.position = float4(world_pos.xyz, hemi);
	O.N = float3(0, 1, 0);
	O.hpos = mul(g_detail_VP, world_pos);

	return O;
}
