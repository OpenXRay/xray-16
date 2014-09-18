#include	"stdafx.h"
#include	"base_lighting.h"

void	base_lighting::select	(xr_vector<R_Light>& dest, xr_vector<R_Light>& src, Fvector& P, float R)
{
	Fsphere		Sphere;
	Sphere.set	(P,R);
	dest.clear	();
	R_Light*	L			= &*src.begin();
	for (; L!=&*src.end(); L++)
	{
		if (L->type==LT_POINT) {
			float dist						= Sphere.P.distance_to(L->position);
			if (dist>(Sphere.R+L->range))	continue;
		}
		dest.push_back(*L);
	}
}

void	base_lighting::select	(base_lighting& from, Fvector& P, float R)
{
	select(rgb,from.rgb,P,R);
	select(hemi,from.hemi,P,R);
	select(sun,from.sun,P,R);
}