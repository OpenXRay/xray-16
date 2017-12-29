
out gl_PerVertex { vec4 gl_Position; };

struct 	a2v
{
	float4	P	; // POSITION;		// Object-space position
 	float2	tc0	; // TEXCOORD0;		// Texture coordinates
};

layout(location = NORMAL)		in float4	v_static_Nh		; // NORMAL;	// (nx,ny,nz,hemi occlusion)
layout(location = TANGENT)		in float4	v_static_T		; // TANGENT;	// tangent
layout(location = BINORMAL)		in float4	v_static_B		; // BINORMAL;	// binormal
layout(location = TEXCOORD0)	in float2	v_static_tc		; // TEXCOORD0;	// (u,v)
#ifdef	USE_LM_HEMI
layout(location = TEXCOORD1)	in float2	v_static_lmh	; // TEXCOORD1;	// (lmu,lmv)
#endif
layout(location = POSITION)		in float4	v_static_P		; // POSITION;	// (float,float,float,1)

layout(location = TEXCOORD1)	out float2	v2p_shadow_tc0	; // TEXCOORD1;	// Diffuse map for aref

v2p_shadow_direct_aref _main ( v_static I );

void main()
{
	v_static	I;
	I.Nh		= v_static_Nh;
	I.T			= v_static_T;
	I.B			= v_static_B;
	I.tc		= v_static_tc;
#ifdef	USE_LM_HEMI
	I.lmh		= v_static_lmh;
#endif
	I.P			= v_static_P;

	v2p_shadow_direct_aref O = _main (I);

	v2p_shadow_tc0 = O.tc0;
	gl_Position = O.hpos;
}
