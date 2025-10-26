#include "common.h"

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

uniform float4 consts; // {1/quant,1/quant,diffusescale,ambient}
uniform float4 wave;   // cx,cy,cz,tm - for wave1
uniform float4 dir2D;  // dir1 - for wave1 (vis_id=1)
uniform float4 dir2D_2; // dir2 - for wave2 (vis_id=2)
uniform float4 detail_params; // Phase 5: x=slot_x_size, y=slot_z_size, z=slot_x_offs, w=slot_z_offs
uniform float grass_wind_displacement; // Phase 5: Wind displacement strength (tunable via ImGui)
uniform float grass_interaction_displacement; // Phase 5: Interaction displacement strength (tunable via ImGui)
uniform float4 g_wind_direction; // Phase 6: Wind angle in degrees (X component only) WindParams
static const float M_PI = 3.1415926;

// Phase 5: Interactive grass textures
// Note: Slot t0 = instance buffer, so we use t1, t2, t3 (engine limit is 4 VS texture slots)
Texture2D interaction_atlas : register(t1);  // RG=displacement XZ, B=bend, A=age
Texture2D wind_texture : register(t2);       // RGB=wind vector, A=strength
// Phase 6: Virtual texturing indirection table (NEW)
Buffer<uint> slot_indirection : register(t3);  // Packed: physical_page (16) | mip (8) | flags (8)

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
	float3 pos;      // Position (12 bytes)
	float scale;     // Scale factor (4 bytes)
	float hemi;      // Hemisphere lighting (4 bytes)
	uint vis_id;     // Visibility/animation type (0=still, 1=wave1, 2=wave2) (4 bytes)
	uint object_id;  // Which grass object type (0-63) (4 bytes)
	float padding;   // Padding to align to 16 bytes (4 bytes) = 32 bytes total
};

// Structured buffer bound to slot 0
StructuredBuffer<InstanceData> detail_buffer : register(t0);

