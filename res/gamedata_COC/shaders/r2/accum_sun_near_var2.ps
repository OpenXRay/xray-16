#include "common.h"
#include "lmodel.h"
#include "shadow.h"

#define	XKERNEL	.6f
half4 	xlight_infinity	(half m, half3 point, half3 normal, half3 light_direction)       				{
  half3 N		= normal;							// normal 
  half3 V 		= -normalize	(point);					// vector2eye
  half3 L 		= -light_direction;						// vector2light
  half3 H		= normalize	(L+V);						// half-angle-vector 
  return tex3Dlod	(s_material,	half4( dot(L,N), dot(H,N), m, 0 ) );		// sample material
}
half  	xsample_hw	(float4 tc,float4 shift){
	static const float 	ts = XKERNEL / float(SMAP_size);
	return tex2Dlod	(s_smap,tc + shift*ts).x;
}
half 	xshadow		(float4 tc, half old)		{
	float4	tcp	= tc.xyzw/tc.w;
  half  s0	= xsample_hw	(tcp,float4(-1,-1,0,0)); 
  half  s1	= xsample_hw	(tcp,float4(+1,-1,0,0)); 
  half  s2	= xsample_hw	(tcp,float4(-1,+1,0,0)); 
  half  s3	= xsample_hw	(tcp,float4(+1,+1,0,0));
	return	(s0+s1+s2+s3)/4.h	;
}
half 	xsunmask	(float4 P)	{		// 
	float2 		tc	= mul	(m_sunmask, P);	//
	return 		tex2Dlod(s_lmap,tc.xyyy).w;	// A8 
}
half  	xsample_h	(float4 tc,float4 shift){
	return tex2Dproj	(s_smap,tc + tc.w*shift).x;
}
uniform float4	o0,o1,o2,o3;
half 	xshadow_2	(float4 tc)		{
	static const float 	ts = KERNEL / float(SMAP_size);
	half4	s		;
  	s.x	= xsample_h	(tc,o0);	//float4(-1,-1,0,0)*ts); 
  	s.y	= xsample_h	(tc,o1);	//float4(+1,-1,0,0)*ts); 
  	s.z	= xsample_h	(tc,o2);	//float4(-1,+1,0,0)*ts); 
  	s.w	= xsample_h	(tc,o3);	//float4(+1,+1,0,0)*ts);
	return	(s.x+s.y+s.z+s.w)*0.25h; 	//dot(s,1.h/4.h)	;
}
half 	xshadow_3	(float4 tc)		{
	static const float 	ts = KERNEL / float(SMAP_size);
	half4	s		;
  	s.x	= xsample_h	(tc,float4(-1,-1,0,0)*ts); 
  	s.y	= xsample_h	(tc,float4(+1,-1,0,0)*ts); 
  	s.z	= xsample_h	(tc,float4(-1,+1,0,0)*ts); 
  	s.w	= xsample_h	(tc,float4(+1,+1,0,0)*ts);
	return	(s.x+s.y+s.z+s.w)*0.25h; 	//dot(s,1.h/4.h)	;
}

//////////////////////////////////////////////////////////////////////////////////////////
// hardware
//////////////////////////////////////////////////////////////////////////////////////////
#ifdef USE_SUNFILTER
float4 	main		( float2 tc : TEXCOORD0, float4 tcJ : TEXCOORD1 ) : COLOR
{
  float4 	_P	= tex2D 	(s_position, 	tc)	;
		_P.w 	= 1.f					;
	float4 	PS	= mul		(m_shadow, 	_P)	;
	half 	s 	= shadowtest_sun(PS,tcJ)*sunmask(_P)	;
	return 	s	;
}
#else
float4 	main		( float4 tc : TEXCOORD0, float4 tcJ : TEXCOORD1 ) : COLOR
{
  float4 _P		= tex2D 	(s_position, 	tc); 

	// ----- shadow
  	float4 	P4 	= float4	(_P.x,_P.y,_P.z,1.f)	;
	float4 	PS	= mul		(m_shadow, 	P4)	;
	half	s	= xshadow_2 	(PS);
	half4	light 	= 0;		//half4	(1,0,0,0)	;
	if (s>0.0001)	{
  		half4 _N= tex2D 	(s_normal,   	tc	); 
		light 	= Ldynamic_color * s * xsunmask	(P4) * xlight_infinity (_P.w,_P,_N,Ldynamic_dir);
	}
	return 		blend		( light, tc );
}
#endif
