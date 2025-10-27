/**
 * @ Version: SCREEN SPACE SHADERS - UPDATE 18
 * @ Description: Ripples code
 * @ Modified time: 2023-09-21 01:31
 * @ Author: https://www.moddb.com/members/ascii1457
 * @ Mod: https://www.moddb.com/mods/stalker-anomaly/addons/screen-space-shaders
 *
 * Based on the work of Sébastien Lagarde for the game "Remember Me"
 * https://seblagarde.wordpress.com/2013/01/03/water-drop-2b-dynamic-rain-and-its-effects/
 *
 * Setup : [ x: speed | y: intensity | z: ripple frequency ]
 * Sample : [ x: Fade | y: Normal-Y | z: Normal-X | w: Time Offset ]
*/

static const float3 SSFX_ripples_speed = float3( 1.05f, 1.31f, 1.58f ); 
static const float4 SSFX_ripples_offset = float4( 0.5f, 0.25f, 0.31f, 0.5f );

static const float SSFX_ripples_PI = 3.141592f;

float2 ssfx_process_ripples(float4 ripples, float3 setup)
{
	// Get bump from texture and transform to -1.0 ~ 1.0
	float2 ripples_N = ripples.yz * 2.0 - 1.0;

	// Apply time
	float RFrac = frac(ripples.w + timers.x * setup.x); // Apply time shift
	float TimeFrac = RFrac - 1.0f + ripples.x; // Fade
	float RFreq = clamp(TimeFrac * setup.z, 0.0f, 4.0); // Frequency and limit ( 2 = 1 full ripple )
	float FinalFactor = saturate(0.7f - RFrac) * ripples.x * sin( RFreq * SSFX_ripples_PI); // Create ripples

	// Fade
	FinalFactor *= smoothstep(4.0f, 0, RFreq);

	// Apply intensity
	ripples_N *= FinalFactor * setup.y;

	return ripples_N;
}

float2 ssfx_rain_ripples( Texture2D ripples_tex, float2 uvs, float3 setup, float depth)
{
	// Falloff
	float Dist = 15.0f - depth;
	
	// Discard when depth > 15
	if (Dist < 0)
		return 0;

	// Sample layers
	float4 Layer0 = ripples_tex.Sample( smp_linear, uvs );
	float4 Layer1 = ripples_tex.Sample( smp_linear, uvs * 0.61f + SSFX_ripples_offset.xy);
	float4 Layer2 = ripples_tex.Sample( smp_linear, uvs * 0.87f + SSFX_ripples_offset.zw);

	// Process 3 layers of ripples
	float2 result =	ssfx_process_ripples(Layer0, float3(SSFX_ripples_speed.x * setup.x, setup.yz)) +
					ssfx_process_ripples(Layer1, float3(SSFX_ripples_speed.y * setup.x, setup.yz)) +
					ssfx_process_ripples(Layer2, float3(SSFX_ripples_speed.z * setup.x, setup.yz));

	result *= Dist * 0.0666f; // 0.0 ~ 15.0 To 0.0 ~ 1.0

	return clamp(result, -1.0f, 1.0f);
}