#ifndef	SHADOW_H
#define SHADOW_H

#include "common.h"

uniform	sampler	s_smap	: register(ps,s0);	// 2D/cube shadowmap

#define	KERNEL	.6f
//#define	USE_SJITTER
//////////////////////////////////////////////////////////////////////////////////////////
// software
//////////////////////////////////////////////////////////////////////////////////////////
half 	sample_sw	(float2 tc, float2 shift, float depth_cmp)
{
	static const float 	ts = KERNEL / float(SMAP_size);
	tc 		+= 		shift*ts;

	float  	texsize = SMAP_size;
	float  	offset 	= 0.5f/texsize;
	float2 	Tex00 	= tc + float2(-offset, -offset);
	float2 	Tex01 	= tc + float2(-offset,  offset);
	float2 	Tex10 	= tc + float2( offset, -offset);
	float2 	Tex11 	= tc + float2( offset,  offset);
	float4 	depth 	= float4(
		depth_cmp-tex2D	(s_smap, Tex00).x,
		depth_cmp-tex2D	(s_smap, Tex01).x,
		depth_cmp-tex2D	(s_smap, Tex10).x,
		depth_cmp-tex2D	(s_smap, Tex11).x);
	half4 	compare = step	(depth,0);
	float2 	fr 		= frac	(Tex00*texsize);
	half2 	ifr 	= half2	(1,1) - fr;
	half4 	fr4 	= half4	(ifr.x*ifr.y, ifr.x*fr.y, fr.x*ifr.y,  fr.x*fr.y);
	return	dot		(compare, fr4);
}
half 	shadow_sw	(float4 tc)	{ 
	float2	tc_dw	= tc.xy / tc.w;
	half4	s;
	s.x	= sample_sw	(tc_dw,float2(-1,-1),tc.z); 
	s.y	= sample_sw	(tc_dw,float2(+1,-1),tc.z); 
	s.z	= sample_sw	(tc_dw,float2(-1,+1),tc.z); 
	s.w	= sample_sw	(tc_dw,float2(+1,+1),tc.z);
	return	dot		(s, 1.h/4.h);
}

//////////////////////////////////////////////////////////////////////////////////////////
// hardware + PCF
//////////////////////////////////////////////////////////////////////////////////////////
half  	sample_hw_pcf	(float4 tc,float4 shift){
	static const float 	ts = KERNEL / float(SMAP_size);
#ifndef SUNSHAFTS_DYNAMIC
	return tex2Dproj	(s_smap,tc + tc.w*shift*ts).x;
#else	//	SUNSHAFTS_DYNAMIC
	float4 tc2 = tc / tc.w + shift * ts;
	tc2.w = 0;
	return tex2Dlod(s_smap, tc2);
#endif	//	SUNSHAFTS_DYNAMIC
}
half 	shadow_hw	(float4 tc)		{
  half  s0		= sample_hw_pcf	(tc,float4(-1,-1,0,0)); 
  half  s1		= sample_hw_pcf	(tc,float4(+1,-1,0,0)); 
  half  s2		= sample_hw_pcf	(tc,float4(-1,+1,0,0)); 
  half  s3		= sample_hw_pcf	(tc,float4(+1,+1,0,0));

  return	(s0+s1+s2+s3)/(4.h);
}

//////////////////////////////////////////////////////////////////////////////////////////
// hardware (ATI) + DF24/Fetch4
//////////////////////////////////////////////////////////////////////////////////////////

/*
half  	sample_hw_f4	(float4 tc,float4 shift){
	static const float 	ts 	= KERNEL / 	float(SMAP_size);
	float4	D4				= tex2Dproj	(s_smap,tc + tc.w*shift*ts);
	float4 	dcmp			= tc.z/tc.w	;
	float4	cmp				= dcmp<D4	;
	return 	dot	(cmp,1.h/4.h);
}
*/

half  	sample_hw_f4	(float4 tc,float4 shift){
	static const float 	ts 	= 	KERNEL / 	float(SMAP_size);
	float4 	T4				= 	tc/tc.w		;
			T4.xy			+=	shift.xy*ts	;

	float4	D4				= 	tex2D		(s_smap, T4);
	float4	compare			= 	T4.z<D4		;

	float  	texsize 		= 	SMAP_size	;
	float2 	fr 				= 	frac		(T4.xy * texsize);
	half2 	ifr 			= 	half2		(1,1) - fr;
	half4 	fr4 			= 	half4		(ifr.x*ifr.y, ifr.x*fr.y, fr.x*ifr.y,  fr.x*fr.y);
	half4 	fr4s		 	= 	fr4.zywx	;

	return	dot	(compare, fr4s)	;
	// return 	dot	(compare, 1.h/4.h)	;
}


half 	shadow_hw_f4	(float4 tc)		{
  half  s0	= sample_hw_f4	(tc,float4(-1,-1,0,0)); 
  half  s1	= sample_hw_f4	(tc,float4(+1,-1,0,0)); 
  half  s2	= sample_hw_f4	(tc,float4(-1,+1,0,0)); 
  half  s3	= sample_hw_f4	(tc,float4(+1,+1,0,0));
	return	(s0+s1+s2+s3)/4.h;
}



