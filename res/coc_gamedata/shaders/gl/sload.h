#ifndef SLOAD_H
#define SLOAD_H

#include "common.h"

#ifdef	MSAA_ALPHATEST_DX10_1
#if MSAA_SAMPLES == 2
const float2 MSAAOffsets[2] = { float2(4,4), float2(-4,-4) };
#endif
#if MSAA_SAMPLES == 4
const float2 MSAAOffsets[4] = { float2(-2,-6), float2(6,-2), float2(-6,2), float2(2,6) };
#endif
#if MSAA_SAMPLES == 8
const float2 MSAAOffsets[8] = { float2(1,-3), float2(-1,3), float2(5,1), float2(-3,-5), 
								               float2(-5,5), float2(-7,-1), float2(3,7), float2(7,-7) };
#endif
#endif	//	MSAA_ALPHATEST_DX10_1

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
   return	tex2D( s_base, tc );
}

#if defined(ALLOW_STEEPPARALLAX) && defined(USE_STEEPPARALLAX)

const float fParallaxStartFade = 8.0;
const float fParallaxStopFade = 12.0;

void UpdateTC( inout p_bumped I )
{
	if (I.position.z < fParallaxStopFade)
	{
		const float maxSamples = 25;
		const float minSamples = 5;
		const float fParallaxOffset = -0.013;

		float3	 eye = mul (float3x3(I.M1.x, I.M2.x, I.M3.x,
									 I.M1.y, I.M2.y, I.M3.y,
									 I.M1.z, I.M2.z, I.M3.z), -I.position.xyz);

		eye = normalize(eye);
		
		//	Calculate number of steps
		float nNumSteps = lerp( maxSamples, minSamples, eye.z );

		float	fStepSize			= 1.0 / nNumSteps;
		float2	vDelta				= eye.xy * fParallaxOffset*1.2;
		float2	vTexOffsetPerStep	= fStepSize * vDelta;

		//	Prepare start data for cycle
		float2	vTexCurrentOffset	= I.tcdh;
		float	fCurrHeight			= 0.0;
		float	fCurrentBound		= 1.0;

		for( int i=0; i<nNumSteps; ++i )
		{
			if (fCurrHeight < fCurrentBound)
			{	
				vTexCurrentOffset += vTexOffsetPerStep;		
				fCurrHeight = textureLod( s_bumpX, vTexCurrentOffset.xy, 0 ).a; 
				fCurrentBound -= fStepSize;
			}
		}

/*
		[unroll(25)]	//	Doesn't work with [loop]
		for( ;fCurrHeight < fCurrentBound; fCurrentBound -= fStepSize )
		{
			vTexCurrentOffset += vTexOffsetPerStep;		
			fCurrHeight = s_bumpX.SampleLevel( smp_base, vTexCurrentOffset.xy, 0 ).a; 
		}
*/
		//	Reconstruct previouse step's data
		vTexCurrentOffset -= vTexOffsetPerStep;
		float fPrevHeight = textureLod( s_bumpX, vTexCurrentOffset.xy, 0 ).a;

		//	Smooth tc position between current and previouse step
		float	fDelta2 = ((fCurrentBound + fStepSize) - fPrevHeight);
		float	fDelta1 = (fCurrentBound - fCurrHeight);
		float	fParallaxAmount = (fCurrentBound * fDelta2 - (fCurrentBound + fStepSize) * fDelta1 ) / ( fDelta2 - fDelta1 );
		float	fParallaxFade 	= smoothstep(fParallaxStopFade, fParallaxStartFade, I.position.z);
		float2	vParallaxOffset = vDelta * ((1- fParallaxAmount )*fParallaxFade);
		float2	vTexCoord = I.tcdh + vParallaxOffset;
	
		//	Output the result
		I.tcdh = vTexCoord;

#if defined(USE_TDETAIL) && defined(USE_STEEPPARALLAX)
		I.tcdbump = vTexCoord * dt_params.xy;
#endif
	}

}

#elif	defined(USE_PARALLAX) || defined(USE_STEEPPARALLAX)

