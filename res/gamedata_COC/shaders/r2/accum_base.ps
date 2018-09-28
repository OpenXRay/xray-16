#include "common.h"
#include "lmodel.h"
#include "shadow.h"
//////////////////////////////////////////////////////////////////////////////////////////
// This is the basic primitive used by convex, volumetric lights
// for example spot-lights, one face of the omni lights, etc.
//////////////////////////////////////////////////////////////////////////////////////////
// following options are available to configure compilation:
// USE_LMAP
// USE_LMAPXFORM
// USE_SHADOW
//////////////////////////////////////////////////////////////////////////////////////////
uniform float4              m_lmap        [2]        ;
float4         main         ( float4 tc : TEXCOORD0, float4 tcJ : TEXCOORD1 ) : COLOR
{
	float4 _P               = tex2Dproj         (s_position,         tc);
	half4  _N               = tex2Dproj         (s_normal,           tc);

	half 	m	= xmaterial	;
# ifndef USE_R2_STATIC_SUN
			m 	= _P.w		;
# endif
        // ----- light-model
      	half        		rsqr;
        half4        		light   = plight_local         	(m, _P, _N, Ldynamic_pos, Ldynamic_pos.w, rsqr);

        // ----- shadow
		half4          		P4      = half4                	(_P.x,_P.y,_P.z,1);
        float4         		PS      = mul                	(m_shadow,         P4);
        half         		s		= 1.h;
        #ifdef  USE_SHADOW
                #ifdef         USE_SJITTER
                  s         = shadowtest        (PS,tcJ);
                #else
                  s         = shadow        	(PS);
                #endif
        #endif

        // ----- lightmap
        half4         lightmap= 1.h;
        #ifdef        USE_LMAP
                #ifdef         USE_LMAPXFORM
              			PS.x         		= dot         	(P4, m_lmap[0]);
                        PS.y                = dot           (P4, m_lmap[1]);
                #endif
                lightmap= tex2Dproj         (s_lmap,        PS);        //
        #endif

        return         blendp        (Ldynamic_color * light * s * lightmap, tc);
}
