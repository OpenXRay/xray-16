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

// Phase 6: New vertex structure for SDF blade geometry
struct v_blade_sdf
{
	float3 pos : POSITION;           // Local blade position (Y-up)
	float2 tc : TEXCOORD0;           // Texture coordinates
	float t : TEXCOORD1;             // Height parameter (0-1) for AO
	float width_scale : TEXCOORD2;   // Width at this vertex
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

v2p_flat main(v_blade_sdf I, uint instance_id : SV_InstanceID)
{
	v2p_flat O;

	// Read instance data from structured buffer
	InstanceData det = detail_buffer[instance_id];

	// ===== PHASE 6: Transform blade to world space =====

	// Construct rotation matrix from instance data
	float3x3 rotation = float3x3(det.m0, det.m1, det.m2);

	// Scale blade by instance scale
	float3 local_pos = I.pos * det.scale;

	// Rotate to instance orientation
	float3 rotated_pos = mul(rotation, local_pos);

	// Translate to world position
	float4 pos = float4(rotated_pos + det.pos, 1.0);

	// Phase 5: Declare atlas_uv outside scope for pixel shader debug
	float2 atlas_uv = float2(0, 0);

	float2 wind_displacement = float2(0, 0);
	// Phase 6: Use t parameter directly from blade vertex (0=base, 1=tip)
	float vertex_height_factor = I.t;

	// Phase 5 + 6: Apply interactive displacement (entity interaction + FBM wind)
	// Replaces old calc_cyclic wave animation with natural FBM wind for all grass types
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

    // === INTERACTION BENDING (trampling/pushing) ===
    // RG channels contain the direction the grass should bend
    float2 interaction_push_dir = interaction.rg * 2.0 - 1.0;  // Decode from [0,1] to [-1,1]
    float interaction_strength = length(interaction_push_dir);

    if (interaction_strength > 0.01)
    {
        // Normalize push direction
		float push_dir_length = length(interaction_push_dir);
		if (push_dir_length > 0.001)
		{
			interaction_push_dir = interaction_push_dir / push_dir_length;
		}
		else
		{
			interaction_push_dir = float2(0, 1); // Default direction
		}

        // Bend factor: 0 at base (no movement), increasing to 1 at tip
        // Use cubic curve for more natural grass bend (tips bend easier)
        float bend_curve = vertex_height_factor * vertex_height_factor * vertex_height_factor;

        // CRITICAL: Limit maximum bend angle to prevent flat grass
        // Even heavily trampled grass maintains ~30-45 degree angle
        float max_bend_angle = 60.0 * 3.14159 / 180.0;  // 60 degrees max
        float actual_bend = interaction_strength * max_bend_angle;

        // Calculate arc-based displacement (maintains natural curve)
        float blade_length = det.scale;  // Actual blade height

        // Horizontal displacement follows arc
        float horizontal_bend = blade_length * sin(actual_bend) * bend_curve;
        pos.xz += interaction_push_dir * horizontal_bend * grass_interaction_displacement;

        // Vertical drop follows arc (maintains height even when bent)
        // cos(60°) = 0.5, so grass at 60 degrees still retains 50% height
        float vertical_drop = blade_length * (1.0 - cos(actual_bend)) * bend_curve;
        pos.y -= vertical_drop * grass_interaction_displacement;
    }

    // === WIND ANIMATION ===
    // Wind applies similarly but with gentler curve
    float wind_bend_curve = vertex_height_factor * vertex_height_factor;

    // Horizontal wind sway
    wind_displacement = wind_direction * wind_strength * wind_bend_curve * grass_wind_displacement;
    pos.xz += wind_displacement;

    // Vertical oscillation from wind (grass waves gently up/down)
    float wind_vertical_wave = sin(wind_strength * 6.28318) * 0.05 * wind_bend_curve;
    pos.y += wind_vertical_wave * det.scale * grass_wind_displacement;

	// ===== PHASE 6: Calculate proper blade normal =====

	float hemi = abs(det.hemi);
	float sun = sign(det.hemi) * 0.25f + 0.25f;

	// Blade tangent (up direction after rotation)
	float3 blade_up = normalize(rotation[1]);  // Y-axis of rotation matrix

	// Blade right (across width)
	float3 blade_right = normalize(rotation[0]);  // X-axis of rotation matrix

	// Normal points forward (perpendicular to blade surface)
	float3 blade_normal = normalize(cross(blade_up, blade_right));

	// === CRITICAL: Ensure normal faces camera for edge-on blades ===
	// This prevents dark edges when viewing grass blades side-on
	float3 view_dir = normalize(det.pos - float3(eye_position.xyz));
	float facing = dot(blade_normal, view_dir);

	// Flip normal if facing away from camera (two-sided lighting helper)
	if (facing > 0.0)
	{
		blade_normal = -blade_normal;
	}

	// Adjust normal based on wind bend (more realistic)
	if (length(wind_displacement) > 0.01)
	{
		float3 wind_dir_3d = normalize(float3(wind_displacement.x, 0, wind_displacement.y));

		// Blend normal toward wind direction at blade tips
		float wind_influence = vertex_height_factor * vertex_height_factor;  // Quadratic falloff
		blade_normal = normalize(lerp(blade_normal, wind_dir_3d, wind_influence * 0.4));
	}

	// ===== OUTPUT =====

	// Transform to view space
	float3 Pe = mul(m_WV, pos);
	float3 view_normal = mul((float3x3)m_WV, blade_normal);

	// Pack output
	O.tcdh = float4(I.tc.xy, hemi, sun);
	O.position = float4(Pe, 1.0f);
	O.N = view_normal;
	O.heightParam = I.t;  // Phase 6: Pass height parameter for AO calculation

	// Phase 5: Pass atlas UV for grass interaction debug visualization
	O.interaction_uv = atlas_uv;  // Pass the correct atlas UV computed with indirection table

	O.hpos = mul(m_WVP, pos);
	return O;
}
FXVS;
