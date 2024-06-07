#define USE_ERROR_CORRECTION
//#define RECALCULATENORMALZ
//#define NORMALIZE_TEXTURES
static const float NORMAL_STRENGTH = 1.0;
static const float DETAIL_STRENGTH = 1.0;
static const float DETAIL_TINT = 1.0;
static const float DETAIL_GLOSS = 1.0;

#ifndef SLOAD_H
#define SLOAD_H

#include "common.h"

#ifdef	MSAA_ALPHATEST_DX10_1
#if MSAA_SAMPLES == 2
static const float2 MSAAOffsets[2] = { float2(4,4), float2(-4,-4) };
#endif
#if MSAA_SAMPLES == 4
static const float2 MSAAOffsets[4] = { float2(-2,-6), float2(6,-2), float2(-6,2), float2(2,6) };
#endif
#if MSAA_SAMPLES == 8
static const float2 MSAAOffsets[8] = { float2(1,-3), float2(-1,3), float2(5,1), float2(-3,-5), 
								               float2(-5,5), float2(-7,-1), float2(3,7), float2(7,-7) };
#endif
#endif	//	MSAA_ALPHATEST_DX10_1



//////////////////////////////////////////////////////////////////////////////////////////
// Texture samplers and blenders             //
//////////////////////////////////////////////////////////////////////////////////////////

float3 SampleNormal(float4 N, float4 NE)
{
	float3 Norm = unpack_normal(N.wzy);
#ifdef	USE_ERROR_CORRECTION
	Norm += unpack_normal(NE.xyz);
#endif
#ifdef	RECALCULATENORMALZ
	Norm.z = sqrt(1 - saturate(dot(Norm.xy, Norm.xy)));
#endif
#ifdef	NORMALIZE_TEXTURES
	Norm = normalize(Norm);
#endif
	return Norm;
}

float3 NormalStrength(float3 N, float Strength)
{
	if(Strength != 1.0)
	{
	N.xy *= Strength;
	N.z = sqrt(1 - saturate(dot(N.xy, N.xy)));
	N = normalize(N);
	}
	return N;
}

float SampleGloss(float4 N)
{
	return N.x;
}

float SampleHeight(float4 NE)
{
	return NE.w;
}

float3 ApplyDetailAlbedo(float3 A1, float3 A2)
{
	//return saturate(A1 * A2 * 2);
	return saturate(A1 * exp2(DETAIL_TINT * (A2 * 2 - 1)));
}

float3 ApplyDetailNormal(float3 N1, float3 N2)
{
	N1 += float3( 0,  0, 1);
	N2 *= float3(-1, -1, 1);
	return normalize(N1*dot(N1, N2)/N1.z - N2);
}

float ApplyDetailGloss(float G1, float G2)
{
	//return saturate(G1 * G2 * 2);
	//return saturate(G1 + (DETAIL_GLOSS * (G2 * 2 - 1)));
	return saturate(G1 * exp2(DETAIL_GLOSS * (G2 * 2 - 1)));
}

