#include "common.h"
#include "shared\waterconfig.h"
#include "shared\watermove.h"

struct        v_vert
{
	float4	P		: POSITION;
	float4	N		: NORMAL;
	float4	T		: TANGENT;
	float4	B		: BINORMAL;
	float4	color	: COLOR0;
	int2	uv		: TEXCOORD0;
};

struct   vf
{
	float2	tbase	: TEXCOORD0;
	float2	tnorm0	: TEXCOORD1;
	float2	tnorm1	: TEXCOORD2;
	float3	M1		: TEXCOORD3;
	float3	M2		: TEXCOORD4;
	float3	M3		: TEXCOORD5;
	float3	v2point	: TEXCOORD6;
#ifdef	USE_SOFT_WATER
#ifdef	NEED_SOFT_WATER
	float4	tctexgen: TEXCOORD7;
#endif
#endif
	float4	c0		: COLOR0;
	float	fog		: FOG;
	float4	hpos	: SV_Position;
};

uniform float4x4	m_texgen;

vf main (v_vert v)
{
	v.N		=	unpack_D3DCOLOR(v.N);
	v.T		=	unpack_D3DCOLOR(v.T);
	v.B		=	unpack_D3DCOLOR(v.B);
	v.color	=	unpack_D3DCOLOR(v.color);

        vf                 o;

        float4         P         = v.P        ;
        float3         NN         = unpack_normal        (v.N)        ;
                P         = watermove        (P)        ;

        o.v2point        = P-eye_position        ;
        o.tbase                = unpack_tc_base        (v.uv,v.T.w,v.B.w);
        o.tnorm0        = watermove_tc                 (o.tbase*W_DISTORT_BASE_TILE_0, P.xz, W_DISTORT_AMP_0);
        o.tnorm1        = watermove_tc                 (o.tbase*W_DISTORT_BASE_TILE_1, P.xz, W_DISTORT_AMP_1);


        float3          N         = unpack_bx2(v.N);
        float3          T         = unpack_bx2(v.T);
        float3          B         = unpack_bx2(v.B);
        float3x3 xform        = mul        ((float3x3)m_W, float3x3(
                                                T.x,B.x,N.x,
                                                T.y,B.y,N.y,
                                                T.z,B.z,N.z
                                ));

        o.M1                 = xform        [0];
        o.M2                 = xform        [1];
        o.M3                 = xform        [2];

        float3         L_rgb         = v.color.xyz;
        float3         L_hemi         = v_hemi(N)*v.N.w;
        float3         L_sun         = v_sun(N)*v.color.w;
        float3         L_final        = L_rgb + L_hemi + L_sun + L_ambient;

        o.hpos                 = mul                        (m_VP, P);
		o.fog       = saturate( calc_fogging  (v.P));

		o.c0		= float4		(L_final,1);

#ifdef	USE_SOFT_WATER
#ifdef	NEED_SOFT_WATER
	o.tctexgen = mul( m_texgen, P);
	float3	Pe	= mul		(m_V,  P);
	o.tctexgen.z = Pe.z;
#endif
#endif
        return o;
}