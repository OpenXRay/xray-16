#include "common.h"
#include "shared\waterconfig.h"
#include "shared\watermove.h"
#include "iostructs\v_water.h"

uniform float4x4 m_texgen;

v2p _main (v_vert v)
{
	v.N		= unpack_D3DCOLOR(v.N);
	v.T		= unpack_D3DCOLOR(v.T);
	v.B		= unpack_D3DCOLOR(v.B);
	v.color		= unpack_D3DCOLOR(v.color);

	v2p		o;

        float4	P	= v.P;                				// world
        float3	NN	= unpack_normal(v.N);
		P	= watermove(P);

	o.v2point	= P.xyz-eye_position;
	o.tbase		= unpack_tc_base	(v.uv,v.T.w,v.B.w);	// copy tc
	o.tnorm0	= watermove_tc		(o.tbase*W_DISTORT_BASE_TILE_0, P.xz, W_DISTORT_AMP_0);
	o.tnorm1	= watermove_tc		(o.tbase*W_DISTORT_BASE_TILE_1, P.xz, W_DISTORT_AMP_1);

	// Calculate the 3x3 transform from tangent space to eye-space
	// TangentToEyeSpace = object2eye * tangent2object
	//                     = object2eye * transpose(object2tangent) (since the inverse of a rotation is its transpose)
	float3		N	= unpack_bx2(v.N);			// just scale (assume normal in the -0.5, 0.5)
	float3		T	= unpack_bx2(v.T);			//
	float3		B	= unpack_bx2(v.B);			//
	float3x3	xform	= mul(float3x3(m_W), float3x3(T,B,N));
	// The pixel shader operates on the bump-map in [0..1] range
	// Remap this range in the matrix, anyway we are pixel-shader limited :)
	// ...... [ 2  0  0  0]
	// ...... [ 0  2  0  0]
	// ...... [ 0  0  2  0]
	// ...... [-1 -1 -1  1]
	// issue: strange, but it's slower :(
	// issue: interpolators? dp4? VS limited? black magic?

	// Feed this transform to pixel shader
	o.M1		= xform[0];
	o.M2		= xform[1];
	o.M3		= xform[2];

	float3 L_rgb	= v.color.xyz;						// precalculated RGB lighting
	float3 L_hemi	= v_hemi(N)*v.N.w;					// hemisphere
	float3 L_sun	= v_sun(N)*v.color.w;					// sun
	float3 L_final	= L_rgb + L_hemi + L_sun + L_ambient.rgb;

	o.hpos		= mul		(m_VP, P);				// xform, input in world coords
	o.fog		= saturate	(calc_fogging(v.P));
	//o.fog		*= o.fog;
	o.c0		= float4	(L_final, 1.0);

#if defined(USE_SOFT_WATER) && defined(NEED_SOFT_WATER)
	o.tctexgen	= mul		(m_texgen, P);
	float3	Pe	= mul		(m_V, P).xyz;
	o.tctexgen.z	= Pe.z;
#endif	// defined(USE_SOFT_WATER) && defined(NEED_SOFT_WATER)

	return o;
}