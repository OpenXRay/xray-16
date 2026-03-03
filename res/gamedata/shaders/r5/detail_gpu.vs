#define SM_6_0
#include "common.h"
#include "bindless_common.h"

// ========================================
// GHOST OF TSUSHIMA-INSPIRED GRASS RENDERING
// ========================================
// This shader implements grass rendering based on the approach described in:
// "Ghost of Tsushima's procedural grass system" (GDC 2021, Eric Wohllaib)
//
// KEY TECHNIQUES IMPLEMENTED:
// 1. GPU-driven instancing with structured buffers (detail_buffer)
// 2. Arc-based blade bending (simplified alternative to Bezier curves)
// 3. Normal calculation from tangent × facing (GoT lines 172-183)
// 4. Height-based AO passed to pixel shader (minimal G-buffer writes)
// 5. Interactive grass with virtual texture indirection
// 6. FBM wind animation with scrolling noise
//
// DIFFERENCES FROM GOT:
// - GoT generates Bezier curves per-blade on GPU; we use pre-generated triangle strips
// - GoT uses compute shader for culling; we use CPU BVH + GPU instancing
// - Same visual result, optimized for X-Ray engine's deferred pipeline
// ========================================

// Detail-specific constant buffer (must match C++ DetailFrameConstants)
cbuffer DetailGlobals : register(b3)
{
    float4 consts;                  // {1/quant,1/quant,diffusescale,ambient}
    float4 wave;                    // cx,cy,cz,tm - for wave animation
    float4 dir2D;                   // Wind direction 1 (XY = direction, normalized)
    float4 dir2D_2;                 // Wind direction 2 (perpendicular)
    float4x4 g_detail_VP;
    float4 detail_params;           // x=slot_x_size, y=slot_z_size, z=slot_x_offs, w=slot_z_offs
    float4 g_wind_direction;        // x=wind angle degrees, y=wind speed, z/w=unused
    float grass_wind_displacement;  // Wind displacement strength
    float grass_interaction_displacement;  // Interaction displacement strength
    uint interaction_atlas_index;   // Bindless index for interaction atlas
    uint perlin4d_texture_index;        // Bindless index for wind texture
    // Grass color parameters (must match pixel shader)
    float4 grass_color_tip;         // RGB + padding (blade tip color)
    float4 grass_color_base;        // RGB + padding (blade base color)
    float4 grass_sss_color;         // RGB + intensity (subsurface scattering)
    float grass_color_variation;    // Per-blade color variation amount
    float grass_blade_height;       // Blade height multiplier (default 1.0)
    float _pad0, _pad1;             // Padding to 16-byte alignment
};

static const float M_PI = 3.1415926;

// Height variation parameters (using FBM noise for natural look)
static const float HEIGHT_VARIATION_MIN = 0.7;   // Minimum height multiplier (70%)
static const float HEIGHT_VARIATION_MAX = 1.15;  // Maximum height multiplier (115%)
static const float HEIGHT_NOISE_SCALE = 0.05;    // World-space frequency of height variation

// Phase 6: Virtual texturing indirection table
// NOTE: common_samplers.h uses t0-t31, so we use t32+ to avoid conflicts
Buffer<uint> slot_indirection : register(t32);  // Packed: physical_page (16) | mip (8) | flags (8)

// Phase 6: New vertex structure for SDF blade geometry
// NOTE: Semantic names must match C++ VertexAttributeDesc in FGDetailManager.cpp
// NVRHI requires different semantic names for different formats
struct v_blade_sdf
{
	float3 pos : POSITION;           // Local blade position (Y-up)
	float2 tc : TEXCOORD;            // Texture coordinates
	float t : COLOR0;                // Height parameter (0-1) for AO
	float width_scale : COLOR1;      // Width at this vertex
};

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

// NOTE: common_samplers.h uses t0-t31, so we use t32+ to avoid conflicts
// Perlin4D 3D volume — bound directly at t12 (not bindless, since bindless is Texture2D only)
Texture3D g_Perlin4D : register(t12);

