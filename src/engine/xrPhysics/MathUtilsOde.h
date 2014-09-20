#pragma once

#pragma warning(disable:4995)
#pragma warning(disable:4267)
#include "../3rd party/ode/include/ode/common.h"
#include "../3rd party/ode/include/ode/odemath.h"
#include "../3rd party/ode/include/ode/objects.h"
#include "../3rd party/ode/include/ode/rotation.h"
#include "../3rd party/ode/include/ode/compatibility.h"
#include "../3rd party/ode/include/ode/collision.h"
#include "../3rd party/ode/include/ode/matrix.h"

#include "mathutils.h"
#include "ode_redefine.h"

static const	dReal	accurate_normalize_epsilon			= 1.192092896e-05F;

ICF void	accurate_normalize(float* a)
{
	dReal	sqr_magnitude	= a[0]*a[0] + a[1]*a[1] + a[2]*a[2];

	if		(sqr_magnitude > accurate_normalize_epsilon)
	{
		dReal	l	=	dRecipSqrt(sqr_magnitude);
		a[0]		*=	l;
		a[1]		*=	l;
		a[2]		*=	l;
		return;
	}

	dReal a0,a1,a2,aa0,aa1,aa2,l;
	VERIFY (a);
	a0 = a[0];
	a1 = a[1];
	a2 = a[2];
	aa0 = dFabs(a0);
	aa1 = dFabs(a1);
	aa2 = dFabs(a2);
	if (aa1 > aa0) {
		if (aa2 > aa1) {
			goto aa2_largest;
		}
		else {		// aa1 is largest
			a0 /= aa1;
			a2 /= aa1;
			l = dRecipSqrt (a0*a0 + a2*a2 + 1);
			a[0] = a0*l;
			a[1] = (float)_copysign(l,a1);
			a[2] = a2*l;
		}
	}
	else {
		if (aa2 > aa0) {
aa2_largest:	// aa2 is largest
			a0 /= aa2;
			a1 /= aa2;
			l = dRecipSqrt (a0*a0 + a1*a1 + 1);
			a[0] = a0*l;
			a[1] = a1*l;
			a[2] = (float)_copysign(l,a2);
		}
		else {		// aa0 is largest
			if (aa0 <= 0) {
				// dDEBUGMSG ("vector has zero size"); ... this messace is annoying
				a[0] = 1;	// if all a's are zero, this is where we'll end up.
				a[1] = 0;	// return a default unit length vector.
				a[2] = 0;
				return;
			}
			a1 /= aa0;
			a2 /= aa0;
			l = dRecipSqrt (a1*a1 + a2*a2 + 1);
			a[0] = (float)_copysign(l,a0);
			a[1] = a1*l;
			a[2] = a2*l;
		}
	}
}

IC	bool	dVectorLimit(const float* v,float l,float* lv)
{
	float mag		=	_sqrt(dDOT(v,v));
	if(mag>l)
	{
		float f=mag/l;
		lv[0]=v[0]/f;lv[1]=v[1]/f;lv[2]=v[2]/f;
		return true;
	}
	else
	{
		dVectorSet(lv,v);
		return false;
	}
}

IC	void	dVectorInterpolate(float* res,const float* from,const float* to,float k) //changes to
{
	dVector3 tov;
	dVectorSet(res,from);
	dVectorSet(tov,to);
	dVectorInterpolate(res,tov,k);
}

float E_NL( dBodyID b1, dBodyID b2, const dReal* norm );

float E_NlS( dBodyID body, const dReal* norm, float norm_sign );//if body c.geom.g1 norm_sign + else -

#pragma warning(default:4995)
#pragma warning(default:4267)