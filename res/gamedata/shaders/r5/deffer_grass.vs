#include "common.h"
#include "check_screenspace.h"

float4 benders_pos[32];
float4 benders_setup;

float4 consts; // {1/quant,1/quant,diffusescale,ambient}
float4 wave; // cx,cy,cz,tm
float4 dir2D; 
#if INSTANCED_DETAILS
Texture1D<float4> array;
#else
//tbuffer DetailsData
//{
	uniform float4 		array[61*4];
//}
#endif

#if INSTANCED_DETAILS
v2p_bumped 	main (v_detail v, uint instance_id : SV_InstanceID)
#else
v2p_bumped 	main (v_detail v)
#endif
{
	v2p_bumped 		O;
	// index

#if INSTANCED_DETAILS
	int 	i 	= instance_id * 2;

	float4 a0 = array.Load(int2(i, 0), 0);
	float4 a1 = array.Load(int2(i, 0), 1);

	float4  m0 	= float4(a0.y,    0, -a0.x, a1.x);
	float4  m1 	= float4(   0, a1.w,     0, a1.y);
	float4  m2 	= float4(a0.x,    0,  a0.y, a1.z);
	float4  c0 	= a0.zzzw;
#else
	int 	i 	= v.misc.w;
	float4  m0 	= array[i+0];
	float4  m1 	= array[i+1];
	float4  m2 	= array[i+2];
	float4  c0 	= array[i+3];
#endif

	// Transform pos to world coords
	float4 pos;
 	pos.x = dot(m0, v.pos);
 	pos.y = dot(m1, v.pos);
 	pos.z = dot(m2, v.pos);
	pos.w = 1;
	
	//Wave effect
	float base = m1.w;
	float dp = calc_cyclic(dot(pos, wave));
	float H = pos.y - base; 			// height of vertex (scaled)
	float frac = v.misc.z * consts.x;	// fractional
	float inten = H * dp;
	float2 result = calc_xz_wave(dir2D.xz * inten, frac);
	
	// Add wind
	pos = float4(pos.x + result.x, pos.y, pos.z + result.y, 1);
	
	// INTERACTIVE GRASS - SSS Update 15.4
	// https://www.moddb.com/mods/stalker-anomaly/addons/screen-space-shaders/
#ifdef SSFX_INTER_GRASS
#if SSFX_INT_GRASS > 0
	for (int b = 0; b < SSFX_INT_GRASS + 1; b++)
	{
		// Direction, Radius & Bending Strength, Distance and Height Limit
		float3 dir = benders_pos[b + 16].xyz;
		float3 rstr = float3(benders_pos[b].w, benders_pos[b + 16].ww);
		bool non_dynamic = rstr.x <= 0 ? true : false;
		float dist = distance(pos.xz, benders_pos[b].xz);
		float height_limit = 1.0f - saturate(abs(pos.y - benders_pos[b].y) / ( non_dynamic ? 2.0f : rstr.x ));
		height_limit *= H;

		// Adjustments ( Fix Radius or Dynamic Radius )
		rstr.x = non_dynamic ? benders_setup.x : rstr.x;
 		rstr.yz *= non_dynamic ? benders_setup.yz : 1.0f;

		// Strength through distance and bending direction.
		float bend = 1.0f - saturate(dist / (rstr.x + 0.001f));
		float3 bend_dir = normalize(pos.xyz - benders_pos[b].xyz) * bend;
		float3 dir_limit = dir.y >= -1 ? saturate(dot(bend_dir.xyz, dir.xyz) * 5.0f) : 1.0f; // Limit if nedeed

		// Apply direction limit
		bend_dir.xz *= dir_limit.xz;

		// Apply vertex displacement
		pos.xz += bend_dir.xz * 2.0f * rstr.yy * height_limit; 			// Horizontal
		pos.y -= bend * 0.6f * rstr.z * height_limit * dir_limit.y;		// Vertical
	}
#endif
#endif

	// FLORA FIXES & IMPROVEMENTS - SSS Update 14.6
	// https://www.moddb.com/mods/stalker-anomaly/addons/screen-space-shaders/
	
	// Fake Normal, Bi-Normal and Tangent
	float3 N = normalize(float3(pos.x - m0.w, pos.y - m1.w + 1.0f, pos.z - m2.w));

	float3x3 xform	= mul	((float3x3)m_WV, float3x3(
						0,0,N.x,
						0,0,N.y,
						0,0,N.z
					));

	// Feed this transform to pixel shader
	O.M1 			= xform[0];
	O.M2 			= xform[1];
	O.M3 			= xform[2];

	// Eye-space pos/normal
	float 	hemi 	= clamp(c0.w, 0.05f, 1.0f); // Some spots are bugged ( Full black ), better if we limit the value till a better solution. // Option -> v_hemi(N);
	float3	Pe		= mul		(m_V,  	pos		);
	O.tcdh 			= float4	((v.misc * consts).xyyy);
	O.hpos 			= mul		(m_VP,	pos		);
	O.position		= float4	(Pe, 	hemi	);

#if defined(USE_R2_STATIC_SUN) && !defined(USE_LM_HEMI)
	O.tcdh.w		= hemi * c_sun.x + c_sun.y;					// (,,,dir-occlusion)
#endif

	return O;
}
FXVS;