void UpdateTC( inout p_bumped I)
{
	float3	 eye = mul (float3x3(I.M1.x, I.M2.x, I.M3.x,
								 I.M1.y, I.M2.y, I.M3.y,
								 I.M1.z, I.M2.z, I.M3.z), -I.position.xyz);
								 
	float	height	= tex2D( s_bumpX, I.tcdh ).w;	//
			//height  /= 2;
			//height  *= 0.8;
			height	= height*(parallax.x) + (parallax.y);	//
	float2	new_tc  = I.tcdh + height * normalize(eye).xy;	//

	//	Output the result
	I.tcdh	= new_tc;
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

	float4 	Nu	= tex2D( s_bump, I.tcdh );		// IN:	normal.gloss
	float4 	NuE	= tex2D( s_bumpX, I.tcdh);		// IN:	normal_error.height

	S.base		= tbase(I.tcdh);				//	IN:  rgb.a
	S.normal	= Nu.wzy + (NuE.xyz - 1.0);	//	(Nu.wzyx - 0.5) + (E-0.5)
	S.gloss		= Nu.x*Nu.x;					//	S.gloss = Nu.x*Nu.x;
	S.height	= NuE.z;
	//S.height	= 0;

#ifdef        USE_TDETAIL
#ifdef        USE_TDETAIL_BUMP
	float4 NDetail		= tex2D( s_detailBump, I.tcdbump);
	float4 NDetailX		= tex2D( s_detailBumpX, I.tcdbump);
	S.gloss				= S.gloss * NDetail.x * 2.0;
	//S.normal			+= NDetail.wzy-0.5;
	S.normal			+= NDetail.wzy + NDetailX.xyz - 1.0; //	(Nu.wzyx - 0.5) + (E-0.5)

	float4 detail		= tex2D( s_detail, I.tcdbump);
	S.base.rgb			= S.base.rgb * detail.rgb * 2;

//	S.base.rgb			= float3(1,0,0);
#else        //	USE_TDETAIL_BUMP
	float4 detail		= tex2D( s_detail, I.tcdbump);
	S.base.rgb			= S.base.rgb * detail.rgb * 2.0;
	S.gloss				= S.gloss * detail.w * 2.0;
#endif        //	USE_TDETAIL_BUMP
#endif

	return S;
}

surface_bumped sload_i( p_bumped I, float2 pixeloffset )
{
	surface_bumped	S;
   
   // apply offset
#ifdef	MSAA_ALPHATEST_DX10_1
   I.tcdh.xy += pixeloffset.x * ddx(I.tcdh.xy) + pixeloffset.y * ddy(I.tcdh.xy);
#endif

	UpdateTC(I);	//	All kinds of parallax are applied here.

	float4 	Nu	= tex2D( s_bump, I.tcdh );		// IN:	normal.gloss
	float4 	NuE	= tex2D( s_bumpX, I.tcdh);	// IN:	normal_error.height

	S.base		= tbase(I.tcdh);				//	IN:  rgb.a
	S.normal	= Nu.wzy + (NuE.xyz - 1.0);	//	(Nu.wzyx - 0.5) + (E-0.5)
	S.gloss		= Nu.x*Nu.x;					//	S.gloss = Nu.x*Nu.x;
	S.height	= NuE.z;
	//S.height	= 0;

#ifdef        USE_TDETAIL
#ifdef        USE_TDETAIL_BUMP
#ifdef MSAA_ALPHATEST_DX10_1
#if ( (!defined(ALLOW_STEEPPARALLAX) ) && defined(USE_STEEPPARALLAX) )
   I.tcdbump.xy += pixeloffset.x * ddx(I.tcdbump.xy) + pixeloffset.y * ddy(I.tcdbump.xy);
#endif
#endif

	float4 NDetail		= tex2D( s_detailBump, I.tcdbump);
	float4 NDetailX		= tex2D( s_detailBumpX, I.tcdbump);
	S.gloss				= S.gloss * NDetail.x * 2.0;
	//S.normal			+= NDetail.wzy-0.5;
	S.normal			+= NDetail.wzy + NDetailX.xyz - 1.0; //	(Nu.wzyx - 0.5) + (E-0.5)

	float4 detail		= tex2D( s_detail, I.tcdbump);
	S.base.rgb			= S.base.rgb * detail.rgb * 2;

//	S.base.rgb			= float3(1,0,0);
#else        //	USE_TDETAIL_BUMP
#ifdef MSAA_ALPHATEST_DX10_1
	I.tcdbump.xy += pixeloffset.x * ddx(I.tcdbump.xy) + pixeloffset.y * ddy(I.tcdbump.xy);
#endif
	float4 detail		= tex2D( s_detail, I.tcdbump);
	S.base.rgb			= S.base.rgb * detail.rgb * 2.0;
	S.gloss				= S.gloss * detail.w * 2.0;
#endif        //	USE_TDETAIL_BUMP
#endif

	return S;
}

surface_bumped sload ( p_bumped I)
{
      surface_bumped      S   = sload_i	(I);
		S.normal.z			*=	0.5;		//. make bump twice as contrast (fake, remove me if possible)

#ifdef	GBUFFER_OPTIMIZATION
	   S.height = 0;
#endif	//	GBUFFER_OPTIMIZATION
      return              S;
}

surface_bumped sload ( p_bumped I, float2 pixeloffset )
{
      surface_bumped      S   = sload_i	(I, pixeloffset );
		S.normal.z			*=	0.5;		//. make bump twice as contrast (fake, remove me if possible)
#ifdef	GBUFFER_OPTIMIZATION
	   S.height = 0;
#endif	//	GBUFFER_OPTIMIZATION
      return              S;
}

#endif