StructuredBuffer<uint> visible_indices : register(t33);
StructuredBuffer<InstanceData> all_instances : register(t37);
StructuredBuffer<GPUSlotData> slot_data : register(t38);

v2p_flat main(v_blade_sdf I, uint instance_id : SV_InstanceID)
{
	v2p_flat O;

	uint src_idx = visible_indices[instance_id];
	InstanceData raw = all_instances[src_idx];

	struct { float3 pos; float scale; float rotation; uint vis_id; uint object_id; } det;
	det.pos = raw.pos;
	det.object_id = raw.packed & 0x3F;
	det.vis_id = (raw.packed >> 6) & 0x3;
	det.rotation = float((raw.packed >> 8) & 0x3FF) / 1023.0 * TWO_PI;
	det.scale = float((raw.packed >> 18) & 0x3FF) / 1023.0 * PACK_MAX_SCALE;

	// ===== HEIGHT VARIATION USING PERLIN NOISE =====
	// Sample static Perlin noise at different scale/offset for height variation
	// This gives natural-looking height randomization that tiles seamlessly
	float2 height_uv = det.pos.xz * HEIGHT_NOISE_SCALE + float2(0.37, 0.73);  // Offset to decorrelate from wind
	float height_noise = g_Perlin4D.SampleLevel(g_LinearSampler, float3(height_uv, 0), 0).r;  // Use R channel (turbulence)

	// Map noise to height multiplier range
	float height_multiplier = lerp(HEIGHT_VARIATION_MIN, HEIGHT_VARIATION_MAX, height_noise);
	float blade_height = det.scale * height_multiplier * grass_blade_height;

	float3 P0 = det.pos;
	float3 P1 = det.pos + float3(0, blade_height * 0.33, 0);
	float3 P2 = det.pos + float3(0, blade_height * 0.67, 0);
	float3 P3 = det.pos + float3(0, blade_height, 0);

	// Save base position before any modifications for normal calculations
	float3 base_world_pos = P0;

	// ===== CALCULATE FACING DIRECTION (EARLY) =====
	// Use instance rotation from CPU decompression (matches original game's deterministic placement)
	float blade_rotation = det.rotation;
	float3 facing = normalize(float3(sin(blade_rotation), 0.0, cos(blade_rotation)));

	// ===== BEZIER CURVE EVALUATION HELPER =====
	// Cubic Bezier: B(t) = (1-t)³P₀ + 3(1-t)²tP₁ + 3(1-t)t²P₂ + t³P₃
	// GoT doc (lines 60-70): Efficient shader evaluation

	// Phase 6: Use t parameter directly from blade vertex (0=base, 1=tip)
	float vertex_height_factor = I.t;

	// Phase 5: Declare atlas_uv outside scope for pixel shader debug
	float2 atlas_uv = float2(0, 0);

	// Phase 5 + 6: Apply interactive displacement (entity interaction + FBM wind)
	// Replaces old calc_cyclic wave animation with natural FBM wind for all grass types
	// Calculate slot index from world position
	// Detail slot size is 2m (DETAIL_SLOT_SIZE)
	const float slot_size = 2.0;
	int slot_x = int(floor(base_world_pos.x / slot_size));
	int slot_z = int(floor(base_world_pos.z / slot_size));

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
		float2 slot_local = frac(base_world_pos.xz / slot_size);

		// Final atlas UV
		atlas_uv = page_base_uv + slot_local * page_size_uv;

		// Sample interaction atlas using bindless
		Texture2D interaction_tex = GetBindlessTexture(interaction_atlas_index);
		interaction = interaction_tex.SampleLevel(g_LinearSampler, atlas_uv, 0);
	}

	// ===== MODIFY BEZIER CONTROL POINTS FOR WIND/INTERACTION =====
	// GoT doc (lines 391-405): "Wind animation integration"
	// Instead of directly offsetting vertices, we modify P1 and P2

	// ===== WIND ANIMATION (Based on grass_example_repo reference) =====
	// Sample Perlin noise for direction, strength, and turbulence separately
	// Key: Scale inversely with wind_speed, time scroll proportionally
	float wind_speed = max(g_wind_direction.y, 0.1);  // Avoid division by zero
	float time = wave.w;  // Global time

	// Wind DIRECTION sample - larger scale, slower movement
	// Reference: pos.zx * 0.005/wind_speed + TIME * 0.005 * wind_speed
	float2 dir_uv = P0.zx * (0.005 / wind_speed) + time * (0.005 * wind_speed);
	float wind_dir_noise = g_Perlin4D.SampleLevel(g_LinearSampler, float3(dir_uv, 0), 0).r;

	// Wind STRENGTH sample - smaller scale, faster movement
	// Reference: pos.xz * 0.025/wind_speed + TIME * 0.05
	float2 str_uv = P0.xz * (0.025 / wind_speed) + time * 0.05;
	float wind_str_noise = g_Perlin4D.SampleLevel(g_LinearSampler, float3(str_uv, 0), 0).r;

	// Wind TURBULENCE sample - same as strength but with HEIGHT-based time offset
	// This makes blade tips flutter at different phase than base
	// Reference: pos.xz * 0.025/wind_speed + (TIME + height_factor² * 0.25) * 0.05
	float height_factor = vertex_height_factor;  // 0 at base, 1 at tip
	float2 turb_uv = P0.xz * (0.025 / wind_speed) + (time + height_factor * height_factor * 0.25) * 0.05;
	float wind_turb_noise = g_Perlin4D.SampleLevel(g_LinearSampler, float3(turb_uv, 0), 0).r;

	// Process strength: remap to [0.25, 1.0], square for contrast, scale by wind_speed
	float fbm_wind_strength = lerp(0.25, 1.0, wind_str_noise);
	fbm_wind_strength *= fbm_wind_strength;  // Square for more contrast
	fbm_wind_strength *= wind_speed;

	// Process turbulence: same processing as strength
	float wind_turbulence = lerp(0.25, 1.0, wind_turb_noise);
	wind_turbulence *= wind_turbulence;
	wind_turbulence *= min(wind_speed, 1.0) * 0.3;  // Cap turbulence contribution

	// Direction perturbation from noise (±30% variation)
	float fbm_turbulence = (wind_dir_noise * 2.0 - 1.0) * 0.3;

	// Calculate global wind direction from g_wind_direction
	float wind_angle_rad = g_wind_direction.x * (M_PI / 180.0);
	float2 global_wind_dir = float2(sin(wind_angle_rad), cos(wind_angle_rad));

	// Add turbulence perpendicular to wind direction
	float2 perpendicular_dir = float2(-global_wind_dir.y, global_wind_dir.x);
	float2 wind_dir_with_turbulence = normalize(global_wind_dir + perpendicular_dir * fbm_turbulence);

	// Get interaction data
	float2 interaction_push_dir = interaction.rg * 2.0 - 1.0;
	float interaction_strength = length(interaction_push_dir);
	if (interaction_strength > 0.001)
	{
		interaction_push_dir = interaction_push_dir / interaction_strength;
	}

	// ===== MODIFY CONTROL POINTS =====
	// GoT approach: apply wind/interaction to P1 and P2 (not P0 or P3)
	// Higher control points bend more (parabolic distribution)

	// Calculate displacement for control points
	// Wind direction comes from g_wind_direction, strength from FBM texture
	float3 wind_offset = float3(
		wind_dir_with_turbulence.x,
		0.0,  // Wind is horizontal
		wind_dir_with_turbulence.y
	) * fbm_wind_strength * grass_wind_displacement;

	float3 interaction_offset = float3(
		interaction_push_dir.x,
		-0.5,  // Grass bends down when trampled
		interaction_push_dir.y
	) * interaction_strength * grass_interaction_displacement;

	// Combine wind and interaction
	float3 total_offset = wind_offset + interaction_offset;

	// ===== BEZIER CURVE BENDING: ALL GRASS BENDS WITH WIND =====
	// Physics: Wind pushes ALL grass in the wind direction, regardless of facing
	// Facing only affects HOW MUCH the grass bends (resistance), not WHICH WAY
	//
	// Examples (wind blowing EAST):
	//   - Blade facing EAST:  low resistance → bends easily eastward
	//   - Blade facing WEST:  high resistance → bends less, but still eastward
	//   - Blade facing NORTH: medium resistance → moderate eastward bend
	//
	// All grass ultimately bends in the wind direction!

	float2 facing_dir_xz = facing.xz;  // Blade's facing direction (normalized)
	float2 wind_dir_xz = normalize(float2(wind_dir_with_turbulence.x, wind_dir_with_turbulence.y));

	// Calculate alignment: dot product between blade facing and wind direction
	// Result: +1 = aligned with wind, 0 = perpendicular, -1 = against wind
	float alignment = dot(wind_dir_xz, facing_dir_xz);

	// Resistance factor: grass facing INTO wind bends more easily than grass facing AWAY
	// We use abs(alignment) so perpendicular grass has medium resistance
	// Scale from 0.3 (high resistance, facing away) to 1.0 (low resistance, aligned)
	float resistance = lerp(0.3, 1.0, abs(alignment));

	// Wind influence: ALL grass bends in wind direction, modulated by resistance
	float wind_bend_strength = fbm_wind_strength * grass_wind_displacement * resistance;

	// Interaction: also bends in push direction with resistance
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

	// Wind bend direction (all grass bends WITH the wind)
	float3 wind_bend_dir = float3(wind_dir_xz.x, 0.0, wind_dir_xz.y);

	// Combine wind and interaction bending
	float3 total_bend_force = wind_bend_dir * wind_bend_strength + interaction_bend_dir * interaction_bend_strength;
	float total_bend_strength = length(total_bend_force);

	// Normalize to get final bend direction
	float3 bend_dir = float3(0, 0, 0);
	if (total_bend_strength > 0.001)
	{
		bend_dir = normalize(total_bend_force);
	}

	// Calculate bend angle with height-based turbulence
	// Reference: blade tips flutter independently due to height-based time offset in noise sampling
	float max_bend_displacement = blade_height * 0.8;
	float bend_ratio = saturate(total_bend_strength / max_bend_displacement);
	float base_bend_angle = bend_ratio * 1.2;  // Max ~70 degrees
	// Add turbulence that increases with height (blade tips flutter more)
	float bend_angle = base_bend_angle + wind_turbulence * vertex_height_factor;

	if (total_bend_strength > 0.001)
	{
		// P3 (tip): Position it at the end of the arc
		float tip_horizontal = blade_height * sin(bend_angle);
		float tip_vertical_drop = blade_height * (1.0 - cos(bend_angle));

		P3 += bend_dir * tip_horizontal;
		P3.y -= tip_vertical_drop;

		// P1 (lower control point): 1/3 along arc
		float p1_angle = bend_angle * 0.33;
		float p1_horizontal = blade_height * 0.33 * sin(p1_angle);
		float p1_drop = blade_height * 0.33 * (1.0 - cos(p1_angle));

		P1 += bend_dir * p1_horizontal;
		P1.y -= p1_drop;

		// P2 (upper control point): 2/3 along arc
		float p2_angle = bend_angle * 0.67;
		float p2_horizontal = blade_height * 0.67 * sin(p2_angle);
		float p2_drop = blade_height * 0.67 * (1.0 - cos(p2_angle));

		P2 += bend_dir * p2_horizontal;
		P2.y -= p2_drop;

		// Add interaction vertical offset if present (trampling pushes down)
		float interaction_vertical = interaction.b * grass_interaction_displacement * -0.5;
		if (abs(interaction_vertical) > 0.01)
		{
			P1.y += interaction_vertical * 0.33;
			P2.y += interaction_vertical * 0.67;
			P3.y += interaction_vertical;
		}
	}

	// ===== EVALUATE BEZIER CURVE TO GET FINAL VERTEX POSITION =====
	// Cubic Bezier formula at parameter t
	float t = vertex_height_factor;  // 0 at base, 1 at tip
	float mt = 1.0 - t;
	float mt2 = mt * mt;
	float mt3 = mt2 * mt;
	float t2 = t * t;
	float t3 = t2 * t;

	// B(t) = (1-t)³P₀ + 3(1-t)²tP₁ + 3(1-t)t²P₂ + t³P₃
	float3 bezier_pos = mt3 * P0 + 3.0 * mt2 * t * P1 + 3.0 * mt * t2 * P2 + t3 * P3;

	// ===== CALCULATE TANGENT =====
	// TANGENT = Bezier curve derivative at parameter t
	// B'(t) = 3(1-t)²(P₁-P₀) + 6(1-t)t(P₂-P₁) + 3t²(P₃-P₂)
	// GoT doc (lines 72-76): "The first derivative (tangent vector)"
	float3 tangent_raw = 3.0 * mt2 * (P1 - P0) + 6.0 * mt * t * (P2 - P1) + 3.0 * t2 * (P3 - P2);
	float tangent_len = length(tangent_raw);
	float3 tangent = (tangent_len > 0.001) ? (tangent_raw / tangent_len) : float3(0, 1, 0);

	// ===== WIDTH OFFSET USING ROTATION MATRIX (like original X-Ray shader) =====
	// Original X-Ray detail shader transforms vertices using a per-instance matrix.
	// We replicate this by building a Y-axis rotation matrix from det.rotation.
	// The vertex's local X position (I.pos.x) represents blade width offset.

	// Build rotation matrix for Y-axis rotation (blade_rotation is in radians)
	float cos_rot = cos(blade_rotation);
	float sin_rot = sin(blade_rotation);

	// The "right" vector is simply the rotated X-axis
	// For Y-axis rotation: right = (cos, 0, -sin)
	float3 right = float3(cos_rot, 0.0, -sin_rot);

	float3 width_offset = right * (I.pos.x * det.scale);

	// Final position
	float4 pos = float4(bezier_pos + width_offset, 1.0);

	// ===== Calculate blade normal =====
	float slot_hemi = slot_data[slot_idx].hemi;
	float hemi = abs(slot_hemi);
	float sun = sign(slot_hemi) * 0.25f + 0.25f;

	// NORMAL = perpendicular to blade surface = cross(tangent, right)
	// Since right is derived from rotation matrix, this is stable
	float3 normal_raw = cross(tangent, right);
	float normal_len = length(normal_raw);
	// Fallback to facing direction if cross product is degenerate
	float3 blade_normal = (normal_len > 0.001) ? (normal_raw / normal_len) : facing;

	// ===== GHOST OF TSUSHIMA: ROUNDED BLADE NORMALS =====
	float rotationAngle = 3.14159 * 0.3;  // ±30 degrees (PI * 0.3)
	float cosTheta = cos(rotationAngle);
	float sinTheta = sin(rotationAngle);
	float3 axis = tangent;
	float3 rotatedNormal1 = blade_normal * cosTheta
	                      + cross(axis, blade_normal) * sinTheta
	                      + axis * dot(axis, blade_normal) * (1.0 - cosTheta);
	float3 rotatedNormal2 = blade_normal * cosTheta
	                      + cross(axis, blade_normal) * (-sinTheta)
	                      + axis * dot(axis, blade_normal) * (1.0 - cosTheta);

#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
	O.tcdh = float4(I.tc.xy, hemi, sun);
#else
	O.tcdh = I.tc.xy;
#endif

	O.position = float4(pos.xyz, hemi);
	O.N = blade_normal;
	O.heightParam = I.t;
	O.rotatedNormal1 = rotatedNormal1;
	O.rotatedNormal2 = rotatedNormal2;
	O.interaction_uv = atlas_uv;
	O.objectId = det.object_id;
	int2 bc = int2(floor(det.pos.xz));
	uint bh = asuint(bc.x * 73856093 + bc.y * 19349663);
	bh ^= bh >> 16;
	O.bladeHash = float(bh & 0xFFFFu) / 65535.0;
	O.hpos = mul(g_detail_VP, pos);
	return O;
}