//////////////////////////////////////////////////////////////////////////////////////////
// testbed

uniform sampler2D	jitter0;
uniform sampler2D	jitter1;
uniform sampler2D	jitter2;
uniform sampler2D	jitter3;
uniform half4 		jitterS;
half4 	test 		(float4 tc, half2 offset)
{
	float4	tcx	= float4 (tc.xy + tc.w*offset, tc.zw);
	return 	tex2Dproj (s_smap,tcx);
}

half 	shadowtest 	(float4 tc, float4 tcJ)				// jittered sampling
{
	half4	r;

	const 	float 	scale 	= (2.7f/float(SMAP_size));
	half4	J0 	= tex2Dproj	(jitter0,tcJ)*scale;
	half4	J1 	= tex2Dproj	(jitter1,tcJ)*scale;

		r.x 	= test 	(tc,J0.xy).x;
		r.y 	= test 	(tc,J0.wz).y;
		r.z		= test	(tc,J1.xy).z;
		r.w		= test	(tc,J1.wz).x;

	return	dot(r,1.h/4.h);
}

half 	shadowtest_sun 	(float4 tc, float4 tcJ)			// jittered sampling
{
	half4	r;

	//	const 	float 	scale 	= (2.0f/float(SMAP_size));
	const 	float 	scale 	= (0.7f/float(SMAP_size));
	

	float2 	tc_J	= frac(tc.xy/tc.w*SMAP_size/4.0f )*.5f;
	half4	J0 	= tex2D	(jitter0,tc_J)*scale;
	//half4	J1 	= tex2D	(jitter1,tc_J)*scale;

	const float k = .5f/float(SMAP_size);
	r.x 	= test 	(tc, J0.xy+half2(-k,-k)).x;
	r.y 	= test 	(tc, J0.wz+half2( k,-k)).y;
	r.z		= test	(tc,-J0.xy+half2(-k, k)).z;
	r.w		= test	(tc,-J0.wz+half2( k, k)).x;

	return	dot(r,1.h/4.h);
}

half 	shadow_high 	(float4 tc)			// jittered sampling
{

	const	float 	scale 	= (0.5f/float(SMAP_size));

	float2 	tc_J	= frac(tc.xy/tc.w*SMAP_size/4.0f )*.5f;
	half4	J0 	= tex2D	(jitter0,tc_J)*scale;

	const float k = 1.f/float(SMAP_size);
	half4	r;
	r.x 	= test 	(tc,J0.xy+half2(-k,-k)).x;
	r.y 	= test 	(tc,J0.wz+half2( k,-k)).y;
	
 	r.z		= test	(tc,J0.xy+half2(-k, k)).z;
 	r.w		= test	(tc,J0.wz+half2( k, k)).x;
	

	const float k1 = 1.3f/float(SMAP_size);
	half4	r1;
	r1.x 	= test 	(tc,-J0.xy+half2(-k1,0)).x;
	r1.y 	= test 	(tc,-J0.wz+half2( 0,-k1)).y;

	r1.z	= test	(tc,-2*J0.xy+half2( k1, 0)).z;
 	r1.w	= test	(tc,-2*J0.wz+half2( 0, k1)).x;

	return ( r.x + r.y + r.z + r.w + r1.x + r1.y + r1.z + r1.w )*1.h/8.h;
}

//////////////////////////////////////////////////////////////////////////////////////////
// select hardware or software shadowmaps
//////////////////////////////////////////////////////////////////////////////////////////
#ifdef	USE_HWSMAP_PCF
// D24X8+PCF
	half 	shadow		(float4	tc)		{ return shadow_hw	(tc);	}
#else
	#ifdef USE_FETCH4
		// DF24+Fetch4
		half 	shadow 		(float4 tc)		{ return shadow_hw_f4(tc);	}
	#else
		// FP32
		half 	shadow		(float4 tc) 	{ return shadow_sw	(tc);	}
	#endif
#endif


#ifdef	USE_HWSMAP_PCF
	// D24X8+PCF
	half 	shadow_volumetric		(float4	tc)		{ return sample_hw_pcf	( tc, float4(0,0,0,0) ); }
#else
	#ifdef USE_FETCH4
		// DF24+Fetch4
		half 	shadow_volumetric 		(float4 tc)		{ return sample_hw_f4	(tc, float4(0,0,0,0)); }
	#else
		// FP32
		half 	shadow_volumetric 		(float4 tc) 	{ return sample_sw	(tc.xy / tc.w,float2(0,0),tc.z); }
	#endif
#endif

//////////////////////////////////////////////////////////////////////////////////////////
#ifdef  USE_SUNMASK	
uniform float3x4	m_sunmask	;				// ortho-projection
half 	sunmask		(float4 P)	{				// 
	float2 		tc	= mul	(m_sunmask, P);		//
	return 		tex2D 		(s_lmap,tc).w;		// A8 
	
}
#else
half 	sunmask		(float4 P)	{ return 1.h; }	// 
#endif

//////////////////////////////////////////////////////////////////////////////////////////
uniform float4x4	m_shadow;

#endif
