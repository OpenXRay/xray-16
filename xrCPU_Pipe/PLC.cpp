#include "stdafx.h"
#pragma hdrstop

const	float		S_distance		= 48;
const	float		S_distance2		= S_distance*S_distance;

const	float		S_fade			= 4.5;
const	float		S_fade2			= S_fade*S_fade;

// x86 -----------------------------------------------------------------------------------------------------

__inline float PLC_energy_x86(Fvector& P, Fvector& N, light* L, float E)
{
	Fvector Ldir;
	if (L->flags.type==IRender_Light::DIRECT)
	{
		// Cos
		Ldir.invert	(L->direction);
		float D		= Ldir.dotproduct( N );
		if( D <=0 )						return 0;
		return E;
	} else {
		// Distance
		float sqD	= P.distance_to_sqr(L->position);
		if (sqD > (L->range*L->range))	return 0;
		
		// Dir
		Ldir.sub	(L->position,P);
		Ldir.normalize_safe();
		float D		= Ldir.dotproduct( N );
		if( D <=0 )						return 0;

		// Trace Light
		float R		= _sqrt		(sqD);
		float att	= 1-(1/(1+R));
		return (E * att);
	}
}


void __stdcall PLC_calc3_x86(int& c0, int& c1, int& c2, CRenderDevice& Device, Fvector* P, Fvector& N, light* L, float energy, Fvector& O)
{
	float	E		= PLC_energy_x86(P[0],N,L,energy);
	float	C1		= clampr(Device.vCameraPosition.distance_to_sqr(P[0])/S_distance2,	0.f,1.f);
	float	C2		= clampr(O.distance_to_sqr(P[0])/S_fade2,							0.f,1.f);
	float	A		= 1.f-1.5f*E*(1.f-C1)*(1.f-C2);
	c0 = iCeil(255.f*A);
	E		= PLC_energy_x86(P[1],N,L,energy);
	C1		= clampr(Device.vCameraPosition.distance_to_sqr(P[1])/S_distance2,	0.f,1.f);
	C2		= clampr(O.distance_to_sqr(P[1])/S_fade2,							0.f,1.f);
	A		= 1.f-1.5f*E*(1.f-C1)*(1.f-C2);
	c1 = iCeil(255.f*A);
	E		= PLC_energy_x86(P[2],N,L,energy);
	C1		= clampr(Device.vCameraPosition.distance_to_sqr(P[2])/S_distance2,	0.f,1.f);
	C2		= clampr(O.distance_to_sqr(P[2])/S_fade2,							0.f,1.f);
	A		= 1.f-1.5f*E*(1.f-C1)*(1.f-C2);
	c2 = iCeil(255.f*A);
}

// SSE -----------------------------------------------------------------------------------------------------

__forceinline float PLC_energy_SSE(Fvector& P, Fvector& N, light* L, float E)
{
	Fvector Ldir;
	if (L->flags.type==IRender_Light::DIRECT)
	{
		// Cos
		Ldir.invert	(L->direction);
		float D		= Ldir.dotproduct( N );
		if( D <=0 )						return 0;
		return E;
	} else {
		// Distance
		float sqD	= P.distance_to_sqr(L->position);
		if (sqD > (L->range*L->range))	return 0;
		
		// Dir
		Ldir.sub	(L->position,P);
		Ldir.normalize_safe();
		float D		= Ldir.dotproduct( N );
		if( D <=0 )						return 0;

		// Trace Light
		float att;
		__m128 rcpr = _mm_rsqrt_ss( _mm_load_ss( &sqD ) );
		rcpr = _mm_rcp_ss( _mm_add_ss( rcpr , _mm_set_ss( 1.0f ) ) );
		_mm_store_ss( &att , rcpr );

		return (E * att);
	}
}

__forceinline int iCeil_SSE( float const x ) 
{
	return _mm_cvt_ss2si( _mm_set_ss( x ) );
}


void __stdcall PLC_calc3_SSE(int& c0, int& c1, int& c2, CRenderDevice& Device, Fvector* P, Fvector& N, light* L, float energy, Fvector& O)
{
	float	E		= PLC_energy_SSE(P[0],N,L,energy);
	float	C1		= clampr(Device.vCameraPosition.distance_to_sqr(P[0])/S_distance2,	0.f,1.f);
	float	C2		= clampr(O.distance_to_sqr(P[0])/S_fade2,							0.f,1.f);
	float	A		= 1.f-1.5f*E*(1.f-C1)*(1.f-C2);
	c0 = iCeil_SSE(255.f*A);
	E		= PLC_energy_SSE(P[1],N,L,energy);
	C1		= clampr(Device.vCameraPosition.distance_to_sqr(P[1])/S_distance2,	0.f,1.f);
	C2		= clampr(O.distance_to_sqr(P[1])/S_fade2,							0.f,1.f);
	A		= 1.f-1.5f*E*(1.f-C1)*(1.f-C2);
	c1 = iCeil_SSE(255.f*A);
	E		= PLC_energy_SSE(P[2],N,L,energy);
	C1		= clampr(Device.vCameraPosition.distance_to_sqr(P[2])/S_distance2,	0.f,1.f);
	C2		= clampr(O.distance_to_sqr(P[2])/S_fade2,							0.f,1.f);
	A		= 1.f-1.5f*E*(1.f-C1)*(1.f-C2);
	c2 = iCeil_SSE(255.f*A);
}
