/**
 * @ Version: SCREEN SPACE SHADERS - UPDATE 19
 * @ Description: Wind Main File
 * @ Modified time: 2023-12-23 16:05
 * @ Author: https://www.moddb.com/members/ascii1457
 * @ Mod: https://www.moddb.com/mods/stalker-anomaly/addons/screen-space-shaders
 */

uniform float4 wind_params;

uniform float4 ssfx_wsetup_grass; // Anim Speed - Turbulence - Push - Wave
uniform float4 ssfx_wsetup_trees; // Branches Speed - Trunk Speed - Bending - Min Wind Speed
uniform float4 ssfx_wind_anim;

Texture2D s_waves;
sampler smp_linear2;

struct wind_setup 
{
	float2 direction;
	float speed;
	float sqrt_speed;

	float trees_animspeed;
	float trees_trunk_animspeed;
	float trees_bend;

	float grass_animspeed;
	float grass_turbulence;
	float grass_push;
	float grass_wave;
};

wind_setup ssfx_wind_setup()
{
	wind_setup wsetup;

	// Direction. Radians to Vector
	float r = -wind_params.x + 1.57079f;
	wsetup.direction = float2(cos(r),sin(r));

	// Wind Speed
	wsetup.speed = max(ssfx_wsetup_trees.w, saturate(wind_params.y * 0.001));
	wsetup.sqrt_speed = saturate(sqrt(wsetup.speed * 1.66f));

	// Setup
	wsetup.grass_animspeed = ssfx_wsetup_grass.x;
	wsetup.grass_turbulence = ssfx_wsetup_grass.y;
	wsetup.grass_push = ssfx_wsetup_grass.z;
	wsetup.grass_wave = ssfx_wsetup_grass.w;

	wsetup.trees_animspeed = ssfx_wsetup_trees.x;
	wsetup.trees_trunk_animspeed = ssfx_wsetup_trees.y;
	wsetup.trees_bend = ssfx_wsetup_trees.z;

	return wsetup;
}

#ifdef SSFX_WIND_ISGRASS

// Flow Map - X: X-Anim | Y: Z-Anim | Z: Wind Wave | W : Detail

float3 ssfx_wind_grass(float3 pos, float H, wind_setup W)
{
	// Height Limit. ( Add stiffness to tall grass )
	float HLimit = saturate(H * H - 0.01f) * saturate(1.0f - H * 0.1f);
	
	// Offset animation
	float2 Offset = -ssfx_wind_anim.xy * W.grass_animspeed;

	// Sample ( The scale defines the detail of the motion )
	float3 Flow = s_waves.SampleLevel( smp_linear2, (pos.xz + Offset) * 0.018f, 0);

	// Grass Motion ( -1.0 ~ 1.0 ). Turbulence.
	float2 GrassMotion = (Flow.xy * 2.0f - 1.0f) * W.grass_turbulence;

	// Apply wind direction and flow. Wind push.
	float2 WindDir = Flow.z * W.direction * W.grass_push;
	
	// Add everything and apply height limit
	float3 Final = float3(GrassMotion.x + WindDir.x, Flow.z * W.grass_wave, GrassMotion.y + WindDir.y) * W.speed * HLimit;
	
	return Final;
}

#else // Non Grass

float3 ssfx_wind_tree_trunk(float3 pos, float Tree_H, wind_setup W)
{
	// Phase ( from matrix ) + Offset 
	float Phase = m_xform._24 + ssfx_wind_anim.z * W.trees_trunk_animspeed;

	// Trunk wave
	float TWave = (cos(Phase) * sin(Phase * 5.0f) + 0.5f) * W.trees_bend;

	// Wind speed
	float WSpeed = saturate(W.sqrt_speed * 1.5f);

	// Base wind speed displacement
	float Base_Bend = WSpeed * 0.006f * saturate(1.0f - Tree_H * 0.005f);

	// Intensity ( Squared height )
	Base_Bend *= Tree_H * Tree_H * TWave * WSpeed;

	// Apply direction
	float2 Final = Base_Bend.xx * W.direction;

	return float3(Final, saturate((TWave + 1.0f) * 0.5));
}

float3 ssfx_wind_tree_branches(float3 pos, float Tree_H, float tc_y, wind_setup W)
{
	// UV Offset
	float2 Offset = -ssfx_wind_anim.xy * W.trees_animspeed;
	
	// Sample flow map
	float3 Flow = s_waves.SampleLevel( smp_linear2, (pos.xz + Offset) * 0.02f, 0);
	
	// Sample 2, slower and detailed
	float3 Flow2 = s_waves.SampleLevel( smp_linear2, (pos.xz + Offset * 0.2f) * 0.1f, 0);

	// Branch motion [ -1.0 ~ 1.0 ]
	float3 branchMotion = float3(Flow.x, Flow2.y, Flow.y) * 2.0f - 1.0f;

	// Trunk position
	float3 Trunk = ssfx_wind_tree_trunk(pos, Tree_H, W);

	// Gust from trunk data.
	branchMotion.xz *= Trunk.z * clamp(Tree_H * 0.1f, 1.0f, 2.5f);

	// Add wind direction
	branchMotion.xz += Flow2.z * W.direction;

	// Add wind gust
	branchMotion.y *= saturate(Tree_H * 0.1f);

	// Everything is limited by the UV and wind speed
	branchMotion *= (1.0f - tc_y) * W.speed;

	// Add trunk animation
	branchMotion.xz += Trunk.xy;
	
	return branchMotion;
}

#endif