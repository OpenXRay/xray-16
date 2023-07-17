/**
 * @ Version: SCREEN SPACE SHADERS - UPDATE 14.5
 * @ Description: Indirect Light Shader
 * @ Modified time: 2023-02-01 07:08
 * @ Author: https://www.moddb.com/members/ascii1457
 * @ Mod: https://www.moddb.com/mods/stalker-anomaly/addons/screen-space-shaders
 */

#include "settings_screenspace_IL.h"

#ifndef SSFX_READY
	#include "screenspace_common.h"
#endif

// Internal Vars
static const int il_quality[4] = { 8, 16, 24, 32 };

// Some vars to fix incompatibilities for the moment...
uniform float4 ssfx_wpn_dof_1;
uniform float4 fakescope_params3;

float3 ssfx_il_bounce(float3 P, float3 N, float Range, int count, uint iSample) 
{
	// Use surface normal and add the hemisphere distribution
	float3 sample_rays = ssfx_hemisphere[count] + N.xyz;
	
	// Position to sample
	float3 occ_pos = P.xyz + sample_rays * Range;
	
	// Sample position to UV
	float2 occ_pos_uv = SSFX_view_to_uv(occ_pos);
	
	// Get position buffer to calc normal and get depth
	float4 sample_pos = SSFX_get_position(occ_pos_uv, iSample);

	// Adjust G_IL_MAX_DIFFERENCE if the sampled pixel is a weapon/hand...
	float Maxdiff = sample_pos.z < 1.5f ? 2.0f : G_IL_MAX_DIFFERENCE;

	// IL base intensity
	float il_intensity = smoothstep(Maxdiff, 0.0f, abs(P.z - sample_pos.z));

	// Use Normal to adjust intensity and avoid self IL ( 1 version for MAT FLORA FIXES )
#ifndef SSFX_FLORAFIX
	il_intensity *= 1.0 - saturate(dot(gbuf_unpack_normal( sample_pos.xy ), N ));
#else
	bool IsFlora = abs(sample_pos.w - MAT_FLORA) <= 0.04f; // Is MAT_FLORA?
	il_intensity *= 1.0 - saturate(dot(gbuf_unpack_normal( sample_pos.xy ), N ) - IsFlora); // Discard intensity adjustment if MAT_FLORA
#endif

	// Never discard the sample if comes from the sky. We use this for some sort of sky light.
	if (is_sky(sample_pos.z))
		il_intensity = 1.0f * G_IL_SKYLIGHT_INTENSITY;
	
	// Discard if the final intensity is lower or equal to G_IL_DISCARD_SAMPLE_AT
	[branch]
	if (il_intensity > G_IL_DISCARD_SAMPLE_AT)
	{
		// Sample scene color
		float3 sample_il = s_blur_8.SampleLevel(smp_rtlinear, occ_pos_uv, 0);

		// Adjust intensity using Normal, Difference and sky.
		sample_il *= il_intensity;

		// Return sample
		return sample_il;
	}

	// Well...
	return 0;
}


void ssfx_il(float2 tc, float2 pos2d, float3 P, float3 N, inout float3 color, uint iSample)
{
	// Skip Sky. ( Disable when used with Shader Based 2D Scopes )
	if (P.z <= SKY_EPS || fakescope_params3.x > 0)
		return;

	// Discard IL when using NV
#ifdef SSFX_BEEFS_NVG
	if (shader_param_8.x > 0)
		return;
#endif

	// Var to accumulate the IL
	float3 il = 0;

	// Depth distance. Used for some calcs later
	float PLen = length(P);

	// Weapon factor to adjust some values for weapons
	float WeaponFactor = smoothstep(G_IL_WEAPON_LENGTH * 0.5f, G_IL_WEAPON_LENGTH, PLen);

	float il_noise = frac(sin(dot(tc, float2(12.0, 78.0) )) * 43758.0) * G_IL_NOISE;

	bool IsWpn = PLen < 1.5;

	[unroll (il_quality[G_IL_QUALITY])]
	for (int i = 0; i < il_quality[G_IL_QUALITY]; i++)
	{
		// Adjust weapons range
		float range = (G_IL_WEAPON_RANGE + G_IL_RANGE * WeaponFactor) * (1.0f + il_noise);

		// Do bounce
		il += ssfx_il_bounce(P, N, range, i, iSample);
	}

	// Normalize result
	il /= il_quality[G_IL_QUALITY];

	// Fogging
#ifdef SSFX_FOG
	float3 WorldP = mul(m_inv_V, float4(P.xyz, 1));
	float Fog =  saturate(PLen * fog_params.w + fog_params.x);
	
	// Same as SSFX_FOGGING but multiplied * 2
	float fog_height = smoothstep(G_FOG_HEIGHT, -G_FOG_HEIGHT, WorldP.y) * G_FOG_HEIGHT_INTENSITY;
	float fog_extra = saturate(Fog + fog_height * (Fog * G_FOG_HEIGHT_DENSITY));
	float Fade = 1.0f - saturate(fog_extra * 2.0f);
#else
	// Vanilla fog calc multiplied * 2
	float Fade = 1.0f - saturate((PLen * fog_params.w + fog_params.x) * 2.0f);
#endif

	// "Fix" DOF incompatibility ( Reload at the moment... Maybe peripheral blur requires the same? )
	Fade *= 1.0f - ssfx_wpn_dof_1.z * smoothstep(1.0f, 2.4f, PLen);

	// Screen edge fade ( To avoid some artifacts )
	float4 calc_edges = smoothstep(-0.05f, 0.3f, float4(tc.x, 1.0f - tc.x, tc.y, 1.0f - tc.y));
	Fade *= calc_edges.x * calc_edges.y * calc_edges.z * calc_edges.w;

	// Apply fading. ( Edges + Fog + DOF fix )
	il *= Fade * saturate(PLen * 2.0f);

	// Vibrance
#ifndef G_IL_DEBUG_MODE
	il = vibrance(il, G_IL_COLOR_VIBRANCE);
#else
	il = vibrance(il, 100);
#endif

	// -- Mix with scene color.
	float blendfact = 1.0 - dot(color, LUMINANCE_VECTOR); // Blend factor using LUMINANCE_VECTOR

	// Adjust intensity using scene color.
	il = il * dot(color, G_IL_INTENSITY) * blendfact * blendfact; 

	// Check color difference between color and IL.
	float colordiff = saturate(normalize(il) - normalize(color));
	
	// Square difference to get a nice falloff curve where a higher difference fall faster than a lower one.
	colordiff = sqrt(colordiff);

	// Final Mix. Adjust intensity using colordiff.
	color = saturate(color + il * (1.0f - colordiff));
}