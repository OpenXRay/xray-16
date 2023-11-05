/**
 * @ Version: SCREEN SPACE SHADERS - UPDATE 14
 * @ Description: Main file
 * @ Modified time: 2023-01-18 21:11
 * @ Author: https://www.moddb.com/members/ascii1457
 * @ Mod: https://www.moddb.com/mods/stalker-anomaly/addons/screen-space-shaders
 */

#define SSFX_READY

#include "common.h"
#include "lmodel.h"
#include "hmodel.h"

#include "screenspace_common_noise.h"
#include "screenspace_common_ripples.h"

#include "check_screenspace.h"

static const float3 ssfx_hemisphere[32] =
{
	float3(-0.134, 0.044, -0.825),	float3(0.045, -0.431, -0.529),	float3(-0.537, 0.195, -0.371),
	float3(0.525, -0.397, 0.713),	float3(0.895, 0.302, 0.139),	float3(-0.613, -0.408, -0.141),
	float3(0.307, 0.822, 0.169),	float3(-0.819, 0.037, -0.388),	float3(0.376, 0.009, 0.193),
	float3(-0.006, -0.103, -0.035),	float3(0.098, 0.393, 0.019),	float3(0.542, -0.218, -0.593),
	float3(0.526, -0.183, 0.424),	float3(-0.529, -0.178, 0.684),	float3(0.066, -0.657, -0.570),
	float3(-0.214, 0.288, 0.188),	float3(-0.689, -0.222, -0.192),	float3(-0.008, -0.212, -0.721),
	float3(0.053, -0.863, 0.054),	float3(0.639, -0.558, 0.289),	float3(-0.255, 0.958, 0.099),
	float3(-0.488, 0.473, -0.381),	float3(-0.592, -0.332, 0.137),	float3(0.080, 0.756, -0.494),
	float3(-0.638, 0.319, 0.686),	float3(-0.663, 0.230, -0.634),	float3(0.235, -0.547, 0.664),
	float3(0.164, -0.710, 0.086),	float3(-0.009, 0.493, -0.038),	float3(-0.322, 0.147, -0.105),
	float3(-0.554, -0.725, 0.289),	float3(0.534, 0.157, -0.250),
};

#ifdef USE_MSAA
	Texture2DMS<float4,MSAA_SAMPLES> s_rimage;
#else
	Texture2D s_rimage;
#endif

uniform float4 sky_color;

TextureCube sky_s0;
TextureCube sky_s1;

static const float2 ssfx_pixel_size = 1.0f / screen_res.xy;
static const float ssfx_PI = 3.14159265f;

struct RayTrace
{
	float2 r_pos;
	float2 r_step;
	float2 r_start;
	float r_length;
	float z_start;
	float z_end;
};

bool SSFX_is_saturated(float2 value) { return (value.x == saturate(value.x) && value.y == saturate(value.y)); }

bool SSFX_is_valid_uv(float2 value) { return (value.x >= 0.0f && value.x <= 1.0f) && (value.y >= 0.0f && value.y <= 1.0f); }

float2 SSFX_view_to_uv(float3 Pos)
{
	float4 tc = mul(m_P, float4(Pos, 1));
	return (tc.xy / tc.w) * float2(0.5f, -0.5f) + 0.5f;
}

float SSFX_calc_SSR_fade(float2 tc, float start, float end)
{
	// Vectical fade
	float ray_fade = saturate(tc.y * 5.0f);
	
	// Horizontal fade
	float2 calc_edges = smoothstep(start, end, float2(tc.x, 1.0f - tc.x));
	ray_fade *= calc_edges.x * calc_edges.y;

	return ray_fade;
}

float3 SSFX_yaw_vector(float3 Vec, float Rot)
{
	float s, c;
	sincos(Rot, s, c);

	// y-axis rotation matrix
	float3x3 rot_mat = 
	{
		c, 0, s,
		0, 1, 0,
		-s, 0, c
	};
	return mul(rot_mat, Vec);
}

