#include "common.h"

uniform float4 consts; // {1/quant,1/quant,diffusescale,ambient}
uniform float4 wave;   // cx,cy,cz,tm - for wave1
uniform float4 dir2D;  // dir1 - for wave1 (vis_id=1)
uniform float4 dir2D_2; // dir2 - for wave2 (vis_id=2)
uniform float4 detail_params; // Phase 5: x=slot_x_size, y=slot_z_size, z=slot_x_offs, w=slot_z_offs
uniform float grass_wind_displacement; // Phase 5: Wind displacement strength (tunable via ImGui)
uniform float grass_interaction_displacement; // Phase 5: Interaction displacement strength (tunable via ImGui)

// Phase 5: Interactive grass textures
// Note: Slot t0 = instance buffer, so we use t1, t2, t3 (engine limit is 4 VS texture slots)
Texture2D interaction_atlas : register(t1);  // RG=displacement XZ, B=bend, A=age
Texture2D wind_texture : register(t2);       // RGB=wind vector, A=strength
// Phase 6: Virtual texturing indirection table (NEW)
Buffer<uint> slot_indirection : register(t3);  // Packed: physical_page (16) | mip (8) | flags (8)
SamplerState interaction_sampler : register(s0);

// New vertex structure for instanced details
struct v_detail_instanced
{
	float4 pos : POSITION;    // position.xyz, frac (normalized height) in w
	float2 tc  : TEXCOORD0;   // texture coordinates
};

// Instance data structure (must match C++ InstanceData)
struct InstanceData
{
	float3 m0;       // First column of rotation matrix (X-axis)
	float scale;     // Scale factor
	float3 m1;       // Second column of rotation matrix (Y-axis)
	float hemi;      // Hemisphere lighting
	float3 m2;       // Third column of rotation matrix (Z-axis)
	uint vis_id;     // Visibility/animation type (0=still, 1=wave1, 2=wave2)
	float3 pos;      // Position
	uint object_id;  // Which grass object type (0-63)
};

// Structured buffer bound to slot 0
StructuredBuffer<InstanceData> detail_buffer : register(t0);

v2p_flat main(v_detail_instanced I, uint instance_id : SV_InstanceID)
{
	v2p_flat O;

	// Read instance data from structured buffer
	InstanceData det = detail_buffer[instance_id];

	// Use matrix columns directly (no reconstruction needed!)
	float3x3 mmhpb = float3x3(det.m0, det.m1, det.m2);

	float hemi = abs(det.hemi);
	float sun = sign(det.hemi) * 0.25f + 0.25f;

	float4 m0 = float4(mmhpb[0] * det.scale, det.pos.x);
	float4 m1 = float4(mmhpb[1] * det.scale, det.pos.y);
	float4 m2 = float4(mmhpb[2] * det.scale, det.pos.z);

	float4 pos;
	pos.x = dot(m0, float4(I.pos.xyz, 1.0));
	pos.y = dot(m1, float4(I.pos.xyz, 1.0));
	pos.z = dot(m2, float4(I.pos.xyz, 1.0));
	pos.w = 1.0f;

	// Phase 5: Declare atlas_uv outside scope for pixel shader debug
	float2 atlas_uv = float2(0, 0);

	// Phase 5: Apply interactive displacement (entity interaction + FBM wind)
	// Replaces old calc_cyclic wave animation with natural FBM wind for all grass types
	{
		// Calculate vertex height factor (0 at base, 1 at top)
		float vertex_height = I.pos.y * length(m1.xyz);
		float vertex_height_factor = saturate(vertex_height / det.scale);

		// Calculate slot index from world position
		// Detail slot size is 2m (DETAIL_SLOT_SIZE)
		const float slot_size = 2.0;
		int slot_x = int(floor(pos.x / slot_size));
		int slot_z = int(floor(pos.z / slot_size));

		// Map slot coordinates to slot array index
		// Slots are stored in row-major order: (sz + z_offs) * x_size + (sx + x_offs)
		uint x_size = uint(detail_params.x);
		uint z_size = uint(detail_params.y);
		int x_offs = int(detail_params.z);
		int z_offs = int(detail_params.w);

		// Bounds check and compute index
		int sx_local = slot_x + x_offs;
		int sz_local = slot_z + z_offs;

		// Clamp to valid range
		sx_local = clamp(sx_local, 0, int(x_size) - 1);
		sz_local = clamp(sz_local, 0, int(z_size) - 1);

		uint slot_idx = uint(sz_local) * x_size + uint(sx_local);

		// Phase 6: Virtual texturing with indirection table
		// No more modulo - use proper indirection lookup

		// Lookup indirection table entry
		uint packed_indirection = slot_indirection.Load(slot_idx);
		uint physical_page = packed_indirection & 0xFFFF;  // Lower 16 bits
		uint mip_level = (packed_indirection >> 16) & 0xFF;  // Bits 16-23

		float4 interaction;

		// Check if page is resident
		if (physical_page == 0xFFFF) {
			// Slot not resident in atlas - use fallback (no interaction)
			interaction = float4(0, 0, 0, 0);
		} else {
			// Compute atlas UV from physical page index
			const uint pages_per_row = 64;  // 2048 / 32 = 64 pages per row
			uint page_x = physical_page % pages_per_row;
			uint page_y = physical_page / pages_per_row;

			// Each page is 32×32 pixels in a 2048×2048 atlas
			const float page_size_uv = 32.0 / 2048.0;  // 0.015625

			// Base UV for this page
			float2 page_base_uv = float2(page_x, page_y) * page_size_uv;

			// Local position within slot (0-1)
			float2 slot_local = frac(pos.xz / slot_size);

			// Final atlas UV
			atlas_uv = page_base_uv + slot_local * page_size_uv;

			// Sample interaction atlas
			interaction = interaction_atlas.SampleLevel(interaction_sampler, atlas_uv, 0);
		}

		// Sample wind texture (world-space tiling)
		float2 wind_uv = pos.xz * 0.001;  // 1km tile size
		float4 wind = wind_texture.SampleLevel(interaction_sampler, wind_uv, 0);

		// Decode wind direction from [0,1] to [-1,1]
		float2 wind_direction = wind.xz * 2.0 - 1.0;
		float wind_strength = wind.a;

		// Apply interaction displacement (radial push from entities)
		float2 interaction_displacement = interaction.rg;
		pos.xz += interaction_displacement * grass_interaction_displacement;

		// Apply vertical displacement (grass bends DOWN when trampled)
		float bend_amount = interaction.b;  // 0-1, from compute shader
		pos.y -= bend_amount * 0.6 * det.scale * grass_interaction_displacement;

		// Apply wind displacement
		float2 wind_displacement = wind_direction * wind_strength * vertex_height_factor * grass_wind_displacement;
		pos.xz += wind_displacement;
	}

	float3 Pe = mul(m_WV, pos);

	float3 N;
	N.x = pos.x - m0.w;
	N.y = pos.y - m1.w + 0.75f;
	N.z = pos.z - m2.w;

	O.tcdh = float4(I.tc.xy, hemi, sun);
	O.position = float4(Pe, 1.0f);

	N.xyz = mul((float3x3)m_WV, N.xyz);
	O.N = N.xyz;

	// Phase 5: Pass atlas UV for grass interaction debug visualization
	O.interaction_uv = atlas_uv;  // Pass the correct atlas UV computed with indirection table

	O.hpos = mul(m_WVP, pos);

	return O;
}
FXVS;
