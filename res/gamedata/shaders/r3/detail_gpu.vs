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

	// ===== GHOST OF TSUSHIMA: BEZIER CURVE-BASED BLADE BENDING =====
	// GoT doc (lines 78-103): "Control point generation from blade parameters"
	// We define a cubic Bezier curve and modify control points for bending

	// Construct base rotation matrix from instance data
	float3x3 rotation = float3x3(det.m0, det.m1, det.m2);

	// Define Bezier control points in LOCAL SPACE (before rotation)
	// Match C++ GenerateGrassBlade structural shape parameters
	const float tilt = 0.125;       // ~7 degree natural lean (radians)
	const float bend = 0.125;       // Forward curve amount
	const float curve_bias = 0.8;   // Where the curve peaks (0-1)

	// P0: Root at origin
	float3 P0 = float3(0, 0, 0);

	// P3: Tip with structural tilt applied (C++ line 412-415)
	float3 structural_facing;
	structural_facing.x = sin(tilt);
	structural_facing.y = cos(tilt);
	structural_facing.z = 0.0;

	float3 P3;
	P3.x = structural_facing.x * det.scale * 0.2;  // Slight lean in facing direction
	P3.y = det.scale;                               // Full height
	P3.z = structural_facing.z * det.scale * 0.2;

	// Calculate midpoint for control point positioning
	float3 midpoint = (P0 + P3) * 0.5;

	// Bend direction: perpendicular to structural_facing in XZ plane (C++ line 424-427)
	float3 bend_dir;
	bend_dir.x = -structural_facing.z;
	bend_dir.y = 0.0;
	bend_dir.z = structural_facing.x;
	float bend_len = length(bend_dir.xz);
	if (bend_len > 0.001)
	{
		bend_dir.xz = bend_dir.xz / bend_len;
	}

	// P1 and P2: Control points with structural bend (C++ line 435-442)
	float3 P1, P2;
	P1.x = P0.x + (midpoint.x - P0.x) * curve_bias + bend_dir.x * bend * 0.3;
	P1.y = P0.y + (midpoint.y - P0.y) * curve_bias;
	P1.z = P0.z + (midpoint.z - P0.z) * curve_bias + bend_dir.z * bend * 0.3;

	P2.x = midpoint.x + (P3.x - midpoint.x) * curve_bias + bend_dir.x * bend * 0.7;
	P2.y = midpoint.y + (P3.y - midpoint.y) * curve_bias;
	P2.z = midpoint.z + (P3.z - midpoint.z) * curve_bias + bend_dir.z * bend * 0.7;

	// Transform to world space (applies instance orientation + position)
	P0 = mul(rotation, P0) + det.pos;
	P1 = mul(rotation, P1) + det.pos;
	P2 = mul(rotation, P2) + det.pos;
	P3 = mul(rotation, P3) + det.pos;

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
		interaction = interaction_atlas.SampleLevel(interaction_sampler, atlas_uv, 0);
	}

	// ===== MODIFY BEZIER CONTROL POINTS FOR WIND/INTERACTION =====
	// GoT doc (lines 391-405): "Wind animation integration"
	// Instead of directly offsetting vertices, we modify P1 and P2

	// Sample wind texture at blade base
	float2 wind_uv = P0.xz * 0.001;  // Use base position for sampling
	float4 wind = wind_texture.SampleLevel(interaction_sampler, wind_uv, 0);

	// Decode wind direction and strength
	float2 wind_direction = wind.xz * 2.0 - 1.0;
	float wind_strength = wind.a;

	// Normalize wind direction
	float wind_dir_len = length(wind_direction);
	if (wind_dir_len > 0.001)
	{
		wind_direction = wind_direction / wind_dir_len;
	}

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
		wind_direction.x,
		0.0,  // Wind is horizontal
		wind_direction.y
	) * wind_strength * grass_wind_displacement;

	float3 interaction_offset = float3(
		interaction_push_dir.x,
		-0.5,  // Grass bends down when trampled
		interaction_push_dir.y
	) * interaction_strength * grass_interaction_displacement;

	// Combine wind and interaction
	float3 total_offset = wind_offset + interaction_offset;

	// ===== BEZIER CURVE BENDING WITH ARC PHYSICS =====
	// Key insight: Grass doesn't just tilt - it ARCS (droops) when bent
	// P0 = fixed base
	// P3 = tip moves horizontally to show wind direction
	// P1, P2 = follow the arc path with natural vertical drop

	// Calculate horizontal displacement (XZ plane)
	float horizontal_displacement = length(total_offset.xz);

	if (horizontal_displacement > 0.001)
	{
		// Normalize horizontal direction
		float2 bend_dir_xz = total_offset.xz / horizontal_displacement;

		// P3 (tip): Move horizontally in wind direction
		// Also drops down due to arc geometry (like original implementation)
		float tip_drop = horizontal_displacement * 0.3;  // Natural droop when bent
		P3 += float3(total_offset.x, -tip_drop + total_offset.y, total_offset.z);

		// P1 (lower control point): 1/3 along the arc
		// Moves less horizontally, drops less vertically
		float p1_horizontal = horizontal_displacement * 0.33;
		float p1_drop = p1_horizontal * p1_horizontal / (horizontal_displacement * 2.0);  // Parabolic drop
		P1 += float3(bend_dir_xz.x * p1_horizontal, -p1_drop, bend_dir_xz.y * p1_horizontal);

		// P2 (upper control point): 2/3 along the arc
		// Moves more horizontally, drops more vertically
		float p2_horizontal = horizontal_displacement * 0.67;
		float p2_drop = p2_horizontal * p2_horizontal / (horizontal_displacement * 1.5);  // Parabolic drop
		P2 += float3(bend_dir_xz.x * p2_horizontal, -p2_drop, bend_dir_xz.y * p2_horizontal);
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

	// Apply width offset (blade has width in X direction in local space)
	// Rotate width vector by instance rotation
	float3 width_offset = mul(rotation, float3(I.pos.x * det.scale, 0, 0));

	// Final position
	float4 pos = float4(bezier_pos + width_offset, 1.0);

	// ===== GHOST OF TSUSHIMA: Calculate blade normal from Bezier derivative =====
	// GoT doc (lines 172-183):
	// "Tangent along blade (from Bezier derivative)"
	// "Facing direction (perpendicular in world space)"
	// "Surface normal perpendicular to both"

	float hemi = abs(det.hemi);
	float sun = sign(det.hemi) * 0.25f + 0.25f;

	// TANGENT = Bezier curve derivative at parameter t
	// B'(t) = 3(1-t)²(P₁-P₀) + 6(1-t)t(P₂-P₁) + 3t²(P₃-P₂)
	// GoT doc (lines 72-76): "The first derivative (tangent vector)"
	float3 tangent = 3.0 * mt2 * (P1 - P0) + 6.0 * mt * t * (P2 - P1) + 3.0 * t2 * (P3 - P2);
	tangent = normalize(tangent);

	// FACING = blade orientation in XZ plane (perpendicular to blade width)
	// This comes from the instance rotation (forward direction)
	float3 facing = normalize(rotation[2]);  // Z-axis of rotation matrix
	facing.y = 0.0;  // Project to XZ plane for horizontal orientation
	facing = normalize(facing);

	// NORMAL = perpendicular to blade surface
	// Ghost of Tsushima: normal = cross(tangent, facing)
	float3 blade_normal = normalize(cross(tangent, facing));

	// Two-sided lighting: ensure normal faces camera
	// GoT doc mentions "glancing angle adjustments" for edge-on blades
	// Use base position (before bending) for stable normal orientation
	float3 view_dir = normalize(base_world_pos - eye_position.xyz);
	if (dot(blade_normal, view_dir) > 0.0)
	{
		blade_normal = -blade_normal;
	}

	// ===== GHOST OF TSUSHIMA: ROUNDED BLADE NORMALS =====
	// Generate rotated normals for cylindrical blade appearance
	// The fragment shader will interpolate between these based on width position
	// Reference: GoT presentation screenshot - creates illusion of rounded cross-section

	// Helper function: rotate vector around axis
	// Rodrigues' rotation formula: v_rot = v*cos(θ) + (k×v)*sin(θ) + k*(k·v)*(1-cos(θ))
	float rotationAngle = 3.14159 * 0.3;  // ±30 degrees (PI * 0.3)

	float cosTheta = cos(rotationAngle);
	float sinTheta = sin(rotationAngle);

	// Rotate around tangent (blade's up direction)
	float3 axis = tangent;

	// Rotated normal 1: rotate by +angle
	float3 rotatedNormal1 = blade_normal * cosTheta
	                      + cross(axis, blade_normal) * sinTheta
	                      + axis * dot(axis, blade_normal) * (1.0 - cosTheta);

	// Rotated normal 2: rotate by -angle
	float3 rotatedNormal2 = blade_normal * cosTheta
	                      + cross(axis, blade_normal) * (-sinTheta)
	                      + axis * dot(axis, blade_normal) * (1.0 - cosTheta);

	// ===== OUTPUT =====

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
