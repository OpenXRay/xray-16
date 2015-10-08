#include "stdafx.h"
#include "raypick.h"
#include "level.h"

CRayPick::CRayPick() 
{
	start_position.set(0,0,0); 
	direction.set(0,0,0); 
	range = 0; 
	flags = collide::rq_target::rqtNone; 
	ignore = NULL;
};

CRayPick::CRayPick(const Fvector& P, const Fvector& D, float R, collide::rq_target F, CScriptGameObject* I)
{
	start_position.set(P); 
	direction.set(D);
	range = R; 
	flags = F; 
	ignore = NULL;
	if (I)
		ignore = smart_cast<CObject*>(&(I->object()));
};

bool	CRayPick::query		()
{
	collide::rq_result		R;
	if (Level().ObjectSpace.RayPick		(start_position, direction, range, flags, R, ignore))
	{
		result.set(R);
		return true;
	} 
	else 
		return false;
}