float ApplyDetailHeight(float H1, float H2)
{
	return H1 + (H2 * 2 - 1);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Bumped surface loader                //
//////////////////////////////////////////////////////////////////////////////////////////
struct	surface_bumped
{
	float4	base;
	float3	normal;
	float	gloss;
	float	height;

};

float4 tbase( float2 tc )
{
   return	s_base.Sample( smp_base, tc);
}

#if defined(ALLOW_STEEPPARALLAX) && defined(USE_STEEPPARALLAX)

//Always remember to check defines, variables, and shit.... god
#define PARALLAX_NEAR_PLANE 0.01
#define PARALLAX_FAR_PLANE 35
#define PARALLAX_DEPTH 0.045


//Ok, we can comment old gsc parallax now.
//We need to change input, to p_bumped struct like in GSC shader
//We also change name of this function to GSC 
//Time to change input to stuff from p_bumped.
void UpdateTC( inout p_bumped I)
{
	//Here's "limited" range of parallax. We use linear depth (z vector of view space position) to do that
	if ((I.position.z > PARALLAX_NEAR_PLANE) && (I.position.z < PARALLAX_FAR_PLANE))
	{
		//That M1/M2/M3 stuff is our TBN matrix (we aligin tangent normals/vectors to to just geometry normals
		float3 eye = normalize(mul(float3x3(I.M1.x, I.M2.x, I.M3.x,
			I.M1.y, I.M2.y, I.M3.y,
			I.M1.z, I.M2.z, I.M3.z), -I.position.xyz));

		// steps minmax and refines minmax
		int4 steps = int4(3, 10, 7, 16); // 3..10, 7..16

		bool need_disp_lerp = true;
		bool need_refine = true; //Thats refinement steps, used to smoothout raymarched results

		float view_angle = abs(dot(float3(0.0, 0.0, 1.0), eye));

		float layer_step = rcp(lerp(steps.y, steps.x, view_angle));

		//float2 tc_step = layer_step * eye.xy * PARALLAX_DEPTH);
		float2 tc_step = layer_step * eye.xy * (parallax.x);

		//Now, we have to change this huita. p.tcdbump is our "tiled" texture coordinate
		//I.tcdh is our "normal" texcoord, lets see above
		float2 displaced_tc = I.tcdh;

		float curr_disp, curr_layer = 0.0;

		do
		{
			displaced_tc -= tc_step;
			curr_disp = 1 - s_bumpX.SampleLevel(smp_base, displaced_tc, 0).w; //Our heightmap sampler 
			curr_layer += layer_step;
		} while (curr_layer < curr_disp);

		if (need_refine)
		{
			displaced_tc += tc_step;
			curr_layer -= layer_step;

			float refine_steps = lerp(steps.w, steps.z, view_angle);

			tc_step /= refine_steps;
			layer_step /= refine_steps;

			do
			{
				displaced_tc -= tc_step;
				curr_disp = 1.0 - s_bumpX.SampleLevel(smp_base, displaced_tc, 0).w;
				curr_layer += layer_step;
			} while (curr_layer < curr_disp);
		}

		if (need_disp_lerp)
		{
			float2 displaced_tc_prev = displaced_tc + tc_step;

			float after_depth = curr_disp - curr_layer;
			float before_depth = 1.0 - s_bumpX.SampleLevel(smp_base, displaced_tc_prev, 0).w - curr_layer + layer_step; //Another sampler name

			float weight = after_depth / (after_depth - before_depth);

			displaced_tc = lerp(displaced_tc, displaced_tc_prev, weight);
		}
		
		//Tiling for detail/tiled textures
	#if defined(USE_TDETAIL) && defined(USE_STEEPPARALLAX)
		I.tcdbump = I.tcdh * dt_params; //tiled UV
		I.tcdbump += displaced_tc - I.tcdh; //offset
	#endif		
	
		I.tcdh = displaced_tc;
	}
}

#elif	defined(USE_PARALLAX) || defined(USE_STEEPPARALLAX)

void UpdateTC( inout p_bumped I)
{
	float3	 eye = mul (float3x3(I.M1.x, I.M2.x, I.M3.x,
								 I.M1.y, I.M2.y, I.M3.y,
								 I.M1.z, I.M2.z, I.M3.z), -I.position.xyz);
								 
	float	height	= s_bumpX.Sample( smp_base, I.tcdh).w;	//
			//height  /= 2;
			//height  *= 0.8;
			height	= height*(parallax.x) + (parallax.y);	//
	float2	new_tc  = I.tcdh + height * normalize(eye);	//

		//Tiling for detail/tiled textures
	#if defined(USE_TDETAIL) && defined(USE_STEEPPARALLAX)
		I.tcdbump = I.tcdh * dt_params + height * normalize(eye);
	#endif	
	
	//	Output the result
	I.tcdh.xy	= new_tc;
}

#else	//	USE_PARALLAX

void UpdateTC( inout p_bumped I)
{
	;
}

#endif	//	USE_PARALLAX

surface_bumped sload_i( p_bumped I)
{
	surface_bumped	S;
   
	UpdateTC(I);	//	All kinds of parallax are applied here.
	
	//Base textures
	//
	S.base = tbase(I.tcdh);
	float4 Nu = s_bump.Sample( smp_base, I.tcdh );
	float4 NuE = s_bumpX.Sample( smp_base, I.tcdh);
	float3 TangentNormal = NormalStrength(SampleNormal(Nu, NuE), NORMAL_STRENGTH);
	S.normal = TangentNormal;
	S.gloss = SampleGloss(Nu);
	S.height = SampleHeight(NuE);
	
	//Detail textures
	//
#ifdef USE_TDETAIL
	float4 detail = s_detail.Sample( smp_base, I.tcdbump);
	S.base.rgb = ApplyDetailAlbedo(S.base.rgb, detail.rgb);
#ifdef USE_TDETAIL_BUMP
	float4 NDetail = s_detailBump.Sample( smp_base, I.tcdbump);
	float4 NDetailX = s_detailBumpX.Sample( smp_base, I.tcdbump);
	float3 DetailNormal = NormalStrength(SampleNormal(NDetail, NDetailX), DETAIL_STRENGTH);
	S.normal = ApplyDetailNormal(TangentNormal, DetailNormal);
	//float DetailGloss = SampleGloss(NDetail);
	float DetailGloss = detail.w; //higher res most of the time
	float DetailHeight = SampleHeight(NDetailX);
	S.height = ApplyDetailHeight(S.height, DetailHeight);
#else
	float DetailGloss = detail.w;
#endif
	S.gloss = ApplyDetailGloss(S.gloss, DetailGloss);
#endif
	//
	
	return S;
}

surface_bumped sload ( p_bumped I)
{
	return	sload_i	(I);
}

surface_bumped sload ( p_bumped I, float2 pixeloffset )
{
   // apply offset
#ifdef	MSAA_ALPHATEST_DX10_1
   I.tcdh.xy += pixeloffset.x * ddx(I.tcdh.xy) + pixeloffset.y * ddy(I.tcdh.xy);
#ifdef USE_TDETAIL
   I.tcdbump.xy += pixeloffset.x * ddx(I.tcdbump.xy) + pixeloffset.y * ddy(I.tcdbump.xy);
#endif
#endif

	return	sload_i	(I);
}

#endif
