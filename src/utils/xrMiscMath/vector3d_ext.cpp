#include "pch.hpp"
#include "xrCommon/math_funcs_inline.h"
#include "xrCore/_vector3d_ext.h"

float dotproduct(const Fvector& v1, const Fvector& v2)
{
	return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z;
}

Fvector crossproduct(const Fvector& v1, const Fvector& v2)
{
	Fvector r;
	r.crossproduct(v1, v2);
	return r;
}

Fvector cr_vectorHP(float h, float p)
{
	float ch = _cos(h), cp = _cos(p), sh = _sin(h), sp = _sin(p);
	Fvector r;
	r.x = -cp*sh;
	r.y = sp;
	r.z = cp*ch;
	return r;
}

float angle_between_vectors(Fvector const v1, Fvector const v2)
{
	float const mag1 = v1.magnitude();
	float const mag2 = v2.magnitude();
	float const epsilon = 1e-6f;
	if (mag1 < epsilon || mag2 < epsilon)
	{
		return 0.f;
	}

	float angle_cos = dotproduct(v1, v2) / (mag1*mag2);
	if (angle_cos < -1.f)
	{
		angle_cos = -1.f;
	}
	else if (angle_cos > +1.f)
	{
		angle_cos = +1.f;
	}
	return acosf(angle_cos);
}

Fvector rotate_point(Fvector const& point, float const angle)
{
	float const cos_alpha = _cos(angle);
	float const sin_alpha = _sin(angle);

	return Fvector().set(point.x*cos_alpha - point.z*sin_alpha,
		0,
		point.x*sin_alpha + point.z*cos_alpha);
}
