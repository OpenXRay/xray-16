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
	float2 fbm_wind_factor = wind.xz * 2.0 - 1.0;
	float fbm_wind_strength = wind.a;

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
	// P1 gets 30% of the effect, P2 gets 70% (tips bend more)
	float3 wind_offset = float3(
		fbm_wind_factor.x,
		0.0,  // Wind is horizontal
		fbm_wind_factor.y
	) * fbm_wind_strength * grass_wind_displacement;

	float3 interaction_offset = float3(
		interaction_push_dir.x,
		-0.5,  // Grass bends down when trampled
		interaction_push_dir.y
	) * interaction_strength * grass_interaction_displacement;

	// Combine wind and interaction
	float3 total_offset = wind_offset + interaction_offset;

	// ===== BEZIER CURVE BENDING: TIP FOLLOWS WIND =====
	// Key: P3 (tip) should point TOWARD the wind/interaction direction
	// P1, P2 create the arc from P0 (base) to P3 (tip)

	// Calculate total bend amount and direction
	float horizontal_bend = length(total_offset.xz);

	if (horizontal_bend > 0.001)
	{
		// Normalize bend direction (where grass should point)
		float2 bend_dir_xz = total_offset.xz / horizontal_bend;
		float3 bend_dir_3d = float3(bend_dir_xz.x, 0.0, bend_dir_xz.y);

		// Calculate how much the blade should bend (angle in radians)
		// More wind/interaction = more bending
		float max_bend_displacement = det.scale * 0.8;  // Blade can bend up to 80% of height horizontally
		float bend_ratio = saturate(horizontal_bend / max_bend_displacement);
		float bend_angle = bend_ratio * 1.2;  // Max ~70 degrees

		// P3 (tip): Position it at the end of the arc
		// Horizontal: Full displacement in wind direction
		// Vertical: Natural drop due to arc geometry
		float tip_horizontal = det.scale * sin(bend_angle);
		float tip_vertical_drop = det.scale * (1.0 - cos(bend_angle));

		P3.x += bend_dir_3d.x * tip_horizontal;
		P3.y -= tip_vertical_drop;
		P3.z += bend_dir_3d.z * tip_horizontal;

		// P1 (lower control point): 1/3 along arc from base to bent tip
		// Creates smooth curve
		float p1_angle = bend_angle * 0.33;
		float p1_horizontal = det.scale * 0.33 * sin(p1_angle);
		float p1_drop = det.scale * 0.33 * (1.0 - cos(p1_angle));

		P1.x += bend_dir_3d.x * p1_horizontal;
		P1.y -= p1_drop;
		P1.z += bend_dir_3d.z * p1_horizontal;

		// P2 (upper control point): 2/3 along arc from base to bent tip
		float p2_angle = bend_angle * 0.67;
		float p2_horizontal = det.scale * 0.67 * sin(p2_angle);
		float p2_drop = det.scale * 0.67 * (1.0 - cos(p2_angle));

		P2.x += bend_dir_3d.x * p2_horizontal;
		P2.y -= p2_drop;
		P2.z += bend_dir_3d.z * p2_horizontal;

		// Add interaction vertical offset if present (trampling pushes down)
		if (total_offset.y < -0.01)
		{
			P1.y += total_offset.y * 0.33;
			P2.y += total_offset.y * 0.67;
			P3.y += total_offset.y;
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

	// ===== CALCULATE FACING DIRECTION =====
	// FACING = blade orientation in world space (perpendicular to blade width)
	// GoT doc (lines 178-179): facing is a fixed per-blade direction, NOT derived from tangent
	// This ensures normals remain stable even when the blade bends heavily
	// Use global wind direction (all grass faces the same way in procedural system)
	float wind_angle_rad = g_wind_direction.x * (M_PI / 180.0);
	float3 facing = normalize(float3(sin(wind_angle_rad), 0.0, cos(wind_angle_rad)));

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
