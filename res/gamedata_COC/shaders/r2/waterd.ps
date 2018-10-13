#include "common.h"
#include "shared\waterconfig.h"

struct   v2p
{
        float2        tbase:                TEXCOORD0;  // base
        float2        tdist0:                TEXCOORD1;  // d0
        float2        tdist1:                TEXCOORD2;  // d1
	float4      tctexgen    :         TEXCOORD3        ;
};

uniform sampler2D        s_distort;
#define POWER        .5h
//////////////////////////////////////////////////////////////////////////////////////////
// Pixel
float4        main        ( v2p I )        : COLOR
{
        float4        t_base                = tex2D        (s_base,          I.tbase  );

        float2        t_d0                = tex2D (s_distort,           I.tdist0 );
        float2        t_d1                = tex2D (s_distort,           I.tdist1 );
        float2        distort                = (t_d0+t_d1)*0.5;                      // average
        float2        zero                = float2        (0.5,0.5);
        float2        faded                = lerp  (distort,zero,        t_base.a );

	//	Igor: additional depth test
#ifdef	USE_SOFT_WATER
#ifdef	NEED_SOFT_WATER
	float	alphaDistort;
	float4 _P               = tex2Dproj         (s_position,         I.tctexgen);
	float waterDepth = _P.z-I.tctexgen.z;
	alphaDistort = saturate(5*waterDepth);
	faded = lerp  ( zero, faded, alphaDistort);
#endif	//	NEED_SOFT_WATER
#endif	//	USE_SOFT_WATER & NEED_SOFT_WATER

        float2        faded_bx2        = (faded*2-1)*W_DISTORT_POWER;
        float         faded_dot        = dot        (float3(faded_bx2,0),.75        );        //0.75
        float         alpha                = .5f        ;                //abs          (faded_dot);


                faded                = faded*POWER - .5*POWER + 0.5        ;

        // out
	//	Igor: need for alpha water
#ifdef	NEED_SOFT_WATER
	return  float4  (faded, 0.0h, alpha);
#else	//	NEED_SOFT_WATER
        return  float4  (faded, .08h, alpha);
#endif	//	NEED_SOFT_WATER
}