float SSFX_get_depth(float2 tc, uint iSample : SV_SAMPLEINDEX)
{
	#ifndef USE_MSAA
		return s_position.Sample(smp_nofilter, tc).z;
	#else
		return s_position.Load(int3(tc * screen_res.xy, 0), iSample).z;
	#endif
}

float4 SSFX_get_position(float2 tc, uint iSample : SV_SAMPLEINDEX)
{
	#ifndef USE_MSAA
		return s_position.Sample(smp_nofilter, tc);
	#else
		return s_position.Load(int3(tc * screen_res.xy, 0), iSample);
	#endif
}

float3 SSFX_get_image(float2 tc, uint iSample : SV_SAMPLEINDEX)
{
	#ifndef USE_MSAA
		return s_rimage.Sample(smp_nofilter, tc).rgb;
	#else
		return s_rimage.Load(int3(tc * screen_res.xy, 0), 0).rgb;
	#endif
}

RayTrace SSFX_ray_init(float3 ray_start_vs, float3 ray_dir_vs, float ray_max_dist, int ray_steps, float noise)
{
	RayTrace rt;
	
	float3 ray_end_vs = ray_start_vs + ray_dir_vs * ray_max_dist;
	
	// Ray depth ( from start and end point )
	rt.z_start		= ray_start_vs.z;
	rt.z_end		= ray_end_vs.z;

	// Compute ray start and end (in UV space)
	rt.r_start		= SSFX_view_to_uv(ray_start_vs);
	float2 ray_end	= SSFX_view_to_uv(ray_end_vs);

	// Compute ray step
	float2 ray_diff	= ray_end - rt.r_start;
	rt.r_step		= ray_diff / (float)ray_steps; 
	rt.r_length		= length(ray_diff);
	
	rt.r_pos		= rt.r_start + rt.r_step * noise;
	
	return rt;
}

float3 SSFX_ray_intersect(RayTrace Ray, uint iSample)
{
	float len = length(Ray.r_pos - Ray.r_start);
	float alpha = len / Ray.r_length;
	float depth_ray = (Ray.z_start * Ray.z_end) / lerp(Ray.z_end, Ray.z_start, alpha);
	float depth_scene = SSFX_get_depth(Ray.r_pos, iSample);
	
	return float3(depth_ray.x - depth_scene, depth_scene, len);
}

// Half-way scene lighting
float4 SSFX_get_fast_scenelighting(float2 tc, uint iSample : SV_SAMPLEINDEX)
{
	#ifndef USE_MSAA
		float4 rL = s_accumulator.Sample(smp_nofilter, tc);
		float4 C = s_diffuse.Sample( smp_nofilter, tc );
	#else
		float4 rL = s_accumulator.Load(int3(tc * screen_res.xy, 0), iSample);
		float4 C = s_diffuse.Load(int3( tc * screen_res.xy, 0 ), iSample);
	#endif
	
	#ifdef SSFX_ENHANCED_SHADERS // We have Enhanced Shaders installed
		
		/*float3 hdiffuse = C.rgb + L_ambient.rgb;
		
		rL.rgb += rL.a * SRGBToLinear(C.rgb);
		
		return float4(LinearTosRGB((rL.rgb + hdiffuse) * saturate(rL.rrr * 100)), C.w);*/
		
		float3 result = rL.rgb + rL.a * C.rgb;
		result *= env_color.rgb;
		
		return float4(result, C.w);

	#else
		
		float3 hdiffuse = C.rgb + L_ambient.rgb;

		return float4((rL.rgb + hdiffuse) * saturate(rL.rrr * 100), C.w);
		
	#endif
}

