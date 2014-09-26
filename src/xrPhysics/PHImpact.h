#ifndef PH_IMPACT_H
#define PH_IMPACT_H

struct SPHImpact 
{
	Fvector force;
	Fvector point;
	u16		geom;
	SPHImpact(const Fvector& aforce,const Fvector& apoint,u16 root_geom){force.set(aforce);point.set(apoint);geom=root_geom;}
};
DEFINE_VECTOR(SPHImpact,PH_IMPACT_STORAGE,PH_IMPACT_I)

#endif