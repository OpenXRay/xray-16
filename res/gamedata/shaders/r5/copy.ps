#include "common.h"

#ifndef ISAMPLE
#define ISAMPLE 0
#endif

//////////////////////////////////////////////////////////////////////////////////////////
// Pixel
//	TODO: DX10: move to load instead of sample (will need to provide integer texture coordinates)
#ifndef MSAA_OPTIMIZATION
float4 main( float2 tc : TEXCOORD0 ) : SV_Target
#else
float4 main( float2 tc : TEXCOORD0, uint iSample : SV_SAMPLEINDEX ) : SV_Target
#endif
{
	//return	tex2D		(s_base,tc);
#ifndef USE_MSAA
	return		s_generic.Sample( smp_nofilter, tc );
#else
#ifndef MSAA_OPTIMIZATION
	return		s_generic.Load( int3( tc * pos_decompression_params2.xy, 0 ), ISAMPLE );
#else
	return		s_generic.Load( int3( tc * pos_decompression_params2.xy, 0 ), iSample );
#endif
#endif
}