// Full scene lighting
float3 SSFX_get_scene(float2 tc, uint iSample : SV_SAMPLEINDEX)
{
	#ifndef USE_MSAA
		float4 rP = s_position.Sample( smp_nofilter, tc );
	#else
		float4 rP = s_position.Load(int3(tc * screen_res.xy, 0), iSample);
	#endif

	if (rP.z <= 0.05f)
		return 0;

	#ifndef USE_MSAA
		float4 rD = s_diffuse.Sample( smp_nofilter, tc );
		float4 rL = s_accumulator.Sample(smp_nofilter, tc);
	#else
		float4 rD = s_diffuse.Load(int3(tc * screen_res.xy, 0), iSample);
		float4 rL = s_accumulator.Load(int3(tc * screen_res.xy, 0), iSample);
	#endif
	
	// Remove emissive materials for now...
	if (length(rL) > 10.0f)
		rL = 0;

	float3 rN = gbuf_unpack_normal( rP.xy );
	float rMtl = gbuf_unpack_mtl( rP.w );
	float rHemi = gbuf_unpack_hemi( rP.w );

	float3 nw = mul( m_inv_V, rN );
	
	#ifdef SSFX_ENHANCED_SHADERS

		rL.rgb += rL.a * SRGBToLinear(rD.rgb);

		// hemisphere
		float3 hdiffuse, hspecular;
		hmodel(hdiffuse, hspecular, rMtl, rHemi, rD, rP, rN);

		// Final color 
		float3 rcolor = rL.rgb + hdiffuse.rgb;
		return LinearTosRGB(rcolor);

	#else

		// hemisphere
		float3 hdiffuse, hspecular;
		hmodel(hdiffuse, hspecular, rMtl, rHemi, rD.w, rP, rN);

		// Final color
		float4 light = float4(rL.rgb + hdiffuse, rL.w);
		float4 C = rD * light;
		float3 spec = C.www * rL.rgb;
		
		return C.rgb + spec;

	#endif
}

float3 SSFX_calc_sky(float3 dir)
{
	dir = SSFX_yaw_vector(dir, -sky_color.w); // Sky rotation
	
	dir.y = (dir.y - max(cos(dir.x) * 0.65f, cos(dir.z) * 0.65f)) * 2.1f; // Fix perspective
	dir.y -= -0.35; // Altitude
	
	float3 sky0 = sky_s0.SampleLevel(smp_base, dir, 0).xyz;
	float3 sky1 = sky_s1.SampleLevel(smp_base, dir, 0).xyz;
	
	// Use hemi color or real sky color if the modded executable is installed.
#ifndef SSFX_MODEXE
	return saturate(L_hemi_color.rgb * 3.0f) * lerp(sky0, sky1, L_ambient.w);
#else
	return saturate(sky_color.bgr * 3.0f) * lerp(sky0, sky1, L_ambient.w);
#endif
}

float3 SSFX_calc_env(float3 dir)
{
	float3 env0 = env_s0.SampleLevel(smp_base, dir, 0).xyz;
	float3 env1 = env_s1.SampleLevel(smp_base, dir, 0).xyz;
	
	return env_color.xyz * lerp( env0, env1, env_color.w );
}

// https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-to-shading/reflection-refraction-fresnel
// 1.3f water ~ 1.5f glass ~ 1.8f diamond
float SSFX_calc_fresnel(float3 V, float3 N, float ior)
{
	float cosi = clamp(-1, 1, dot(V, N));
	float etai = 1, etat = ior;
	if (cosi > 0)
	{
		etai = ior;
		etat = 1;
	}
	// Compute sini using Snell's law
	float sint = etai / etat * sqrt(max(0.f, 1 - cosi * cosi));
	// Total internal reflection
	if (sint >= 1)
		return 1.0f;

	float cost = sqrt(max(0.f, 1 - sint * sint));
	cosi = abs(cosi); 
	float Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost));
	float Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost));

	return (Rs * Rs + Rp * Rp) / 2;
}

static const float2x2 pp_rotation_matrix = { -0.666276f, 0.745705f, -0.745705f, -0.666276f };

float4 SSFX_Blur(float2 uv, float radius)
{
	float3 blur = 0;
	radius *= SSFX_gradient_noise_IGN(uv / 2.0 * screen_res.xy) * 6.28f;
	
	float2 offset = float2(radius, radius);
	float r = 0.9f;
	
	for (int i = 0; i < 16; i++) 
	{
		r += 1.0f / r; 
		offset = mul(offset, pp_rotation_matrix);
		blur += s_image.SampleLevel(smp_rtlinear, uv + (offset * (r - 1.0f) * ssfx_pixel_size), 0).rgb;
	}
	float3 image = blur / 16;
	
	return float4(image, 1.0f);
}