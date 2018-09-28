#include "common.h"
#include "shared\waterconfig.h"

struct   v2p
{
        half2        tbase:                TEXCOORD0;  // base
        half2        tdist0:                TEXCOORD1;  // d0
        half2        tdist1:                TEXCOORD2;  // d1
	float4      tctexgen    :         TEXCOORD3        ;
};

uniform sampler2D        s_distort;
#define POWER        .5h
//////////////////////////////////////////////////////////////////////////////////////////
// Pixel
half4        main        ( v2p I )        : COLOR
{
        half4        t_base                = tex2D        (s_base,          I.tbase  );

        half2        t_d0                = tex2D (s_distort,           I.tdist0 );
        half2        t_d1                = tex2D (s_distort,           I.tdist1 );
        half2        distort                = (t_d0+t_d1)*0.5;                      // average
        half2        zero                = half2        (0.5,0.5);
        half2        faded                = lerp  (distort,zero,        t_base.a );

	//	Igor: additional depth test
#ifdef	USE_SOFT_WATER
#ifdef	NEED_SOFT_WATER
	half	alphaDistort;
	half4 _P               = tex2Dproj         (s_position,         I.tctexgen);
	half waterDepth = _P.z-I.tctexgen.z;
	alphaDistort = saturate(5*waterDepth);
	faded = lerp  ( zero, faded, alphaDistort);
#endif	//	NEED_SOFT_WATER
#endif	//	USE_SOFT_WATER & NEED_SOFT_WATER

        half2        faded_bx2        = (faded*2-1)*W_DISTORT_POWER;
        half         faded_dot        = dot        (half3(faded_bx2,0),.75        );        //0.75
        half         alpha                = .5f        ;                //abs          (faded_dot);


                faded                = faded*POWER - .5*POWER + 0.5        ;

        // out
	//	Igor: need for alpha water
#ifdef	NEED_SOFT_WATER
	return  half4  (faded, 0.0h, alpha);
#else	//	NEED_SOFT_WATER
        return  half4  (faded, .08h, alpha);
#endif	//	NEED_SOFT_WATER
}
