#include "common.h"
#include "lmodel.h"

float4 	main		( float4 tc:TEXCOORD0 )	: COLOR
{
  const float bias_mul 	= 0.999f;

  // Sample the fat framebuffer:
  float4 _P		= tex2Dproj 	(s_position, tc); 
  float4 _N		= tex2Dproj 	(s_normal,   tc); 

	float 	m	= xmaterial	;
# ifndef USE_R2_STATIC_SUN
			m 	= _P.w		;
# endif

  float	rsqr;
  float4	light 		= plight_local 	(m, _P, _N, Ldynamic_pos, Ldynamic_pos.w, rsqr);
	return 	blendp	(Ldynamic_color * light, tc);
}
