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

	// ===== PHASE 6: Transform blade to world space =====

	// Sample wind at instance base position to get prevailing wind direction
	float2 base_wind_uv = det.pos.xz * 0.001;
	float4 base_wind = wind_texture.SampleLevel(interaction_sampler, base_wind_uv, 0);
	float2 base_wind_dir = base_wind.xz * 2.0 - 1.0;
	float base_wind_len = length(base_wind_dir);
	if (base_wind_len > 0.001)
	{
		base_wind_dir = base_wind_dir / base_wind_len;
	}

	// Construct base rotation matrix from instance data
	float3x3 rotation = float3x3(det.m0, det.m1, det.m2);

	// Apply wind lean by rotating the blade around Y-axis towards wind direction
	// Create a rotation matrix that tilts the blade ~10-15 degrees towards wind
	float lean_angle = 0.2;  // ~11 degrees lean towards wind

	// Build rotation matrix around Y-axis for wind lean
	// Rotate the blade's forward direction (Z-axis) towards wind
	float3 wind_dir_3d = float3(base_wind_dir.x, 0.0, base_wind_dir.y);

	// Tilt the up vector (Y-axis) slightly towards wind direction
	float3 tilted_up = normalize(float3(wind_dir_3d.x * lean_angle, 1.0, wind_dir_3d.z * lean_angle));

	// Recompute orthonormal basis with tilted up vector
	float3 right = normalize(cross(tilted_up, rotation[2]));  // Cross with original forward
	float3 forward = normalize(cross(right, tilted_up));

	// Build final rotation matrix with wind lean
	rotation = float3x3(right, tilted_up, forward);

	// Scale blade by instance scale
	float3 local_pos = I.pos * det.scale;

	// Rotate to wind-leaning orientation
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
	// This contains both the global wind direction and local FBM turbulence
	float2 wind_direction = wind.xz * 2.0 - 1.0;
	float wind_strength = wind.a;

	// Normalize wind direction for consistent leaning
	float wind_dir_length = length(wind_direction);
	if (wind_dir_length > 0.001)
	{
		wind_direction = wind_direction / wind_dir_length;
	}

    // === INTERACTION BENDING (trampling/pushing) ===
    // Ghost of Tsushima uses Bezier curve control points for bending
    // We achieve similar results with arc-based displacement (simpler, pre-generated geometry)
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
        // Similar to GoT's parabolic distribution: bendAmount = pow(t, CURVE_POWER)
        float bend_curve = vertex_height_factor * vertex_height_factor * vertex_height_factor;

        // CRITICAL: Limit maximum bend angle to prevent flat grass
        // Even heavily trampled grass maintains ~30-45 degree angle
        float max_bend_angle = 60.0 * 3.14159 / 180.0;  // 60 degrees max
        float actual_bend = interaction_strength * max_bend_angle;

        // Calculate arc-based displacement (maintains natural curve)
        // Ghost of Tsushima modifies Bezier control points; we offset vertices
        float blade_length = det.scale;  // Actual blade height

        // Vertical drop follows arc (maintains height even when bent)
        // cos(60°) = 0.5, so grass at 60 degrees still retains 50% height
        float vertical_drop = blade_length * (1.0 - cos(actual_bend)) * bend_curve;
        pos.y -= vertical_drop * grass_interaction_displacement;
    }

    // === WIND ANIMATION ===
    // Ghost of Tsushima approach: wind affects blade bending
    // IMPORTANT: Grass blades are ROOTED - base stays fixed, only tips move
    float wind_bend_curve = vertex_height_factor * vertex_height_factor;

    // Horizontal wind sway
    wind_displacement = wind_direction * wind_strength * wind_bend_curve * grass_wind_displacement;
    pos.xz += wind_displacement;

    // Vertical drop from horizontal bending (arc physics, like interaction)
    // When blade bends horizontally, tip naturally drops slightly
    // Calculate bend amount from horizontal displacement
    float horizontal_bend_amount = length(wind_displacement);
    if (horizontal_bend_amount > 0.001)
    {
        // Natural vertical drop from bending (grass doesn't stay at full height when bent)
        // This is geometric: if tip moves horizontally, it must drop vertically
        float max_drop = horizontal_bend_amount * 0.2;  // Slight drop, grass is flexible
        pos.y -= max_drop * wind_bend_curve;  // Only tips drop significantly
    }

	// ===== GHOST OF TSUSHIMA: Calculate blade normal from tangent and facing =====
	// GoT approach from doc (lines 172-183):
	// "Tangent along blade (from Bezier derivative)"
	// "Facing direction (perpendicular in world space)"
	// "Surface normal perpendicular to both"

	float hemi = abs(det.hemi);
	float sun = sign(det.hemi) * 0.25f + 0.25f;

	// Tangent = direction along blade length (vertical component after rotation)
	// For our pre-generated geometry, the blade grows along Y-axis
	// IMPORTANT: Do NOT modify tangent based on displacement - vertices are already bent!
	// Modifying tangent causes normal flipping as camera moves
	float3 tangent = normalize(rotation[1]);  // Up direction of blade

	// Facing = blade orientation in XZ plane (which way the blade "looks")
	// This is the forward direction the blade faces (perpendicular to its width)
	float3 facing = normalize(rotation[2]);  // Z-axis of rotation matrix (forward)
	facing.y = 0.0;  // Project to XZ plane
	facing = normalize(facing);

	// Normal = perpendicular to blade surface
	// Ghost of Tsushima: normal = cross(tangent, facing)
	// This gives us a stable normal that doesn't flip as displacement changes
	float3 blade_normal = normalize(cross(tangent, facing));

	// Two-sided lighting: ensure normal faces camera
	// GoT doc mentions "glancing angle adjustments" for edge-on blades
	// CRITICAL: Use actual vertex position (pos.xyz), NOT instance base (det.pos)!
	// Using det.pos causes normals to flip segment-by-segment as camera moves
	float3 view_dir = normalize(rotated_pos.xyz - eye_position.xyz);
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
