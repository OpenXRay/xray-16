/*
	\\\\\\Screen Space Reflections//////

Credits goes to Xerxes1138, Danil Baryshev, and ForHaxed.

References:
https://habr.com/ru/post/244367/
https://github.com/Xerxes1138/UnitySSR/
https://www.amk-team.ru/forum/topic/14078-sslr

If you want to use this code in your project
just keep this header ;) Make modding better.

By LVutner for X-Ray Oxygen project (2020)
*/

#ifndef SSR_H
#define SSR_H

#define SSR_EDGE_ATTENUATION float(0.09) //Edge attenuation intensity
#define SSR_JITTER_INTENSITY float(0.05) //Jittering intensity

#include "common.h"

/*Quality tokens*/
#if !defined(SSR_QUALITY) || (SSR_QUALITY <= 1) || (SSR_QUALITY > 4)
	#define SSR_SAMPLES int(8) // Low
	#define SSR_DISTANCE float(150.0)
#elif SSR_QUALITY==2
	#define SSR_SAMPLES int(12) // Medium
	#define SSR_DISTANCE float(200.0)
#elif SSR_QUALITY==3
	#define SSR_SAMPLES int(16) // High
	#define SSR_DISTANCE float(300.0)
#else
	#define SSR_SAMPLES int(20) // Extreme
	#define SSR_DISTANCE float(400.0)
#endif

/*Helper functions*/
float RayAttenBorder(float2 pos, float value)
{
	float borderDist = min(1.0 - max(pos.x, pos.y), min(pos.x, pos.y));
	return saturate(borderDist > value ? 1.0 : borderDist / value);
}

float4 proj_to_screen(float4 proj)
{
	float4 screen = proj;
	screen.x = (proj.x + proj.w);
	screen.y = (proj.w - proj.y);
	screen.xy *= 0.5;
	return screen;
}

half is_sky(float depth)
{
	return step(abs(depth - 10000.0), 0.001);
}

float3 hash(float3 a)
{
	a *= timers.x;
    a = frac(a * 0.8);
    a += dot(a, a.yxz + 19.19);
    return frac((a.xxy + a.yxx)*a.zyx);
}

/*World space components as input*/
#ifndef SSR_QUALITY
float4 compute_ssr(float3 position, float3 normal, float3 skybox)
{
	return float4(skybox.xyz, 1.0);
}

#else

float4 compute_ssr(float3 position, float3 normal, float3 skybox)
{
	/*Initialize step size and error*/
	float step = 1.0/float(SSR_SAMPLES);

	/*Initialize reflected TC*/
	float2 refl_tc = float2(0.0,0.0);

	/*Prepare ray direction and reflection vector*/
	float3 v2point = normalize(position - eye_position);
	float3 vreflect = normalize(reflect(v2point,normalize(normal)));
	/*Main loop*/
#pragma optionNV (unroll all)
	for(int i = 0; i < SSR_SAMPLES; i++)
	{
		/*Prepare new position*/
		float3 new_position = position + vreflect * step;

		/*Add hash to new position*/
	#ifdef SSR_JITTER
		new_position += hash(position.xyz) * SSR_JITTER_INTENSITY;
	#endif

		/*Convert new position to texcoord*/
		float4 proj_position = mul(m_VP, float4(new_position, 1.0));
		float4 p2ss = proj_to_screen(proj_position);
		refl_tc.xy = p2ss.xy /= p2ss.w;

		/*Sample hit depth*/
	#ifndef SSR_HALF_DEPTH
		#ifndef USE_MSAA
			float hit_depth = tex2D(s_position, float2(refl_tc.x, -refl_tc.y)).z;
		#else
            float2 refl_tc_t = refl_tc.xy * screen_res.xy;
			float hit_depth = texelFetch(s_position, int2(refl_tc_t.x, screen_res.y - refl_tc_t.y), 0).z;
		#endif
	#else
			float hit_depth = tex2D(s_half_depth, float2(refl_tc.x, -refl_tc.y)).x;
	#endif

		/*Intersect sky from hit depth*/
		hit_depth = lerp(hit_depth, 0.0, is_sky(hit_depth));

		/*Sample depth*/
		float depth = mul(m_V, float4(position, 1.0)).z;

		/*Fixing incorrect refls*/
		if((depth - hit_depth) > 0.0 || (hit_depth > SSR_DISTANCE))
			return float4(skybox.xyz, 1.0);

		/*Depth difference*/
		step = length(hit_depth - depth);
	}

	/*Edge attenuation*/
	float edge = RayAttenBorder(refl_tc.xy, SSR_EDGE_ATTENUATION);

	/*Sample image with reflected TC*/
    float2 refl_tc_tmp = refl_tc.xy * screen_res.xy;
	float3 img = texelFetch(s_image, int2(refl_tc_tmp.x, screen_res.y - refl_tc_tmp.y), 0).xyz;

	/*Image.rgb, Reflcontrol.a*/
	return float4(img.xyz, edge);
}
#endif //SSR quality
#endif //Header
