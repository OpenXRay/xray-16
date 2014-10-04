#include "common.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Pixel
half4 	main	( v_shadow_direct_aref I )	: COLOR
{
#ifdef 	USE_HWSMAP
	return 	tex2D		(s_base, I.tc0);
#else
	// 1. Base texture + kill pixels with low alpha
	half4 	C 	= tex2D		(s_base, I.tc0);
	clip		(C.w - def_aref);

	// 2.
	return I.depth;
#endif
}
