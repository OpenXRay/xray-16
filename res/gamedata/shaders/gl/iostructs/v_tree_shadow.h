
out gl_PerVertex { vec4 gl_Position; };

layout(location = POSITION)		in float4	v_shadow_P		; // POSITION;		// (float,float,float,1)
#ifdef	USE_AREF
layout(location = TEXCOORD0)	in float4	v_shadow_tc		; // TEXCOORD0;	// (u,v,frac,???)
#endif	//	USE_AREF

#ifdef	USE_AREF
layout(location = TEXCOORD1) 	out float2	v2p_shadow_tc0	; // TEXCOORD1;	// Diffuse map for aref
#endif	//	USE_AREF

#ifdef	USE_AREF
v2p_shadow_direct_aref _main ( v_shadow_direct_aref I );
#else	//	USE_AREF
v2p_shadow_direct _main ( v_shadow_direct I );
#endif	//	USE_AREF

void main()
{
#ifdef	USE_AREF
	v_shadow_direct_aref	I;
	I.tc		= v_shadow_tc;
#else	//	USE_AREF
	v_shadow_direct			I;
#endif	//	USE_AREF
	I.P			= v_shadow_P;

#ifdef	USE_AREF
	v2p_shadow_direct_aref O = _main (I);
	v2p_shadow_tc0 = O.tc0;
#else	//	USE_AREF
	v2p_shadow_direct O = _main (I);
#endif	//	USE_AREF

	gl_Position = O.hpos;
}
