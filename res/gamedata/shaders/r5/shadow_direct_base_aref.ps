#include "common.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Pixel
float4 main ( p_shadow_direct_aref I ) : SV_Target
{
#ifdef 	USE_HWSMAP
//	return 	tex2D		(s_base, I.tc0);
	float4 	C 	= s_base.Sample( smp_linear, I.tc0);
	clip	(C.w - def_aref);
	return  C;
#else
	// 1. Base texture + kill pixels with low alpha
	float4 	C 	= tex2D		(s_base, I.tc0);
	clip		(C.w - def_aref);

	// 2.
	return I.depth;
#endif
}
