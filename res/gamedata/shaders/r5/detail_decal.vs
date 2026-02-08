#define SM_6_0
#include "common.h"
#include "bindless_common.h"

struct InstanceData
{
	float3 pos;
	float scale;
	float rotation;
	float hemi;
	uint vis_id;
	uint object_id;
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
	uint decalVertexBase;
	uint decalIndexCount;
	uint pad;
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
	float4x4 m_VP;
	float4 detail_params;
	float4 g_wind_direction;
	float grass_wind_displacement;
	float grass_interaction_displacement;
	uint interaction_atlas_index;
	uint wind_texture_index;
	float4 grass_color_tip;
	float4 grass_color_base;
	float4 grass_sss_color;
	float grass_color_variation;
	float grass_blade_height;
	float _pad0;
	uint build_details_index;
};

StructuredBuffer<InstanceData> detail_buffer : register(t33);
StructuredBuffer<DetailModelGPU> detail_models : register(t35);
StructuredBuffer<DecalPulledVertex> decal_vertices : register(t36);

v2p_flat main(uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID)
{
	v2p_flat O;

	InstanceData det = detail_buffer[instance_id];
	DetailModelGPU mdl = detail_models[det.object_id];

	if (vertex_id >= mdl.decalIndexCount)
	{
		O.hpos = float4(0, 0, 0, 0);
		O.position = 0;
		O.tcdh = 0;
		O.N = 0;
		O.heightParam = 0;
		O.rotatedNormal1 = 0;
		O.rotatedNormal2 = 0;
		O.interaction_uv = 0;
		O.objectId = 0;
		return O;
	}

	DecalPulledVertex v = decal_vertices[mdl.decalVertexBase + vertex_id];

	float3 local_pos = float3(v.px, v.py, v.pz) * det.scale;

	float c = cos(det.rotation);
	float s = sin(det.rotation);
	float3 rotated;
	rotated.x = local_pos.x * c - local_pos.z * s;
	rotated.y = local_pos.y;
	rotated.z = local_pos.x * s + local_pos.z * c;

	float4 world_pos = float4(rotated + det.pos, 1.0);

	float3 Pe = mul(m_WV, world_pos);

	float3 N_up = float3(0, 1, 0);
	float3 view_N = mul((float3x3)m_WV, N_up);

	float2 uv = float2(v.u, v.v);
	uint flip = asuint(det.rotation) ^ det.vis_id;
	if (flip & 1u) uv.x = mdl.uv_min_x + mdl.uv_max_x - uv.x;
	if (flip & 2u) uv.y = mdl.uv_min_y + mdl.uv_max_y - uv.y;

	float hemi = abs(det.hemi);
	float sun = sign(det.hemi) * 0.25 + 0.25;

#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
	O.tcdh = float4(uv, hemi, sun);
#else
	O.tcdh = uv;
#endif

	O.position = float4(Pe, hemi);
	O.N = view_N;
	O.heightParam = 0;
	O.rotatedNormal1 = view_N;
	O.rotatedNormal2 = view_N;
	O.interaction_uv = float2(0, 0);
	O.objectId = det.object_id;
	O.hpos = mul(m_P, float4(Pe, 1.0));

	return O;
}
