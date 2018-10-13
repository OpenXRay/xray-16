#include "common.h"

//	TODO: DX10: move to load instead of sample (will need to provide integer texture coordinates)
#define EPS	(0.9f/255.f)
//#define EPS	(40.f/255.f)
#define CLIP_THRESHOLD	(1.0f/255.f)
#ifdef MSAA_OPTIMIZATION
#ifdef GBUFFER_OPTIMIZATION
float4 main ( p_TL I, float4 pos2d : SV_Position, uint iSample : SV_SAMPLEINDEX ) : SV_Target
#else
float4 main ( p_TL I, uint iSample : SV_SAMPLEINDEX ) : SV_Target
#endif
#else
#ifdef GBUFFER_OPTIMIZATION
float4 main ( p_TL I, float4 pos2d : SV_Position ) : SV_Target
#else
float4 main ( p_TL I ) : SV_Target
#endif
#endif
{
  	// Sample the fat framebuffer:
  	//float4	NH		= tex2D( s_normal, I.Tex0); 

	gbuffer_data gbd = gbuffer_load_data( GLD_P(I.Tex0, pos2d, ISAMPLE) );

	float4	NH		= float4( gbd.N, gbd.hemi );
  	float	L 		= NH.w * dot( Ldynamic_dir, (float3)NH ) + EPS; // Use hemisphere as approximation of max light
//float	L 		= dot( Ldynamic_dir, (float3)NH ) + EPS; // Use hemisphere as approximation of max light

//	L = 1;

	clip(L-CLIP_THRESHOLD);

  	return	float4( L, L, L, L );
}

