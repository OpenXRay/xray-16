#define SM_6_0
#include "common.h"
#include "bindless_common.h"

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
    float _pad0, _pad1, _pad2;
};

static const float M_PI = 3.1415926;

static const float HEIGHT_VARIATION_MIN = 0.7;
static const float HEIGHT_VARIATION_MAX = 1.15;
static const float HEIGHT_NOISE_SCALE = 0.05;

Buffer<uint> slot_indirection : register(t32);

struct v_blade_sdf
{
	float3 pos : POSITION;
	float2 tc : TEXCOORD;
	float t : COLOR0;
	float width_scale : COLOR1;
};

struct InstanceData
{
	float3 pos;
	float scale;
	float rotation;
	float hemi;
	uint vis_id;
	uint object_id;
};

StructuredBuffer<InstanceData> detail_buffer : register(t33);

v2p_flat main(v_blade_sdf I, uint instance_id : SV_InstanceID)
{
	v2p_flat O;

	InstanceData det = detail_buffer[instance_id];

	float2 height_uv = det.pos.xz * HEIGHT_NOISE_SCALE + float2(0.37, 0.73);
	Texture2D height_noise_tex = GetBindlessTexture(wind_texture_index);
	float height_noise = height_noise_tex.SampleLevel(g_LinearSampler, height_uv, 0).r;

	float height_multiplier = lerp(HEIGHT_VARIATION_MIN, HEIGHT_VARIATION_MAX, height_noise);
	float blade_height = det.scale * height_multiplier;

	float3 P0 = det.pos;
	float3 P1 = det.pos + float3(0, blade_height * 0.33, 0);
	float3 P2 = det.pos + float3(0, blade_height * 0.67, 0);
	float3 P3 = det.pos + float3(0, blade_height, 0);

	float3 base_world_pos = P0;

	float blade_rotation = det.rotation;
	float3 facing = normalize(float3(sin(blade_rotation), 0.0, cos(blade_rotation)));

	float vertex_height_factor = I.t;

	float2 atlas_uv = float2(0, 0);

	const float slot_size = 2.0;
	int slot_x = int(floor(base_world_pos.x / slot_size));
	int slot_z = int(floor(base_world_pos.z / slot_size));

	uint x_size = uint(detail_params.x);
	uint z_size = uint(detail_params.y);
	int x_offs = int(detail_params.z);
	int z_offs = int(detail_params.w);

	int sx_local = slot_x + x_offs;
	int sz_local = slot_z + z_offs;

	sx_local = clamp(sx_local, 0, int(x_size) - 1);
	sz_local = clamp(sz_local, 0, int(z_size) - 1);

	uint slot_idx = uint(sz_local) * x_size + uint(sx_local);

	uint packed_indirection = slot_indirection.Load(slot_idx);
	uint physical_page = packed_indirection & 0xFFFF;
	uint mip_level = (packed_indirection >> 16) & 0xFF;

	float4 interaction;

	if (physical_page == 0xFFFF) {
		interaction = float4(0, 0, 0, 0);
	} else {
		const uint pages_per_row = 64;
		uint page_x = physical_page % pages_per_row;
		uint page_y = physical_page / pages_per_row;

		const float page_size_uv = 32.0 / 2048.0;

		float2 page_base_uv = float2(page_x, page_y) * page_size_uv;

		float2 slot_local = frac(base_world_pos.xz / slot_size);

		atlas_uv = page_base_uv + slot_local * page_size_uv;

		Texture2D interaction_tex = GetBindlessTexture(interaction_atlas_index);
		interaction = interaction_tex.SampleLevel(g_LinearSampler, atlas_uv, 0);
	}

	float2 wind_uv = P0.xz * 0.001;
	Texture2D wind_tex = GetBindlessTexture(wind_texture_index);
	float4 wind = wind_tex.SampleLevel(g_LinearSampler, wind_uv, 0);

	float fbm_wind_strength = wind.a;
	float fbm_turbulence = (wind.r * 2.0 - 1.0) * 0.3;

	float wind_angle_rad = g_wind_direction.x * (M_PI / 180.0);
	float2 global_wind_dir = float2(sin(wind_angle_rad), cos(wind_angle_rad));

	float2 perpendicular_dir = float2(-global_wind_dir.y, global_wind_dir.x);
	float2 wind_dir_with_turbulence = normalize(global_wind_dir + perpendicular_dir * fbm_turbulence);

	float2 interaction_push_dir = interaction.rg * 2.0 - 1.0;
	float interaction_strength = length(interaction_push_dir);
	if (interaction_strength > 0.001)
	{
		interaction_push_dir = interaction_push_dir / interaction_strength;
	}

	float3 wind_offset = float3(
		wind_dir_with_turbulence.x,
		0.0,
		wind_dir_with_turbulence.y
	) * fbm_wind_strength * grass_wind_displacement;

	float3 interaction_offset = float3(
		interaction_push_dir.x,
		-0.5,
		interaction_push_dir.y
	) * interaction_strength * grass_interaction_displacement;

	float3 total_offset = wind_offset + interaction_offset;

	float2 facing_dir_xz = facing.xz;
	float2 wind_dir_xz = normalize(float2(wind_dir_with_turbulence.x, wind_dir_with_turbulence.y));

	float alignment = dot(wind_dir_xz, facing_dir_xz);

	float resistance = lerp(0.3, 1.0, abs(alignment));

	float wind_bend_strength = fbm_wind_strength * grass_wind_displacement * resistance;

	float interaction_bend_strength = 0.0;
	float3 interaction_bend_dir = float3(0, 0, 0);
	if (interaction_strength > 0.001)
	{
		float2 interaction_dir_xz = normalize(interaction_push_dir);
		float interaction_alignment = dot(interaction_dir_xz, facing_dir_xz);
		float interaction_resistance = lerp(0.3, 1.0, abs(interaction_alignment));
		interaction_bend_strength = interaction_strength * grass_interaction_displacement * interaction_resistance;
		interaction_bend_dir = float3(interaction_dir_xz.x, 0.0, interaction_dir_xz.y);
	}

	float3 wind_bend_dir = float3(wind_dir_xz.x, 0.0, wind_dir_xz.y);

	float3 total_bend_force = wind_bend_dir * wind_bend_strength + interaction_bend_dir * interaction_bend_strength;
	float total_bend_strength = length(total_bend_force);

	float3 bend_dir = float3(0, 0, 0);
	if (total_bend_strength > 0.001)
	{
		bend_dir = normalize(total_bend_force);
	}

	float max_bend_displacement = blade_height * 0.8;
	float bend_ratio = saturate(total_bend_strength / max_bend_displacement);
	float bend_angle = bend_ratio * 1.2;

	if (total_bend_strength > 0.001)
	{
		float tip_horizontal = blade_height * sin(bend_angle);
		float tip_vertical_drop = blade_height * (1.0 - cos(bend_angle));

		P3 += bend_dir * tip_horizontal;
		P3.y -= tip_vertical_drop;

		float p1_angle = bend_angle * 0.33;
		float p1_horizontal = blade_height * 0.33 * sin(p1_angle);
		float p1_drop = blade_height * 0.33 * (1.0 - cos(p1_angle));

		P1 += bend_dir * p1_horizontal;
		P1.y -= p1_drop;

		float p2_angle = bend_angle * 0.67;
		float p2_horizontal = blade_height * 0.67 * sin(p2_angle);
		float p2_drop = blade_height * 0.67 * (1.0 - cos(p2_angle));

		P2 += bend_dir * p2_horizontal;
		P2.y -= p2_drop;

		float interaction_vertical = interaction.b * grass_interaction_displacement * -0.5;
		if (abs(interaction_vertical) > 0.01)
		{
			P1.y += interaction_vertical * 0.33;
			P2.y += interaction_vertical * 0.67;
			P3.y += interaction_vertical;
		}
	}

	float t = vertex_height_factor;
	float mt = 1.0 - t;
	float mt2 = mt * mt;
	float mt3 = mt2 * mt;
	float t2 = t * t;
	float t3 = t2 * t;

	float3 bezier_pos = mt3 * P0 + 3.0 * mt2 * t * P1 + 3.0 * mt * t2 * P2 + t3 * P3;

	float3 tangent_raw = 3.0 * mt2 * (P1 - P0) + 6.0 * mt * t * (P2 - P1) + 3.0 * t2 * (P3 - P2);
	float tangent_len = length(tangent_raw);
	float3 tangent = (tangent_len > 0.001) ? (tangent_raw / tangent_len) : float3(0, 1, 0);

	float cos_rot = cos(blade_rotation);
	float sin_rot = sin(blade_rotation);

	float3 right = float3(cos_rot, 0.0, -sin_rot);

	float3 width_offset = right * (I.pos.x * det.scale);

	float4 pos = float4(bezier_pos + width_offset, 1.0);

	float hemi = abs(det.hemi);
	float sun = sign(det.hemi) * 0.25f + 0.25f;

	float3 normal_raw = cross(tangent, right);
	float normal_len = length(normal_raw);
	float3 blade_normal = (normal_len > 0.001) ? (normal_raw / normal_len) : facing;

	float rotationAngle = 3.14159 * 0.3;
	float cosTheta = cos(rotationAngle);
	float sinTheta = sin(rotationAngle);
	float3 axis = tangent;
	float3 rotatedNormal1 = blade_normal * cosTheta
	                      + cross(axis, blade_normal) * sinTheta
	                      + axis * dot(axis, blade_normal) * (1.0 - cosTheta);
	float3 rotatedNormal2 = blade_normal * cosTheta
	                      + cross(axis, blade_normal) * (-sinTheta)
	                      + axis * dot(axis, blade_normal) * (1.0 - cosTheta);

	float3 Pe = mul(m_WV, pos);
	float3 view_normal = mul((float3x3)m_WV, blade_normal);
	float3 view_rotatedNormal1 = mul((float3x3)m_WV, rotatedNormal1);
	float3 view_rotatedNormal2 = mul((float3x3)m_WV, rotatedNormal2);

#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
	O.tcdh = float4(I.tc.xy, hemi, sun);
#else
	O.tcdh = I.tc.xy;
#endif

	O.position = float4(Pe, hemi);
	O.N = view_normal;
	O.heightParam = I.t;

	O.rotatedNormal1 = view_rotatedNormal1;
	O.rotatedNormal2 = view_rotatedNormal2;

	O.interaction_uv = atlas_uv;

	O.objectId = det.object_id;

	O.hpos = mul(m_WVP, pos);
	return O;
}
