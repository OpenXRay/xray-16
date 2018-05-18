#include "common.h"
#include "lmodel.h"

half4 	main		( float4 tc:TEXCOORD0 )	: COLOR
{
  const half bias_mul 	= 0.999f;

  // Sample the fat framebuffer:
  half4 _P		= tex2Dproj 	(s_position, tc); 
  half4 _N		= tex2Dproj 	(s_normal,   tc); 

	half 	m	= xmaterial	;
# ifndef USE_R2_STATIC_SUN
			m 	= _P.w		;
# endif

  half	rsqr;
  half4	light 		= plight_local 	(m, _P, _N, Ldynamic_pos, Ldynamic_pos.w, rsqr);
	return 	blendp	(Ldynamic_color * light, tc);
}
