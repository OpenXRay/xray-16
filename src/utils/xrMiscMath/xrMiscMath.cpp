#include "pch.hpp"

#include "xrCore/_vector3d.h"

bool exact_normalize(float* a)
{
	double sqr_magnitude = a[0] * a[0] + a[1] * a[1] + a[2] * a[2];
	double epsilon = 1.192092896e-05F;
	if (sqr_magnitude > epsilon)
	{
		double l = rsqrt(sqr_magnitude);
		a[0] *= l;
		a[1] *= l;
		a[2] *= l;
		return true;
	}
	double a0, a1, a2, aa0, aa1, aa2, l;
	a0 = a[0];
	a1 = a[1];
	a2 = a[2];
	aa0 = _abs(a0);
	aa1 = _abs(a1);
	aa2 = _abs(a2);
	if (aa1 > aa0)
	{
		if (aa2 > aa1)
		{
			goto aa2_largest;
		}
		else // aa1 is largest
		{
			a0 /= aa1;
			a2 /= aa1;
			l = rsqrt(a0*a0 + a2*a2 + 1);
			a[0] = a0*l;
			a[1] = (double)_copysign(l, a1);
			a[2] = a2*l;
		}
	}
	else
	{
		if (aa2 > aa0)
		{
		aa2_largest: // aa2 is largest
			a0 /= aa2;
			a1 /= aa2;
			l = rsqrt(a0*a0 + a1*a1 + 1);
			a[0] = a0*l;
			a[1] = a1*l;
			a[2] = (double)_copysign(l, a2);
		}
		else // aa0 is largest
		{
			if (aa0 <= 0)
			{
				// dDEBUGMSG ("vector has zero size"); ... this message is annoying
				a[0] = 0; // if all a's are zero, this is where we'll end up.
				a[1] = 1; // return a default unit length vector.
				a[2] = 0;
				return false;
			}
			a1 /= aa0;
			a2 /= aa0;
			l = rsqrt(a1*a1 + a2*a2 + 1);
			a[0] = (double)_copysign(l, a0);
			a[1] = a1*l;
			a[2] = a2*l;
		}
	}
	return true;
}

bool exact_normalize(Fvector3& a) { return exact_normalize(&a.x); }
