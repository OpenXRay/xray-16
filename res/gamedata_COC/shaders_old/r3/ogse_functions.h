#ifndef OGSE_FUNCTIONS_H
#define OGSE_FUNCTIONS_H

#define SKY_WITH_DEPTH				// sky renders with depth to avoid some problems with reflections
#define SKY_DEPTH float(10000.f)
#define SKY_EPS float(0.001)
#define FARPLANE float(180.0)

float normalize_depth(float depth)
{
	return saturate(depth/FARPLANE);
}

static const float2 poisson_disk[12] = {				// диск Пуассона
	float2(0.2636057f, 0.4026764f),
	float2(0.8931927f, 0.3809958f),
	float2(-0.351227f, 0.9287762f),
	float2(-0.4619994f, -0.001132701f),
	float2(0.1122695f, 0.9784672f),
	float2(0.6323467f, -0.2056593f),
	float2(-0.08509368f, -0.2938382f),
	float2(-0.2061059f, 0.3703068f),
	float2(-0.5982098f, -0.5618949f),
	float2(-0.8395036f, 0.4914388f),
	float2(-0.9885221f, 0.02723012f),
	float2(0.1027245f, -0.8669642f),
};

// new uniform variables - OGSE Team
// global constants
//uniform float4 ogse_c_resolution;	// x - width, y - height, z - 1/width, w - 1/height
uniform float4 ogse_c_screen;		// x - fFOV, y - fAspect, z - Zf/(Zf-Zn), w - Zn*tan(fFov/2)
/*uniform float4 ogse_c_jitter;		// x - jitter u, y - jitter v. Test for smaa t2x, don't touch
uniform float4 ogse_c_rain;			// x - rmap dist, y - flood level, z - wet level, w - rain intensity
uniform float4 ogse_c_various;		// x - moonroad flag from weather*/

#ifndef SKY_WITH_DEPTH
half is_sky(float depth)		{return step(depth, SKY_EPS);}
half is_not_sky(float depth)	{return step(SKY_EPS, depth);}
#else
half is_sky(float depth)		{return step(abs(depth - SKY_DEPTH), SKY_EPS);}
half is_not_sky(float depth)	{return step(SKY_EPS, abs(depth - SKY_DEPTH));}
#endif

float4 proj_to_screen(float4 proj)
{
	float4 screen = proj;
	screen.x = (proj.x + proj.w);
	screen.y = (proj.w - proj.y);
	screen.xy *= 0.5;
	return screen;
}
float4 screen_to_proj(float2 screen, float z)
{
	float4 proj;
	proj.w = 1.0;
	proj.z = z;
	proj.x = screen.x*2 - proj.w;
	proj.y = -screen.y*2 + proj.w;
	return proj;
}
float is_in_range(float3 args)
{
	float mn = (args.x > args.y) ? 1: 0;
	float mx = (args.z > args.x) ? 1: 0;
	return mn*mx;
}

#endif