v2p_flat main(v_blade_sdf I, uint instance_id : SV_InstanceID)
{
	v2p_flat O;

	// Read instance data from structured buffer
	InstanceData det = detail_buffer[instance_id];

	float3 P0 = det.pos;
	float3 P1 = det.pos + float3(0, det.scale * 0.33, 0);
	float3 P2 = det.pos + float3(0, det.scale * 0.67, 0);
	float3 P3 = det.pos + float3(0, det.scale, 0);

	// Save base position before any modifications for normal calculations
	float3 base_world_pos = P0;

	// ===== CALCULATE FACING DIRECTION (EARLY) =====
	// Each blade gets a unique random rotation based on its position
	// This must be calculated early since we need it for wind influence calculation
	float blade_rotation = frac(P0.x * 12.9898 + P0.z * 78.233) * 2.0 * M_PI;  // Random 0-2π
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

		// Sample interaction atlas
		interaction = interaction_atlas.SampleLevel(smp_linear, atlas_uv, 0);
	}

	// ===== MODIFY BEZIER CONTROL POINTS FOR WIND/INTERACTION =====
	// GoT doc (lines 391-405): "Wind animation integration"
	// Instead of directly offsetting vertices, we modify P1 and P2

	// Sample wind texture at blade base for strength/turbulence variation
	float2 wind_uv = P0.xz * 0.001;  // Use base position for sampling
	float4 wind = wind_texture.SampleLevel(smp_linear, wind_uv, 0);

	// Extract wind strength and turbulence from texture
	// wind.a = base wind strength (0-1)
	// wind.r = turbulence variation (0-1)
	float fbm_wind_strength = wind.a;
	float fbm_turbulence = (wind.r * 2.0 - 1.0) * 0.3;  // ±30% turbulence

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

	// Calculate bend angle
	float max_bend_displacement = det.scale * 0.8;
	float bend_ratio = saturate(total_bend_strength / max_bend_displacement);
	float bend_angle = bend_ratio * 1.2;  // Max ~70 degrees

	if (total_bend_strength > 0.001)
	{
		// P3 (tip): Position it at the end of the arc
		float tip_horizontal = det.scale * sin(bend_angle);
		float tip_vertical_drop = det.scale * (1.0 - cos(bend_angle));

		P3 += bend_dir * tip_horizontal;
		P3.y -= tip_vertical_drop;

		// P1 (lower control point): 1/3 along arc
		float p1_angle = bend_angle * 0.33;
		float p1_horizontal = det.scale * 0.33 * sin(p1_angle);
		float p1_drop = det.scale * 0.33 * (1.0 - cos(p1_angle));

		P1 += bend_dir * p1_horizontal;
		P1.y -= p1_drop;

		// P2 (upper control point): 2/3 along arc
		float p2_angle = bend_angle * 0.67;
		float p2_horizontal = det.scale * 0.67 * sin(p2_angle);
		float p2_drop = det.scale * 0.67 * (1.0 - cos(p2_angle));

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
	float3 tangent = 3.0 * mt2 * (P1 - P0) + 6.0 * mt * t * (P2 - P1) + 3.0 * t2 * (P3 - P2);
	tangent = normalize(tangent);

	// ===== GHOST OF TSUSHIMA: GLANCING ANGLE ADJUSTMENT =====
	// GoT doc (lines 185-186): "vertices rotate slightly about the tangent to maintain visibility"
	// When blade is perpendicular to camera, rotate it to face the camera

	// Calculate view direction
	float3 view_dir = normalize(bezier_pos - eye_position);

	// Calculate right vector for blade width
	// Normally this would be perpendicular to tangent and facing
	float3 right = normalize(cross(tangent, facing));

	// Calculate how edge-on the blade is to the camera
	// dot(view_dir, right) = 0 when camera is perpendicular to blade face
	// dot(view_dir, right) = ±1 when camera is aligned with blade face
	float edge_on_factor = abs(dot(view_dir, right));

	// When edge_on_factor is close to 1 (edge-on), rotate blade to face camera
	// Blend the right vector toward camera-facing direction
	if (edge_on_factor > 0.8)  // Only adjust at steep angles
	{
		// Camera-facing right vector (perpendicular to tangent and view)
		float3 camera_right = normalize(cross(tangent, view_dir));

		// Blend between natural right and camera-facing right
		// More edge-on = more camera-facing
		float blend = (edge_on_factor - 0.8) / 0.2;  // 0.8-1.0 -> 0-1
		right = normalize(lerp(right, camera_right, blend * 0.5));  // Max 50% blend
	}

	// Apply width offset using adjusted right vector
	float3 width_offset = right * (I.pos.x * det.scale);

	// Final position
	float4 pos = float4(bezier_pos + width_offset, 1.0);

	// ===== GHOST OF TSUSHIMA: Calculate blade normal =====
	float hemi = abs(det.hemi);
	float sun = sign(det.hemi) * 0.25f + 0.25f;

	// NORMAL = perpendicular to blade surface
	// Ghost of Tsushima: normal = cross(tangent, right)
	// Note: We use the adjusted 'right' vector from glancing angle adjustment
	// This ensures normals match the adjusted blade geometry
	float3 blade_normal = normalize(cross(tangent, right));

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

	// Transform to view space
	float3 Pe = mul(m_WV, pos);
	float3 view_normal = mul((float3x3)m_WV, blade_normal);
	float3 view_rotatedNormal1 = mul((float3x3)m_WV, rotatedNormal1);
	float3 view_rotatedNormal2 = mul((float3x3)m_WV, rotatedNormal2);

	// Pack output
	// Standard X-Ray approach for v2p_flat:
	// - tcdh.xy = texture coordinates
	// - tcdh.zw = hemi, sun (only if USE_R2_STATIC_SUN && !USE_LM_HEMI)
	// - position.xyz = view-space position
	// - position.w = hemi
#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
	O.tcdh = float4(I.tc.xy, hemi, sun);
#else
	O.tcdh = I.tc.xy;
#endif

	O.position = float4(Pe, hemi);  // Pack hemi in position.w
	O.N = view_normal;
	O.heightParam = I.t;  // Phase 6: Pass height parameter for AO calculation

	// Phase 6: Pass rotated normals for rounded blade effect
	O.rotatedNormal1 = view_rotatedNormal1;
	O.rotatedNormal2 = view_rotatedNormal2;

	// Phase 5: Pass atlas UV for grass interaction debug visualization
	O.interaction_uv = atlas_uv;  // Pass the correct atlas UV computed with indirection table

	O.hpos = mul(m_WVP, pos);
	return O;
}
FXVS;
