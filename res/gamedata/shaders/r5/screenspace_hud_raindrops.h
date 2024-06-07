/**
 * @ Version: SCREEN SPACE SHADERS - UPDATE 17
 * @ Description: HUD Raindrops - Base
 * @ Modified time: 2023-08-04 07:23
 * @ Author: https://www.moddb.com/members/ascii1457
 * @ Mod: https://www.moddb.com/mods/stalker-anomaly/addons/screen-space-shaders
 */

Texture2D s_hud_rain; // Raindrops texture [ x: Fade | y: Normal-Y | z: Normal-X | w: Time Offset ]

#define SSFX_HUD_LIGHTVECTOR	float3(0.30f, 0.59f, 0.11f) // Vector to adjust the intensity of the raindrops at night

uniform float4 ssfx_hud_drops_1; // Time, Int, refle, refra
uniform float4 ssfx_hud_drops_2; // Density, Size, Extra Gloss, Gloss

float3 ssfx_process_drops(float4 droplets, float speed, float intensity )
{
	// Unpack normal [ -1.0 ~ 1.0 ]
	float2 droplets_N = droplets.yz * 2 - 1;

	// Apply time
	float DropsTime = ssfx_hud_drops_1.x * speed; // Fade * speed factor
	float DropsFrac = 1.0f - frac(droplets.w + DropsTime); // Time offset
	float DropsInt = saturate((DropsFrac * droplets.x) - ssfx_hud_drops_2.x); // (Time * Fade) - Density

	// Apply intensity
	DropsInt *= intensity;
	droplets_N *= DropsInt;

	return float3(droplets_N, DropsInt);
}

// Mapping based on https://iquilezles.org/articles/biplanar/
// Thanks to the chaotic nature of the drops, we can do a very basic planar mapping with 1 sample ( I'm using 2 to add variety to the drops )
float3 ssfx_hud_raindrops( Texture2D drops_tex, float3 p, float uv_scale )
{
	// Partial derivatives to reconstruct normal & mapping
	float3 d1 = ddx(p);
	float3 d2 = ddy(p);
		
	// Recontruct normal
	float3 n = abs(normalize(cross(d1, d2)));

	// Major axis (in x; yz are following axis)
	int3 ma = (n.x > n.y && n.x > n.z)	? int3(0, 1, 2) : // [ yz ] Side
			  (n.y > n.z)				? int3(1, 2, 0) : // [ zx ] Top
										  int3(2, 0, 1) ; // [ xy ] Front 

	// Uvs & partial derivatives
	float2 uvs = float2(p[ma.y], p[ma.z]) * ssfx_hud_drops_2.y;
	float4 d1_d2 = float4(d1[ma.y], d1[ma.z], d2[ma.y], d2[ma.z]);

	// Project + Fetch [ x: Fade | y: Normal-Y | z: Normal-X | w: Time Offset ]
	float4 Layer0 = drops_tex.SampleGrad(smp_base, uvs * 15.0f, d1_d2.xy, d1_d2.zw);
	float4 Layer1 = drops_tex.SampleGrad(smp_base, (uvs + float2(.23,.46)) * 8.0f, d1_d2.xy, d1_d2.zw);

	float w = pow( n[ma.x], 5 );

	// Process animation
	float3 result = ssfx_process_drops(Layer0, 0.2f, w) + ssfx_process_drops(Layer1, 0.1f, w);
	result.xy = clamp(result.xy, -1.0f, 1.0f);

	return result;
}