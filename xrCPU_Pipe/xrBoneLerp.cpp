#include "stdafx.h"
#pragma hdrstop

#ifdef _EDITOR
 	#include "SkeletonAnimated.h"
#else
 	#include "..\SkeletonAnimated.h"
#endif

IC float myasin(const float x)
{
	static const float c1 = 0.892399f;
	static const float c3 = 1.693204f;
	static const float c5 =-3.853735f;
	static const float c7 = 2.838933f;

	const float x2 = x * x;
	const float d = x * (c1 + x2 * (c3 + x2 * (c5 + x2 * c7)));

	return d;
}


IC float myacos(const float x)
{
	return PI_DIV_2 - myasin(x);
}

IC	void	Qslerp(_quaternion<float>& D, _quaternion<float> &Q0, _quaternion<float> &Q1, float T)
{
	float Scale0,Scale1,sign;
	
	VERIFY( ( 0 <= T ) && ( T <= 1.0f ) );
	
	float cosom =	(Q0.w * Q1.w) + (Q0.x * Q1.x) + (Q0.y * Q1.y) + (Q0.z * Q1.z);
	
	if (cosom < 0) 	{
		cosom	= -cosom;
		sign	= -1.f;
	} else {
		sign	= 1.f;
	}
	
	if ( (1.0f - cosom) > EPS ) {
		float	omega	= myacos( cosom );
		float	i_sinom = 1.f / _sin( omega );
		float	t_omega	= T*omega;
		Scale0 = _sin( omega - 	t_omega ) * i_sinom;
		Scale1 = _sin( t_omega			) * i_sinom;
	} else  {
		// has numerical difficulties around cosom == 0
		// in this case degenerate to linear interpolation
		Scale0 = 1.0f - T;
		Scale1 = T;
	}
	Scale1 *= sign;

	D.x = Scale0 * Q0.x + Scale1 * Q1.x;
	D.y = Scale0 * Q0.y + Scale1 * Q1.y;
	D.z = Scale0 * Q0.z + Scale1 * Q1.z;
	D.w = Scale0 * Q0.w + Scale1 * Q1.w;
}

void __stdcall xrBoneLerp_x86	(CKey* D, CKeyQ* K1, CKeyQ* K2, float delta)
{
	Fquaternion	Q1,Q2;

	Q1.x		= float(K1->x)*KEY_QuantI;
	Q1.y		= float(K1->y)*KEY_QuantI;
	Q1.z		= float(K1->z)*KEY_QuantI;
	Q1.w		= float(K1->w)*KEY_QuantI;

	Q2.x		= float(K2->x)*KEY_QuantI;
	Q2.y		= float(K2->y)*KEY_QuantI;
	Q2.z		= float(K2->z)*KEY_QuantI;
	Q2.w		= float(K2->w)*KEY_QuantI;

	VERIFY		(delta>=0.f && delta<=1.f);
	D->Q.slerp	(Q1,Q2,delta);
	D->T.lerp	(K1->t,K2->t,delta);
}
